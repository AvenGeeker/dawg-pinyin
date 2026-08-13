# dawg-pinyin

跨平台拼音输入法工程：核心为基于 DAWG 词库与字符级 n-gram 语言模型
（charlm）的 C++20 输入引擎，平台层为薄壳。

- `core/` —— 引擎核心（唯一包含算法逻辑的层），见 [core/README.md](core/README.md)
- `platform/DawgIme/` —— iOS 端（Xcode / SwiftUI，键盘扩展规划中）
- `platform/Android/` —— Android 端（Gradle / Kotlin）
- `tools/lm/` —— charlm 训练管线（Python，语料不入库）
- 鸿蒙端（InputMethodExtension）后续立项

工程约定（工具链、C++ 规范、构建与测试、Git 流程）见 [AGENTS.md](AGENTS.md)。
