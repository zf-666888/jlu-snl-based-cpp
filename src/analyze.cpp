#include "analyze.h"

static int semanticErrorCount = 0;
static vector<map<string, Symbol>> scopeStack;
static int currentLevel = 0;

// 辅助：输出语义错误
static void semanticError(int line, const string& msg) {
    cerr << "Line " << line << ": Semantic Error - " << msg << endl;
    semanticErrorCount++;
}

// 辅助：在当前作用域查找符号
static Symbol* lookupCurrent(const string& name) {
    auto& current = scopeStack.back();
    auto it = current.find(name);
    if (it != current.end()) {
        return &it->second;
    }
    return nullptr;
}

// 辅助：在所有作用域查找符号（从内到外）
static Symbol* lookupAll(const string& name) {
    for (int i = scopeStack.size() - 1; i >= 0; i--) {
        auto it = scopeStack[i].find(name);
        if (it != scopeStack[i].end()) {
            return &it->second;
        }
    }
    return nullptr;
}

// 辅助：在当前作用域插入符号
static bool insertSymbol(const Symbol& sym) {
    auto& current = scopeStack.back();
    if (current.find(sym.name) != current.end()) {
        return false; // 重复定义
    }
    current[sym.name] = sym;
    return true;
}

// 辅助：进入新作用域
static void enterScope() {
    scopeStack.push_back(map<string, Symbol>());
    currentLevel++;
}

// 辅助：退出作用域
static void exitScope() {
    scopeStack.pop_back();
    currentLevel--;
}

// 辅助：类型名规范化
static string normalizeType(const string& t) {
    if (t == "integer" || t == "char" || t == "array" || t == "record") {
        return t;
    }
    return t; // 自定义类型名
}

// 辅助：解析自定义类型到基础类型
static string resolveType(const string& typeName) {
    for (int i = scopeStack.size() - 1; i >= 0; i--) {
        auto it = scopeStack[i].find(typeName);
        if (it != scopeStack[i].end() && it->second.kind == TypeIdK) {
            return it->second.type_name;
        }
    }
    return typeName;
}

// 辅助：判断两个类型是否兼容
static bool typeCompatible(const string& t1, const string& t2) {
    string r1 = normalizeType(t1);
    string r2 = normalizeType(t2);
    if (r1 == r2) return true;
    // integer 和 char 可以互操作
    if ((r1 == "integer" && r2 == "char") || (r1 == "char" && r2 == "integer")) {
        return true;
    }
    return false;
}

// 前向声明
static void analyzeNode(TreeNode* tree);

// ============================================================
// 分析声明部分：建立符号表
// ============================================================

// 分析类型声明
static void analyzeTypeDec(TreeNode* t) {
    while (t) {
        if (t->nodekind == DecK) {
            Symbol sym;
            sym.name = t->name;
            sym.kind = TypeIdK;
            sym.type_name = t->type_name;
            sym.level = currentLevel;

            // 如果是记录类型，收集字段信息
            if (t->type_name == "record" && t->child[0]) {
                TreeNode* fieldNode = t->child[0];
                while (fieldNode) {
                    if (fieldNode->nodekind == DecK) {
                        for (auto& fid : fieldNode->id_list) {
                            FieldItem fi;
                            fi.name = fid;
                            fi.type_name = fieldNode->type_name;
                            sym.fields.push_back(fi);
                        }
                    }
                    fieldNode = fieldNode->sibling;
                }
            }

            if (!insertSymbol(sym)) {
                semanticError(t->lineno, "重复定义的类型标识符: " + t->name);
            }
        }
        t = t->sibling;
    }
}

