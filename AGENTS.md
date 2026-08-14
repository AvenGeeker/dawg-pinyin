# AGENTS.md — dawg-pinyin (DawgIme)

本项目是一个跨平台拼音输入法工程，采用 AI 全流程开发。本文件是 AI 代理（Codex 等）
在本仓库工作时必须遵守的约定：工程结构、工具链版本、C++ 规范、构建与单元测试规范。

## 1. 项目概览与目录结构

核心是一个基于 DAWG（Directed Acyclic Word Graph）词库的拼音输入引擎，C++ 实现、
跨平台复用；各平台只做薄壳（UI + 输入法框架对接）。

```
dawg-pinyin/
├── core/                      # C++ 核心引擎（唯一允许包含算法逻辑的地方）
│   ├── CMakeLists.txt
│   ├── include/dawg/          # 对外公共头文件（namespace dawg）
│   ├── src/                   # 实现
│   │   ├── dawg/              # DAWG 词库结构与序列化
│   │   └── pinyin/            # 拼音切分、音节解码、候选生成
│   ├── tests/                 # GoogleTest 单元测试（*_test.cc）
│   └── testdata/              # 测试用词库/语料（小文件，大的二进制不入库）
├── platform/
│   ├── DawgIme/               # iOS 端（Xcode 工程，SwiftUI + 键盘扩展）
│   ├── Android/               # Android 端（Gradle，Kotlin + Compose + JNI）
│   └── HarmonyOS/             # 鸿蒙端（DevEco，ArkTS + NAPI）
├── tools/
│   ├── lm/                    # charlm 训练管线（Python）
│   └── assets/build_assets.sh # 生产词库一键构建（产物 assets/，不入库）
└── AGENTS.md
```

架构红线：
- `core/` 是纯 C++，**不得**依赖平台 SDK（UIKit / Android / JNI 头文件）、
  不得依赖第三方运行时库；允许的标准库之外依赖只有测试框架（仅测试目标可用）。
- 平台层不得实现拼音/词库算法，只能调用 core 的公共 API。
- core 对外 API 放在 `include/dawg/`，跨 ABI 边界（JNI / Swift 桥接）使用
  `extern "C"` 接口或 pimpl 隔离，内部 C++ 类型不外泄。

## 2. 工具链与环境（以本机实测为准）

| 组件 | 版本 | 说明 |
| --- | --- | --- |
| macOS | 26.2 | 开发宿主机 |
| Xcode | 26.5 | Apple clang 21.0 |
| C++ 标准 | **C++20** | `-std=c++20`，禁止 GNU 扩展（不用 `-std=gnu++20`） |
| CMake | ≥ 3.22（本机 3.23.2） | `cmake_minimum_required(VERSION 3.22)` |
| Android NDK | **25.1.8937393 (r25)** | 统一使用该版本，勿用 r21 |
| Gradle | 8.9（wrapper 已内置） | 一律用 `./gradlew`，不用全局 gradle |
| AGP / Kotlin | 8.7.3 / 2.0.0 | 版本集中在 `gradle/libs.versions.toml` |
| Android SDK | compileSdk 34 / minSdk 21 / targetSdk 34 | JVM target 11 |
| Swift | 5.0（Xcode 工程设置） | iOS deployment target 26.5 |

构建生成器默认 Unix Makefiles（本机未装 ninja；如安装 ninja 可用但不强制）。

## 3. C++ 编码规范

- 标准：C++20。核心库**禁用异常与 RTTI**（`-fno-exceptions -fno-rtti`），
  错误处理用返回值（错误码 / `std::optional` / `std::expected` 风格）。
- 编译警告：`-Wall -Wextra -Werror`（测试目标可放宽 `Werror`）。新代码必须零警告。
- 风格遵循 Google C++ Style，要点：
  - 文件名 `pinyin_decoder.h` / `pinyin_decoder.cc`；头文件使用 `#pragma once`。
  - 命名空间 `dawg::`（内部实现可用 `dawg::detail`），禁止 `using namespace std;`。
  - 类型 `PinyinDecoder`；函数/方法 `Decode()`；变量 `input_len`；
    常量 `kMaxSyllableLen`；枚举值 `ErrorCode::kInvalidInput`；宏 `DAWG_` 前缀。
  - 公共头文件只前向声明，尽量不暴露实现细节；头文件自包含（self-contained）。
- 每个新建源文件头部加 Apache-2.0 SPDX 注释：`// SPDX-License-Identifier: Apache-2.0`。
- 注释与标识符用英文；面向用户的文档可用中文。
- 格式化：如环境装有 clang-format，提交前按仓库 `.clang-format`（Google 基础）格式化；
  没有时以手工保持上述风格一致，不因格式问题阻塞。

## 4. 构建（CMake）

`core/CMakeLists.txt` 约定：

```cmake
cmake_minimum_required(VERSION 3.22)
project(dawg_core LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAWG_BUILD_TESTS "Build unit tests" ON)
```

常用命令（在仓库根目录执行）：

```bash
# 宿主机构建（Debug，含测试）
cmake -S core -B build/core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core -j8

# 运行测试
ctest --test-dir build/core --output-on-failure

# Android 交叉编译（供 Gradle externalNativeBuild 或手动验证）
cmake -S core -B build/core-android \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/Library/Android/sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DDAWG_BUILD_TESTS=OFF
```

要求：core 的 CMake 必须同时支持宿主测试构建与 NDK 交叉编译；
测试目标（含 GoogleTest 下载）只在 `DAWG_BUILD_TESTS=ON` 且宿主构建时启用，
交叉编译默认关闭，避免拖慢平台构建。

## 5. 单元测试规范（core / C++）

