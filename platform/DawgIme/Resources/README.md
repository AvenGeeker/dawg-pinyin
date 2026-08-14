此目录由构建脚本（Xcode "Sync IME Assets" run-script phase）生成：

- 存在仓库根 `assets/` 生产词库时复制 `dawg_pinyin.dawg` / `dawg_pinyin.clm`；
- 否则回退复制 `core/testdata/` 的 tiny 资产（同名）。

目录内容不入库（见根 .gitignore 例外）。