// 分析变量声明
static void analyzeVarDec(TreeNode* t) {
    while (t) {
        if (t->nodekind == DecK) {
            string typeName = t->type_name;

            for (auto& id : t->id_list) {
                Symbol sym;
                sym.name = id;
                sym.kind = VarIdK;
                sym.type_name = typeName;
                sym.level = currentLevel;

                // 如果是数组类型（直接声明或通过自定义类型）
                if (typeName == "array" && t->child[0]) {
                    sym.elem_type = t->child[0]->type_name;
                    sym.low = t->val;
                    sym.top = t->child[1] ? t->child[1]->val : 0;
                } else {
                    // 解析自定义类型名
                    Symbol* typeDef = lookupAll(typeName);
                    if (typeDef && typeDef->kind == TypeIdK) {
                        if (typeDef->type_name == "array") {
                            sym.type_name = "array";
                            sym.elem_type = typeDef->elem_type;
                            sym.low = typeDef->low;
                            sym.top = typeDef->top;
                        }
                    }
                }

                if (!insertSymbol(sym)) {
                    semanticError(t->lineno, "重复定义的变量标识符: " + id);
                }
            }
        }
        t = t->sibling;
    }
}

// 分析过程参数
static void analyzeParamList(TreeNode* t, Symbol& procSym) {
    while (t) {
        if (t->nodekind == DecK) {
            for (auto& id : t->id_list) {
                ParamItem param;
                param.name = id;
                param.type_name = t->type_name;
                param.is_var = t->is_var_param;
                procSym.params.push_back(param);

                // 在过程作用域内注册参数
                Symbol paramSym;
                paramSym.name = id;
                paramSym.kind = VarIdK;
                paramSym.type_name = t->type_name;
                paramSym.level = currentLevel;
                if (!insertSymbol(paramSym)) {
                    semanticError(t->lineno, "重复定义的参数: " + id);
                }
            }
        }
        t = t->sibling;
    }
}

// 分析过程声明
static void analyzeProcDec(TreeNode* t) {
    while (t) {
        if (t->nodekind == ProcDecK) {
            // 注册过程到当前作用域
            Symbol procSym;
            procSym.name = t->name;
            procSym.kind = ProcIdK;
            procSym.type_name = "void";
            procSym.level = currentLevel - 1; // 在父作用域中

            // 先在父作用域注册
            int parentIdx = (int)scopeStack.size() - 2;
            if (parentIdx < 0) parentIdx = 0;
            auto& parentScope = scopeStack[parentIdx];
            if (parentScope.find(t->name) != parentScope.end()) {
                semanticError(t->lineno, "重复定义的过程标识符: " + t->name);
            } else {
                // 临时存储参数信息
                TreeNode* paramNode = t->child[0];
                while (paramNode) {
                    if (paramNode->nodekind == DecK) {
                        for (auto& id : paramNode->id_list) {
                            ParamItem p;
                            p.name = id;
                            p.type_name = paramNode->type_name;
                            p.is_var = paramNode->is_var_param;
                            procSym.params.push_back(p);
                        }
                    }
                    paramNode = paramNode->sibling;
                }
                parentScope[t->name] = procSym;
            }

            // 进入过程作用域
            enterScope();

            // 注册参数到过程作用域
            if (t->child[0]) {
                TreeNode* pNode = t->child[0];
                while (pNode) {
                    if (pNode->nodekind == DecK) {
                        for (auto& id : pNode->id_list) {
                            Symbol paramSym;
                            paramSym.name = id;
                            paramSym.kind = VarIdK;
                            paramSym.type_name = pNode->type_name;
                            paramSym.level = currentLevel;
                            if (!insertSymbol(paramSym)) {
                                semanticError(pNode->lineno, "重复定义的参数: " + id);
                            }
                        }
                    }
                    pNode = pNode->sibling;
                }
            }

            // 分析过程内部声明
            if (t->child[1]) {
                analyzeNode(t->child[1]);
            }

            // 分析过程体
            if (t->child[2]) {
                analyzeNode(t->child[2]);
            }

            exitScope();
        }
        t = t->sibling;
    }
}

