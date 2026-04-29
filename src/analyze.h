#ifndef ANALYZE_H
#define ANALYZE_H

#include "globals.h"
#include "tree.h"

// 初始化语义分析器
void initAnalyzer();

// 执行语义分析
void analyze(TreeNode* tree);

// 获取错误数量
int getSemanticErrorCount();

#endif
