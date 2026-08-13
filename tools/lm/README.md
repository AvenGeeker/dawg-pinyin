# charlm 训练管线（tools/lm）

为 dawg-pinyin core 生成字符级 n-gram 语言模型（`.clm`）。训练/量化脚本为
Python 3，不进 core C++ 运行时；二进制格式与量化常数必须与
`core/src/lm/char_lm_format.h` 保持一致。

## 流程

```bash
# 1. 语料归一：纯文本或 zhwiki XML dump（--wiki），输出每行一个中文句
python3 tools/lm/prepare_corpus.py corpus.txt corpus.norm.txt
# 维基 dump 示例（语料不入库、不随仓库分发）：
# python3 tools/lm/prepare_corpus.py zhwiki-latest-pages-articles.xml.bz2 out.txt --wiki --max-lines 2000000

# 2. 训练：修改版 KN 平滑 + 熵剪枝（order 2..8，默认 6）
python3 tools/lm/train_charlm.py corpus.norm.txt charlm.raw --order 6 --vocab 8190 --min-count 2 --top-k 128

# 3. 质量门禁：留出集困惑度
python3 tools/lm/eval_lm.py charlm.raw heldout.norm.txt --max-ppl 30

# 4. 量化导出 .clm（8-bit 对数概率/backoff，打包 trie）
python3 tools/lm/quantize_lm.py charlm.raw charlm.clm
```

## 说明与限制

- 计数在内存中进行，面向 demo/测试规模语料；全量语料训练请在内存充足的
  机器上运行，或调低 `--order` / 提高 `--min-count` / 降低 `--top-k`。
- 句首以 (order-1) 个 `<s>` 填充，与运行时空上下文语义对齐；`<unk>` 桶
  承接词表外字符。
- 训练数据许可证：`prepare_corpus.py` 下载的语料只在本机使用，不提交进
  仓库；随仓库分发的只有 `core/testdata/` 中的自造小语料与微型模型。