// ============================================================
// 分析表达式
// ============================================================
static string analyzeExp(TreeNode* t) {
    if (!t) return "void";

    if (t->nodekind != ExpK) {
        analyzeNode(t);
        return "void";
    }

    switch (t->expkind) {
        case ConstK:
            return "integer";

        case IdV: {
            // 处理记录域访问 (r.x)
            string lookupName = t->name;
            string fieldName;
            size_t dotPos = t->name.find('.');
            if (dotPos != string::npos) {
                lookupName = t->name.substr(0, dotPos);
                fieldName = t->name.substr(dotPos + 1);
            }

            Symbol* sym = lookupAll(lookupName);
            if (!sym) {
                semanticError(t->lineno, "无声明的标识符: " + lookupName);
                return "void";
            }
            if (sym->kind == ProcIdK) {
                semanticError(t->lineno, "标识符类别不匹配: " + lookupName + " 是过程名，不能作为变量使用");
                return "void";
            }
            if (sym->kind == TypeIdK) {
                semanticError(t->lineno, "标识符类别不匹配: " + t->name + " 是类型名，不能作为变量使用");
                return "void";
            }
            // 如果是数组下标访问
            if (t->child[0]) {
                string indexType = analyzeExp(t->child[0]);
                if (indexType != "integer" && indexType != "void") {
                    semanticError(t->lineno, "数组下标必须是整型");
                }
                // 解析类型名：sym->type_name 可能是 "array" 或自定义类型名
                string arrType = sym->type_name;
                if (arrType != "array") {
                    Symbol* typeDef = lookupAll(arrType);
                    if (typeDef && typeDef->kind == TypeIdK) {
                        arrType = typeDef->type_name;
                    }
                }
                if (arrType != "array") {
                    semanticError(t->lineno, "标识符 " + lookupName + " 不是数组类型");
                    return "void";
                }
                return sym->elem_type.empty() ? "integer" : sym->elem_type;
            }
            // 检查是否是域名访问（如 a.field）
            if (!fieldName.empty()) {
                // 解析类型名：sym->type_name 可能是 "record" 或自定义类型名
                string resolvedType = sym->type_name;
                if (resolvedType != "record" && resolvedType != "array") {
                    // 查找类型定义
                    Symbol* typeDef = lookupAll(resolvedType);
                    if (typeDef && typeDef->kind == TypeIdK) {
                        resolvedType = typeDef->type_name;
                    }
                }
                if (resolvedType != "record") {
                    semanticError(t->lineno, "标识符 " + lookupName + " 不是记录类型");
                    return "void";
                }
                return "integer";
            }
            return sym->type_name;
        }

        case OpK: {
            string leftType = analyzeExp(t->child[0]);
            string rightType = analyzeExp(t->child[1]);

            // 比较运算符返回 boolean
            if (t->op == LT || t->op == EQ) {
                if (leftType != "void" && rightType != "void" && !typeCompatible(leftType, rightType)) {
                    semanticError(t->lineno, "比较运算符两侧类型不兼容");
                }
                return "boolean";
            }

            // 算术运算符
            if (leftType != "void" && rightType != "void") {
                if (leftType != "integer" || rightType != "integer") {
                    semanticError(t->lineno, "算术运算符的操作数必须是整型");
                }
            }
            return "integer";
        }
    }
    return "void";
}

