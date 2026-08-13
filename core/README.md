# dawg core —— 拼音输入引擎（C++20，跨平台）

纯 C++20 核心引擎：DAWG 词库 + 字符级 n-gram 语言模型（charlm）+
音节切分 / 词格 beam 解码。无第三方运行时依赖、禁异常与 RTTI，
Android / iOS / 鸿蒙三端通过薄桥接层复用（JNI / Swift / NAPI）。

## 目录

```
include/dawg/    公共头：engine.h / candidate.h / error.h / dawg_capi.h
src/util/        mmap 只读文件、UTF-8、小端字段读写
src/pinyin/      音节表、音节 DAG 切分、词格构建、beam 解码
src/dawg/        DAWG 词库：DawgReader（运行时）/ DawgBuilder（离线）
src/lm/          charlm 读取器与 .clm 格式常量
tools/           dawg_build_dict / dawg_import_rime / dawg_bench（宿主工具）
tests/           GoogleTest v1.15.2（FetchContent 固定版本）
testdata/        自造小词库 / 小语料与生成资产（tiny.dawg / tiny.clm）
```

## 构建与测试

```bash
cmake -S core -B build/core -DCMAKE_BUILD_TYPE=Debug   # 首次需联网下载 GoogleTest
cmake --build build/core -j8
ctest --test-dir build/core --output-on-failure
```

NDK 交叉编译（测试自动关闭）：

```bash
cmake -S core -B build/core-android \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/Library/Android/sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DDAWG_BUILD_TESTS=OFF
```

## 解码流水线

1. `Syllabifier`：小写字母 → 音节 DAG（全拼边 + 首字母简拼边，保留
   `xian` 类切分歧义）。
2. `LatticeBuilder`：音节 DAG × DAWG 词库同步遍历，枚举所有
   （词, 音节跨度）词格边。
3. `Decoder`：beam Viterbi，路径分 = 归一化词库权重 + charlm 增量打分；
   语言模型上下文 = 已上屏文本 + 路径已选词，因此**后分词由 LM 驱动**。
4. 候选 = 句候选（多词路径）/ 词候选 / 单字候选；按真实上文重排、
   去重、按 beam 下限过滤。

## 资产格式

- `.dawg`：头部 + 以音节序列为键的最小化 FSA + 紧凑词条池
  （详见 `src/dawg/dawg_format.h`）。构建：

  ```bash
  ./build/core/dawg_build_dict dict.txt out.dawg        # word\t拼音\t权重
  ./build/core/dawg_import_rime rime_dict.txt out.txt   # Rime 词典导入器
  ```

- `.clm`：打包 trie + 8-bit 量化 log10 概率/backoff
  （详见 `src/lm/char_lm_format.h`）。训练管线见 `../tools/lm/README.md`。
  测试资产重新生成：`core/testdata/regenerate.sh`。

## 内存预算（iOS 键盘扩展 77MB）

`.dawg` / `.clm` 全部 mmap 只读：clean 页可被系统回收且不计入进程足迹。
预算目标：词库 ≤15MB、LM ≤40MB、运行时堆 <10MB。
