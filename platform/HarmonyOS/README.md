# HarmonyOS 端（DawgIme 拼音键盘）

DevEco Studio 工程（Stage 模型 / ArkTS），壳 + 引擎桥接，全部算法逻辑在
仓库 `core/`（经 NAPI 薄封装接入）。

## 目录

```
AppScope/                     bundle 配置（com.avengeeker.dawgime）
entry/src/main/module.json5   EntryAbility（启用指引）+ DawgIME（inputMethod 扩展）
entry/src/main/ets/
  inputmethod/DawgIME.ets     InputMethodExtensionAbility：资产落盘、引擎初始化、面板显隐
  keyboard/KeySpec.ets        与 iOS/Android 逐字一致的度量与布局公式（vp）
  keyboard/KeyboardState.ets  与 iOS/Android 一致的输入状态机
  keyboard/DawgEngine.ets     libdawg_napi.so 封装
  pages/KeyboardPanel.ets     键盘面板 UI（候选栏 + 按键区）
entry/src/main/cpp/           CMake + dawg_napi.cc（add_subdirectory core）
entry/src/main/resources/rawfile/  词库资产（tools/sync_assets.sh 同步，不入库）
```

## 构建（本期未在本机执行）

本机 DevEco Studio 3.1 仅带 OpenHarmony **API 9** SDK，且**缺少 Native
组件**，因此本工程按约定只交付源码，未做本机编译验证。编译前需要：

1. DevEco SDK Manager 安装 **Native** 组件（NAPI/CMake 支持）。
2. 键盘面板 `Panel`/`KeyboardController` 显隐 API 在 **API 10+** 才提供
   （API 9 SDK 的 `@ohos.inputMethodEngine` 无 `Panel`）。请将
   `build-profile.json5` 的 compile/compatibleSdkVersion 提升到 10 或以上
   （推荐安装 API 12 后打开工程），否则 `DawgIME.ets` 的 Panel 相关代码
   无法通过编译。装好 SDK 后若出现编译报错由后续任务修复。

```bash
bash tools/sync_assets.sh        # 同步词库资产到 rawfile
# DevEco 打开本工程 -> Build Hap(s)；或命令行 hvigorw assembleHap
hdc install entry/build/default/outputs/default/entry-default-signed.hap
```

## 启用键盘

设置 → 系统和更新 → 语言和输入法 → 添加 DawgIme Pinyin。

## 已知限制

- API 9 `InputClient` 无「读取光标前文本」接口：LM 上下文仅由本输入法
  已上屏文本内部追踪（见 `KeyboardState.ets` 注释）。
- 竖屏专用；横屏维持竖屏布局。