// ============================================================
// 分析语句
// ============================================================
static void analyzeStmt(TreeNode* t) {
    if (!t) return;

    switch (t->stmtkind) {
        case AssignK: {
            // 左值检查
            TreeNode* lhs = t->child[0];
            TreeNode* rhs = t->child[1];

            if (lhs && lhs->nodekind == ExpK && lhs->expkind == IdV) {
                // 处理记录域访问 (r.x) - 只查找基础变量名
                string baseName = lhs->name;
                size_t dotPos = baseName.find('.');
                if (dotPos != string::npos) {
                    baseName = baseName.substr(0, dotPos);
                }
                Symbol* sym = lookupAll(baseName);
                if (!sym) {
                    semanticError(lhs->lineno, "无声明的标识符: " + baseName);
                } else if (sym->kind != VarIdK) {
                    semanticError(lhs->lineno, "赋值语句左端不是变量标识符: " + baseName);
                }
            }

            string leftType = analyzeExp(lhs);
            string rightType = analyzeExp(rhs);

            if (leftType != "void" && rightType != "void" && !typeCompatible(leftType, rightType)) {
                semanticError(t->lineno, "赋值语句左右两边类型不相容");
            }
            break;
        }

        case IfK: {
            string condType = analyzeExp(t->child[0]);
            // if 条件应为布尔类型（比较表达式返回 boolean）
            // 如果是简单整型变量，也允许（简化处理）

            if (t->child[1]) analyzeNode(t->child[1]);
            if (t->child[2]) analyzeNode(t->child[2]);
            break;
        }

        case WhileK: {
            string condType = analyzeExp(t->child[0]);
            if (t->child[1]) analyzeNode(t->child[1]);
            break;
        }

        case ReadK: {
            Symbol* sym = lookupAll(t->name);
            if (!sym) {
                semanticError(t->lineno, "无声明的标识符: " + t->name);
            } else if (sym->kind != VarIdK) {
                semanticError(t->lineno, "READ 参数必须是变量: " + t->name);
            }
            break;
        }

        case WriteK: {
            analyzeExp(t->child[0]);
            break;
        }

        case ReturnK: {
            if (t->child[0]) analyzeExp(t->child[0]);
            break;
        }

        case CallK: {
            Symbol* sym = lookupAll(t->name);
            if (!sym) {
                semanticError(t->lineno, "无声明的标识符: " + t->name);
                break;
            }
            if (sym->kind != ProcIdK) {
                semanticError(t->lineno, "过程调用语句中标识符不是过程标识符: " + t->name);
                break;
            }

            // 检查参数数量
            int actualCount = 0;
            TreeNode* arg = t->child[0];
            while (arg) {
                actualCount++;
                arg = arg->sibling;
            }

            int expectedCount = (int)sym->params.size();
            if (actualCount != expectedCount) {
                semanticError(t->lineno, "过程调用形实参个数不相同: 期望 " +
                    to_string(expectedCount) + "，实际 " + to_string(actualCount));
            }

            // 检查参数类型
            arg = t->child[0];
            for (int i = 0; i < (int)sym->params.size() && arg; i++) {
                string argType = analyzeExp(arg);
                if (argType != "void" && !typeCompatible(argType, sym->params[i].type_name)) {
                    semanticError(t->lineno, "过程调用形实参类型不匹配: 第 " +
                        to_string(i + 1) + " 个参数");
                }
                arg = arg->sibling;
            }
            break;
        }
    }
}

// ============================================================
// 递归分析语法树
// ============================================================
static void analyzeNode(TreeNode* tree) {
    if (!tree) return;

    TreeNode* p = tree;

    while (p) {
        switch (p->nodekind) {
            case ProK:
                // 根节点：进入全局作用域
                enterScope();
                for (int i = 0; i < 4; i++) {
                    if (p->child[i]) analyzeNode(p->child[i]);
                }
                exitScope();
                break;

            case TypeK:
                analyzeTypeDec(p->child[0]);
                break;

            case VarK:
                analyzeVarDec(p->child[0]);
                break;

            case ProcDecK:
                analyzeProcDec(p);
                break;

            case StmLk:
                if (p->child[0]) analyzeNode(p->child[0]);
                break;

            case StmtK:
                analyzeStmt(p);
                break;

            case ExpK:
                analyzeExp(p);
                break;

            default:
                break;
        }

        p = p->sibling;
    }
}

// ============================================================
// 公共接口
// ============================================================
void initAnalyzer() {
    scopeStack.clear();
    currentLevel = 0;
    semanticErrorCount = 0;
}

void analyze(TreeNode* tree) {
    analyzeNode(tree);
}

int getSemanticErrorCount() {
    return semanticErrorCount;
}
