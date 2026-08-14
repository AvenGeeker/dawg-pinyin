# dawg-pinyin

跨平台拼音输入法工程：核心为基于 DAWG 词库与字符级 n-gram 语言模型
（charlm）的 C++20 输入引擎，平台层为薄壳。

- `core/` —— 引擎核心（唯一包含算法逻辑的层），见 [core/README.md](core/README.md)
- `platform/DawgIme/` —— iOS 端（Xcode / SwiftUI）：App + `DawgImeKeyboard`
  键盘扩展 + `DawgCore` 静态库（直接编译 core 源，C API 经桥接头接入）
- `platform/Android/` —— Android 端（Gradle / Kotlin / Compose）：
  `DawgInputMethodService` + app-local CMake（`add_subdirectory` core）
  产出 `libdawg_jni.so`
- `platform/HarmonyOS/` —— 鸿蒙端（DevEco / ArkTS，Stage 模型）：
  `DawgIME` InputMethodExtensionAbility + NAPI 桥（见该目录 README 的
  SDK 版本要求）
- `tools/lm/` —— charlm 训练管线（Python，语料不入库）
- `tools/assets/build_assets.sh` —— 生产词库一键构建（Rime luna_pinyin →
  `.dawg`；zhwiki + LCCC 语料 → charlm → `.clm`），产物落在 `assets/`
  （不入库；平台构建缺失时自动回退 tiny 测试资产）

三端键盘 UI 遵循同一份度量/交互规范（苹果键盘尺寸：候选栏 44、键高 42、
间距 6 等），分别实现于 `KeySpec.swift` / `KeySpec.kt` / `KeySpec.ets`，
改动任一端需同步另两端。

工程约定（工具链、C++ 规范、构建与测试、Git 流程）见 [AGENTS.md](AGENTS.md)。
