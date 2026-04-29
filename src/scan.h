#ifndef SCAN_H
#define SCAN_H

#include "globals.h"

// 保留字表
struct ReservedWord {
    const char* str;
    LexType tok;
};

// 初始化词法分析器
void initScanner(const string& filename);

// 获取下一个 Token
Token getToken();

// 判断是否还有 Token
bool hasMoreTokens();

// LexType 转字符串
string lexTypeToString(LexType lex);

#endif