- 框架：**GoogleTest**，通过 `FetchContent` 固定版本 **v1.15.2**，禁止跟随 `main`。
- 位置与命名：
  - 测试文件放 `core/tests/`，命名 `<被测模块>_test.cc`（如 `pinyin_decoder_test.cc`），
    与被测源文件一一对应；一个 `add_executable` + `gtest_discover_tests`。
  - 测试套件名 = 被测类名 + `Test`（如 `PinyinDecoderTest`）；
    用例名描述行为与预期：`DecodesSingleSyllable`、`ReturnsEmptyOnInvalidInput`。
- 写法要求：
  - 每个 TEST 只验证一个行为，按 Arrange–Act–Assert 组织；
    断言用 `EXPECT_*`，致命前置条件用 `ASSERT_*`。
  - 共享构造/词库加载放 `SetUp()` fixture，不在用例里重复。
  - 测试必须自包含：用 `core/testdata/` 的小词库或临时目录，不依赖绝对路径、
    网络、当前工作目录之外的文件。
  - 边界必测：空输入、超长输入、非法音节、词库缺失/损坏文件。
- 覆盖率门槛：新增/修改 core 逻辑必须附带或更新对应测试；修 bug 必须先写一个
  能复现该 bug 的失败测试，再修复。
- 验证命令（代理在声明“完成”前必须实际执行并通过）：

```bash
cmake --build build/core -j8 && ctest --test-dir build/core --output-on-failure
```

## 6. 平台端规范

### iOS（platform/DawgIme，Xcode 工程）
- Bundle ID `com.avengeeker.DawgIme`；UI 用 SwiftUI；后续输入法扩展用
  Keyboard Extension target，引擎通过 core 编译出的静态库/XCFramework 接入。
- 命令行构建/测试：

```bash
xcodebuild -project platform/DawgIme/DawgIme.xcodeproj -scheme DawgIme \
  -destination 'generic/platform=iOS Simulator' build
```

- 不要手工编辑 `xcuserdata/`、`UserInterfaceState` 等用户态文件，不得提交。

### Android（platform/Android，Gradle）
- 所有命令在 `platform/Android/` 下用 wrapper 执行：

```bash
./gradlew :app:testDebugUnitTest          # 单元测试（JUnit4）
./gradlew :app:assembleDebug              # 构建
./gradlew :app:connectedDebugAndroidTest  # 设备/模拟器测试
```

- Kotlin 官方代码风格；依赖版本只改 `gradle/libs.versions.toml`。
- core 接入方式：app-local CMake（`app/src/main/cpp/CMakeLists.txt`）
  `add_subdirectory` 引入 core 并构建 `dawg_jni` 共享库 + JNI 薄封装
  （`System.loadLibrary("dawg_jni")`）。不直接以 core/CMakeLists.txt 作为
  externalNativeBuild 入口，避免把 JNI 目标塞进 core。JNI 层只做类型转换，
  不写逻辑（契约由 `core/tests/capi_test.cc` 固化）。
- `local.properties`、`.idea/`、`.gradle/` 不入库。

### 鸿蒙（platform/HarmonyOS，DevEco 工程）
- Stage 模型；bundleName `com.avengeeker.dawgime`；键盘为
  `DawgIME`（InputMethodExtensionAbility，`type: inputMethod`），启用指引
  页在 entry 的 `EntryAbility`。
- core 接入方式：`entry/src/main/cpp/CMakeLists.txt` `add_subdirectory`
  引入 core，产出 NAPI 模块 `libdawg_napi.so`（create/search/candidateText/
  destroy，与 C API 一一对应），同样只做类型转换。
- 词库资产经 `tools/sync_assets.sh` 同步到 rawfile，扩展启动时复制到
  filesDir（mmap 需要真实文件路径）。
- 已知 SDK 限制（详见 platform/HarmonyOS/README.md）：本机 DevEco 3.1
  为 API 9 且缺 Native 组件；键盘面板 Panel API 需 API 10+，因此该端
  源码交付、编译后置。

### 三端键盘一致性约定
- 键盘度量/交互规范三端逐字一致（候选栏 44、键高 42、行距 12、键距 6、
  边距 3、圆角 4；拼写缓冲 ≤64、空格上屏首候选、退格先删缓冲、回车
  上屏原始字母、shift 一次性大写、符号页直插）。实现文件：
  `KeySpec.swift` / `KeySpec.kt` / `KeySpec.ets`（度量与布局）+
  `KeyboardModel.swift` / `ImeSession.kt` / `KeyboardState.ets`（状态机）。
  改动任一端必须同步另两端并更新对应单测。

## 7. Git 与协作约定

- 分支前缀 `codex/`（如 `codex/dawg-builder`）；主干为 `main`，禁止直接 push main。
- 提交信息用 Conventional Commits（英文）：`feat(core): build DAWG from word list`、
  `test(pinyin): cover ambiguous syllable splits`、`fix:`、`chore:`、`docs:`。
- 提交前检查清单：构建通过、`ctest` 全绿、无新增编译警告、没有混入
  `.DS_Store` / `build/` / IDE 用户态文件。
- 许可证 Apache-2.0：引入任何第三方代码/数据前确认许可证兼容（允许 MIT/BSD/Apache-2.0，
  禁止 GPL 系）。

## 8. AI 开发工作守则

- 小步提交：一次改动聚焦一个模块（core / iOS / Android 不混提）。
- 改动 core 公共 API 时，同步更新所有平台调用点或明确标注 TODO。
- 不确定的行为先写测试固化预期，再实现。
- 遇到需要联网下载依赖（FetchContent、Gradle 依赖）而沙箱网络受限时，
  说明原因并请求提权重试，不要绕过或 vendor 不明来源的代码。
- 生成物（构建产物、词典二进制、IDE 状态）一律不入库；仓库根目录应维护 `.gitignore`。
