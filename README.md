# SNL 编译器

小型嵌套式语言 SNL (Small Nested Language) 的编译器实现，C++ 编写。

## 功能

- **词法分析** — DFA 驱动，识别 21 个保留字、标识符、常量、运算符和分隔符
- **语法分析** — 递归下降，基于 SNL 的 104 条文法规则构建语法树
- **语义分析** — 符号表管理（嵌套作用域），支持 12 种语义错误检测

## 项目结构

```
src/
├── globals.h          # 全局定义（Token 类型、节点类型、符号表结构）
├── scan.h / scan.cpp  # 词法分析器
├── tree.h / tree.cpp  # 语法树节点定义与打印
├── parse.h / parse.cpp # 语法分析器（递归下降）
├── analyze.h / analyze.cpp # 语义分析器
└── main.cpp           # 主程序入口
test/
├── correct/           # 10 个正确测试用例
└── error/             # 10 个错误测试用例
```

## 编译

需要 MSVC (Visual Studio) 或 g++。

**MSVC：**
```bat
build.bat
```

**g++：**
```bash
g++ -std=c++17 -o snl_compiler src/main.cpp src/scan.cpp src/tree.cpp src/parse.cpp src/analyze.cpp -I src
```

## 使用

```
snl_compiler.exe <文件.snl> [选项]
  -l    仅词法分析（输出 Token 序列）
  -p    仅语法分析（输出语法树）
  -s    语义分析
  -a    全部分析（默认）
```

### 示例

```bash
# 完整分析
./snl_compiler test/correct/t4_while.snl

# 仅输出语法树
./snl_compiler test/correct/t7_proc.snl -p

# 检测语义错误
./snl_compiler test/error/e5_param_count.snl -s
```

## SNL 语言特性

- 数据类型：integer、char、array、record
- 过程支持嵌套定义和递归调用
- 控制流：if-then-else-fi、while-do-endwh
- I/O：read、write

## 语法树示例

```
ProK
  PheadK  t4
  VarK
    DecK  integer  i
    DecK  integer  sum
  StmLk
    StmtK  Assign
      ExpK  sum  IdV
      ExpK  Const  0
    StmtK  While
      ExpK  Op  <
        ExpK  i  IdV
        ExpK  Const  11
      ...
```
