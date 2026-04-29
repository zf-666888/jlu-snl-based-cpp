#ifndef PARSE_H
#define PARSE_H

#include "globals.h"
#include "scan.h"
#include "tree.h"

// 初始化语法分析器
void initParser(const string& filename);

// 执行语法分析，返回语法树根节点
TreeNode* parse();

// 获取词法分析器的下一个Token（供语义分析使用）
Token getCurrentToken();

#endif
