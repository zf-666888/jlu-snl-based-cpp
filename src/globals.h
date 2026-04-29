#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>

using namespace std;

// ============================================================
// 单词类别枚举 (LexType) — 按 PPT 定义
// ============================================================
typedef enum {
    // 保留字
    PROGRAM, IF, THEN, ELSE, END, PROCEDURE, TYPE, VAR,
    WHILE, ARRAY, OF, RECORD, INTEGER, CHAR, READ, WRITE,
    RETURN, ENDWH, BEGIN, FI, DO,
    // 标识符和常量
    ID, INTC, CHARC,
    // 运算符
    PLUS, MINUS, TIMES, OVER, LT, EQ, ASSIGN,
    // 分隔符
    LPAREN, RPAREN, SEMI, COMMA, DOT, UNDERANGE, LBRACE, RBRACE,
    // 文件结束
    ENDFILE
} LexType;

// ============================================================
// 语法树节点类型
// ============================================================
typedef enum {
    ProK,       // 程序根节点
    PheadK,     // 程序头
    TypeK,      // 类型声明列表头
    VarK,       // 变量声明列表头
    ProcDecK,   // 过程声明节点
    DecK,       // 声明节点
    StmLk,      // 语句列表头
    StmtK,      // 语句节点
    ExpK        // 表达式节点
} NodeKind;

// 语句子类型
typedef enum {
    IfK, WhileK, AssignK, ReadK, WriteK, CallK, ReturnK
} StmtKind;

// 表达式子类型
typedef enum {
    OpK, ConstK, IdV
} ExpKind;

// 类型种类（用于语义分析）
typedef enum {
    VoidTy, IntegerTy, CharTy, BooleanTy, ArrayTy, RecordTy
} ExpType;

// 标识符种类（用于符号表）
typedef enum {
    TypeIdK, VarIdK, ProcIdK
} IdKind;

// ============================================================
// Token 结构
// ============================================================
struct Token {
    int lineNo;
    LexType lex;
    string sem;
};

// ============================================================
// 语法树节点结构
// ============================================================
struct TreeNode {
    TreeNode* child[4];     // 子节点
    TreeNode* sibling;      // 兄弟节点
    int lineno;             // 源代码行号
    NodeKind nodekind;      // 节点类型

    StmtKind stmtkind;      // StmtK 时有效
    ExpKind expkind;        // ExpK 时有效
    ExpType type;           // 表达式类型（语义分析用）

    LexType op;             // 运算符
    int val;                // 常量值
    string name;            // 标识符名
    string type_name;       // 类型名
    string proc_name;       // 过程名

    // 过程声明时的参数信息
    bool is_var_param;      // 是否为引用参数 (VAR)
    vector<string> id_list; // 声明中的标识符列表

    TreeNode() {
        for (int i = 0; i < 4; i++) child[i] = nullptr;
        sibling = nullptr;
        lineno = 0;
        nodekind = ProK;
        stmtkind = IfK;
        expkind = OpK;
        type = VoidTy;
        op = ENDFILE;
        val = 0;
        is_var_param = false;
    }
};

// ============================================================
// 符号表条目
// ============================================================
struct FieldItem {
    string name;
    string type_name;
};

struct ParamItem {
    string name;
    string type_name;
    bool is_var;
};

struct Symbol {
    string name;
    IdKind kind;
    string type_name;       // 类型名
    int level;              // 嵌套层次

    // 数组信息
    int low, top;
    string elem_type;

    // 记录信息
    vector<FieldItem> fields;

    // 过程参数信息
    vector<ParamItem> params;

    Symbol() : level(0), low(0), top(0) {}
};

// ============================================================
// 辅助函数声明
// string lexTypeToString(LexType lex);
// 由 scan.cpp 实现

#endif
