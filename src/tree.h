#ifndef TREE_H
#define TREE_H

#include "globals.h"

// 创建新节点
TreeNode* newTreeNode(NodeKind kind);

// 打印语法树（层次文本输出）
void printSyntaxTree(TreeNode* tree, int indent = 0);

// 释放语法树内存
void freeTree(TreeNode* tree);

#endif
