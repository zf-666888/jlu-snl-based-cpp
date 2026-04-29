#include "scan.h"

static ifstream sourceFile;
static int lineNo = 1;
static char currentChar;
static bool fileEnded = false;

// 保留字表
static ReservedWord reservedWords[] = {
    {"program", PROGRAM}, {"if", IF}, {"then", THEN}, {"else", ELSE},
    {"end", END}, {"procedure", PROCEDURE}, {"type", TYPE}, {"var", VAR},
    {"while", WHILE}, {"array", ARRAY}, {"of", OF}, {"record", RECORD},
    {"integer", INTEGER}, {"char", CHAR}, {"read", READ}, {"write", WRITE},
    {"return", RETURN}, {"endwh", ENDWH}, {"begin", BEGIN}, {"fi", FI},
    {"do", DO}
};
static const int NUM_RESERVED = sizeof(reservedWords) / sizeof(reservedWords[0]);

string lexTypeToString(LexType lex) {
    switch (lex) {
        case PROGRAM: return "PROGRAM";
        case IF: return "IF";
        case THEN: return "THEN";
        case ELSE: return "ELSE";
        case END: return "END";
        case PROCEDURE: return "PROCEDURE";
        case TYPE: return "TYPE";
        case VAR: return "VAR";
        case WHILE: return "WHILE";
        case ARRAY: return "ARRAY";
        case OF: return "OF";
        case RECORD: return "RECORD";
        case INTEGER: return "INTEGER";
        case CHAR: return "CHAR";
        case READ: return "READ";
        case WRITE: return "WRITE";
        case RETURN: return "RETURN";
        case ENDWH: return "ENDWH";
        case BEGIN: return "BEGIN";
        case FI: return "FI";
        case DO: return "DO";
        case ID: return "ID";
        case INTC: return "INTC";
        case CHARC: return "CHARC";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case TIMES: return "TIMES";
        case OVER: return "OVER";
        case LT: return "LT";
        case EQ: return "EQ";
        case ASSIGN: return "ASSIGN";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case SEMI: return "SEMI";
        case COMMA: return "COMMA";
        case DOT: return "DOT";
        case UNDERANGE: return "UNDERANGE";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case ENDFILE: return "ENDFILE";
        default: return "UNKNOWN";
    }
}

static char getNextChar() {
    if (sourceFile.get(currentChar)) {
        return currentChar;
    } else {
        fileEnded = true;
        return EOF;
    }
}

static void ungetChar() {
    sourceFile.unget();
}

void initScanner(const string& filename) {
    if (sourceFile.is_open()) {
        sourceFile.close();
    }
    sourceFile.clear();
    sourceFile.open(filename);
    if (!sourceFile.is_open()) {
        cerr << "Error: Cannot open file " << filename << endl;
        exit(1);
    }
    lineNo = 1;
    fileEnded = false;
    getNextChar();
}

bool hasMoreTokens() {
    return !fileEnded;
}

// 查找保留字
static LexType reservedLookup(const string& s) {
    for (int i = 0; i < NUM_RESERVED; i++) {
        if (s == reservedWords[i].str) {
            return reservedWords[i].tok;
        }
    }
    return ID;
}

Token getToken() {
    Token token;
    token.lineNo = lineNo;
    token.lex = ENDFILE;
    token.sem = "";

    // 跳过空白符
    while (!fileEnded && (currentChar == ' ' || currentChar == '\t' || currentChar == '\r')) {
        getNextChar();
    }

    if (fileEnded) {
        token.lex = ENDFILE;
        token.sem = "EOF";
        return token;
    }

    // 处理换行
    if (currentChar == '\n') {
        lineNo++;
        getNextChar();
        return getToken();
    }

    token.lineNo = lineNo;

    // 注释 { ... }
    if (currentChar == '{') {
        getNextChar();
        while (!fileEnded && currentChar != '}') {
            if (currentChar == '\n') lineNo++;
            getNextChar();
        }
        if (!fileEnded) getNextChar(); // 跳过 }
        return getToken();
    }

    // 标识符或保留字: 以字母开头，后跟字母或数字
    if (isalpha(currentChar)) {
        string lexbuf;
        lexbuf += currentChar;
        getNextChar();
        while (!fileEnded && (isalnum(currentChar) || currentChar == '_')) {
            lexbuf += currentChar;
            getNextChar();
        }
        // 转小写用于保留字匹配
        string lower = lexbuf;
        for (auto& c : lower) c = tolower(c);
        token.lex = reservedLookup(lower);
        token.sem = lexbuf;
        return token;
    }

    // 整数常量
    if (isdigit(currentChar)) {
        string lexbuf;
        lexbuf += currentChar;
        getNextChar();
        while (!fileEnded && isdigit(currentChar)) {
            lexbuf += currentChar;
            getNextChar();
        }
        token.lex = INTC;
        token.sem = lexbuf;
        return token;
    }

    // 字符常量 'x'
    if (currentChar == '\'') {
        getNextChar(); // 跳过左引号
        if (!fileEnded && currentChar != '\'') {
            token.sem = string(1, currentChar);
            getNextChar();
        }
        if (!fileEnded && currentChar == '\'') {
            getNextChar(); // 跳过右引号
        }
        token.lex = CHARC;
        return token;
    }

    // 运算符和分隔符
    char ch = currentChar;
    switch (ch) {
        case '+':
            token.lex = PLUS; token.sem = "+";
            getNextChar(); break;
        case '-':
            token.lex = MINUS; token.sem = "-";
            getNextChar(); break;
        case '*':
            token.lex = TIMES; token.sem = "*";
            getNextChar(); break;
        case '/':
            token.lex = OVER; token.sem = "/";
            getNextChar(); break;
        case '(':
            token.lex = LPAREN; token.sem = "(";
            getNextChar(); break;
        case ')':
            token.lex = RPAREN; token.sem = ")";
            getNextChar(); break;
        case '[':
            token.lex = LBRACE; token.sem = "[";
            getNextChar(); break;
        case ']':
            token.lex = RBRACE; token.sem = "]";
            getNextChar(); break;
        case ';':
            token.lex = SEMI; token.sem = ";";
            getNextChar(); break;
        case ',':
            token.lex = COMMA; token.sem = ",";
            getNextChar(); break;
        case '=':
            token.lex = EQ; token.sem = "=";
            getNextChar(); break;
        case '<':
            getNextChar();
            if (!fileEnded && currentChar == '=') {
                // <= 不在 SNL 标准运算符中，但可以处理
                token.lex = LT; token.sem = "<=";
                getNextChar();
            } else {
                token.lex = LT; token.sem = "<";
            }
            break;
        case ':':
            getNextChar();
            if (!fileEnded && currentChar == '=') {
                token.lex = ASSIGN; token.sem = ":=";
                getNextChar();
            } else {
                // 单独的冒号不是合法 token
                cerr << "Line " << lineNo << ": Error - unexpected character ':'" << endl;
                token.lex = ENDFILE; token.sem = "";
            }
            break;
        case '.':
            getNextChar();
            if (!fileEnded && currentChar == '.') {
                token.lex = UNDERANGE; token.sem = "..";
                getNextChar();
            } else {
                token.lex = DOT; token.sem = ".";
            }
            break;
        default:
            cerr << "Line " << lineNo << ": Error - unexpected character '" << ch << "'" << endl;
            getNextChar();
            return getToken(); // 跳过非法字符
    }

    return token;
}
