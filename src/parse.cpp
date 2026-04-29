#include "parse.h"

static Token currentToken;
static bool errorOccurred = false;

// 前向声明
static TreeNode* program();
static TreeNode* programHead();
static TreeNode* declarePart();
static TreeNode* typeDec();
static TreeNode* typeDeclaration();
static TreeNode* typeDecList();
static TreeNode* typeName();
static TreeNode* varDec();
static TreeNode* varDeclaration();
static TreeNode* varDecList();
static TreeNode* procDec();
static TreeNode* procDeclaration();
static TreeNode* paramList();
static TreeNode* paramDecList();
static TreeNode* programBody();
static TreeNode* stmList();
static TreeNode* statement();
static TreeNode* conditionalStm();
static TreeNode* loopStm();
static TreeNode* inputStm();
static TreeNode* outputStm();
static TreeNode* returnStm();
static TreeNode* callStmRest(TreeNode* idNode);
static TreeNode* assignmentRest(TreeNode* idNode);
static TreeNode* relExp();
static TreeNode* exp();
static TreeNode* term();
static TreeNode* factor();
static TreeNode* variable();
static TreeNode* fieldVar();
static void match(LexType expected);

// ============================================================
// 错误处理
// ============================================================
static void syntaxError(const string& message) {
    cerr << "Line " << currentToken.lineNo << ": Syntax Error - " << message << endl;
    cerr << "  Got: " << lexTypeToString(currentToken.lex)
         << " ('" << currentToken.sem << "')" << endl;
    errorOccurred = true;
}

// 同步：跳过 token 直到遇到目标类型
static void skipUntil(LexType target) {
    while (currentToken.lex != ENDFILE && currentToken.lex != target) {
        currentToken = getToken();
    }
    if (currentToken.lex == target) {
        currentToken = getToken();
    }
}

// 匹配并前进
static void match(LexType expected) {
    if (currentToken.lex == expected) {
        currentToken = getToken();
    } else {
        syntaxError("Expected '" + lexTypeToString(expected) + "'");
    }
}

// 辅助：判断当前 token 是否为类型起始符号
static bool isTypeStart() {
    return currentToken.lex == INTEGER || currentToken.lex == CHAR ||
           currentToken.lex == ARRAY || currentToken.lex == RECORD ||
           currentToken.lex == ID;
}

// 辅助：创建标识符表达式节点
static TreeNode* makeIdExpNode(const string& name, int line) {
    TreeNode* t = newTreeNode(ExpK);
    t->expkind = IdV;
    t->name = name;
    t->lineno = line;
    return t;
}

// 辅助：创建常量表达式节点
static TreeNode* makeConstExpNode(int val, int line) {
    TreeNode* t = newTreeNode(ExpK);
    t->expkind = ConstK;
    t->val = val;
    t->lineno = line;
    return t;
}

// ============================================================
// 公共接口
// ============================================================
void initParser(const string& filename) {
    initScanner(filename);
    errorOccurred = false;
    currentToken = getToken();
}

Token getCurrentToken() {
    return currentToken;
}

TreeNode* parse() {
    TreeNode* root = program();
    if (currentToken.lex != ENDFILE) {
        syntaxError("Expected end of file");
    }
    return root;
}

// ============================================================
// 规则 1: Program ::= ProgramHead DeclarePart ProgramBody .
// ============================================================
static TreeNode* program() {
    TreeNode* root = newTreeNode(ProK);
    root->lineno = currentToken.lineNo;

    TreeNode* head = programHead();
    TreeNode* decl = declarePart();

    // 跳过声明和程序体之间可能存在的多余分号
    while (currentToken.lex == SEMI) {
        match(SEMI);
    }

    TreeNode* body = programBody();

    root->child[0] = head;
    root->child[1] = decl;
    root->child[2] = body;

    // 匹配末尾的 '.'
    if (currentToken.lex == DOT) {
        match(DOT);
    } else {
        syntaxError("Expected '.' at end of program");
    }

    return root;
}

// 规则 2-3: ProgramHead ::= PROGRAM ProgramName
//           ProgramName ::= ID
static TreeNode* programHead() {
    TreeNode* t = newTreeNode(PheadK);
    t->lineno = currentToken.lineNo;

    match(PROGRAM);
    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    } else {
        syntaxError("Expected program name");
    }

    return t;
}

// 规则 4: DeclarePart ::= TypeDec VarDec ProcDec
static TreeNode* declarePart() {
    TreeNode* typePart = typeDec();
    TreeNode* varPart = varDec();
    TreeNode* procPart = procDec();

    // 将三部分通过兄弟节点串联
    TreeNode* result = typePart;
    if (typePart) {
        // 找到 typePart 的最后一个兄弟
        TreeNode* p = typePart;
        while (p->sibling) p = p->sibling;
        p->sibling = varPart;
        if (varPart) {
            TreeNode* q = varPart;
            while (q->sibling) q = q->sibling;
            q->sibling = procPart;
        } else {
            typePart->sibling = procPart;
        }
    } else if (varPart) {
        result = varPart;
        TreeNode* q = varPart;
        while (q->sibling) q = q->sibling;
        q->sibling = procPart;
    } else {
        result = procPart;
    }

    return result;
}

// 规则 6: TypeDec ::= ε | TypeDeclaration
static TreeNode* typeDec() {
    if (currentToken.lex == TYPE) {
        return typeDeclaration();
    }
    return nullptr; // ε
}

// 规则 7: TypeDeclaration ::= TYPE TypeDecList
static TreeNode* typeDeclaration() {
    TreeNode* t = newTreeNode(TypeK);
    t->lineno = currentToken.lineNo;
    match(TYPE);
    t->child[0] = typeDecList();
    return t;
}

// 规则 8-10: TypeDecList ::= TypeId = TypeName ; TypeDecMore
//            TypeDecMore ::= ε | TypeDecList
static TreeNode* typeDecList() {
    TreeNode* t = newTreeNode(DecK);
    t->lineno = currentToken.lineNo;

    // TypeId ::= ID
    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    } else {
        syntaxError("Expected type name identifier");
    }

    // = TypeName ;
    if (currentToken.lex == EQ) {
        match(EQ);
    } else {
        syntaxError("Expected '='");
    }

    TreeNode* typeNode = typeName();
    if (typeNode) {
        t->type_name = typeNode->type_name;
        t->child[0] = typeNode;
    }

    if (currentToken.lex == SEMI) {
        match(SEMI);
    } else {
        syntaxError("Expected ';'");
    }

    // TypeDecMore
    if (currentToken.lex == ID) {
        t->sibling = typeDecList();
    }

    return t;
}

// 规则 12-18: TypeName ::= BaseType | StructureType | ID
static TreeNode* typeName() {
    TreeNode* t = newTreeNode(DecK);
    t->lineno = currentToken.lineNo;

    switch (currentToken.lex) {
        case INTEGER:
            t->type_name = "integer";
            match(INTEGER);
            break;
        case CHAR:
            t->type_name = "char";
            match(CHAR);
            break;
        case ARRAY: {
            match(ARRAY);
            match(LBRACE); // [
            // Low ::= INTC
            int low = 0, top = 0;
            if (currentToken.lex == INTC) {
                low = stoi(currentToken.sem);
                match(INTC);
            } else {
                syntaxError("Expected integer for array lower bound");
            }
            match(UNDERANGE); // ..
            // Top ::= INTC
            if (currentToken.lex == INTC) {
                top = stoi(currentToken.sem);
                match(INTC);
            } else {
                syntaxError("Expected integer for array upper bound");
            }
            match(RBRACE); // ]
            match(OF);
            // BaseType
            if (currentToken.lex == INTEGER) {
                t->type_name = "array";
                t->child[0] = newTreeNode(DecK);
                t->child[0]->type_name = "integer";
                t->child[0]->lineno = currentToken.lineNo;
                match(INTEGER);
            } else if (currentToken.lex == CHAR) {
                t->type_name = "array";
                t->child[0] = newTreeNode(DecK);
                t->child[0]->type_name = "char";
                t->child[0]->lineno = currentToken.lineNo;
                match(CHAR);
            } else {
                syntaxError("Expected base type after OF");
            }
            // 存储数组上下界到节点
            t->val = low;
            // 用 child[1] 存储上界信息
            t->child[1] = newTreeNode(DecK);
            t->child[1]->val = top;
            break;
        }
        case RECORD: {
            match(RECORD);
            t->type_name = "record";
            // FieldDecList
            TreeNode* firstField = nullptr;
            TreeNode* lastField = nullptr;
            // FieldDecList ::= (BaseType | ArrayType) IdList ; FieldDecMore
            while (currentToken.lex == INTEGER || currentToken.lex == CHAR || currentToken.lex == ARRAY) {
                TreeNode* field = newTreeNode(DecK);
                field->lineno = currentToken.lineNo;

                if (currentToken.lex == ARRAY) {
                    match(ARRAY);
                    match(LBRACE);
                    if (currentToken.lex == INTC) match(INTC);
                    match(UNDERANGE);
                    if (currentToken.lex == INTC) match(INTC);
                    match(RBRACE);
                    match(OF);
                    if (currentToken.lex == INTEGER) {
                        field->type_name = "integer";
                        match(INTEGER);
                    } else if (currentToken.lex == CHAR) {
                        field->type_name = "char";
                        match(CHAR);
                    }
                } else if (currentToken.lex == INTEGER) {
                    field->type_name = "integer";
                    match(INTEGER);
                } else {
                    field->type_name = "char";
                    match(CHAR);
                }

                // IdList ::= ID IdMore
                if (currentToken.lex == ID) {
                    field->id_list.push_back(currentToken.sem);
                    match(ID);
                    while (currentToken.lex == COMMA) {
                        match(COMMA);
                        if (currentToken.lex == ID) {
                            field->id_list.push_back(currentToken.sem);
                            match(ID);
                        }
                    }
                }

                if (currentToken.lex == SEMI) {
                    match(SEMI);
                }

                if (!firstField) {
                    firstField = field;
                    lastField = field;
                } else {
                    lastField->sibling = field;
                    lastField = field;
                }

                // FieldDecMore ::= ε | FieldDecList
                // 继续循环检查是否还有字段
            }
            t->child[0] = firstField;
            match(END);
            break;
        }
        case ID:
            t->type_name = currentToken.sem;
            match(ID);
            break;
        default:
            syntaxError("Expected type name");
            break;
    }
    return t;
}

// 规则 30-31: VarDec ::= ε | VarDeclaration
static TreeNode* varDec() {
    if (currentToken.lex == VAR) {
        return varDeclaration();
    }
    return nullptr;
}

// 规则 32: VarDeclaration ::= VAR VarDecList
static TreeNode* varDeclaration() {
    TreeNode* t = newTreeNode(VarK);
    t->lineno = currentToken.lineNo;
    match(VAR);
    t->child[0] = varDecList();
    return t;
}

// 规则 33-35: VarDecList ::= TypeName VarIdList ; VarDecMore
static TreeNode* varDecList() {
    TreeNode* t = newTreeNode(DecK);
    t->lineno = currentToken.lineNo;

    // TypeName
    if (isTypeStart()) {
        TreeNode* typeNode = typeName();
        if (typeNode) {
            t->type_name = typeNode->type_name;
            t->child[0] = typeNode->child[0];
            t->val = typeNode->val;
            // 复制数组信息
            if (typeNode->child[1]) {
                t->child[1] = typeNode->child[1];
            }
            delete typeNode;
        }
    } else {
        syntaxError("Expected type in variable declaration");
    }

    // VarIdList ::= ID VarIdMore
    if (currentToken.lex == ID) {
        t->id_list.push_back(currentToken.sem);
        match(ID);
        while (currentToken.lex == COMMA) {
            match(COMMA);
            if (currentToken.lex == ID) {
                t->id_list.push_back(currentToken.sem);
                match(ID);
            }
        }
    } else {
        syntaxError("Expected identifier in variable declaration");
    }

    if (currentToken.lex == SEMI) {
        match(SEMI);
    } else {
        syntaxError("Expected ';'");
    }

    // VarDecMore
    if (isTypeStart()) {
        t->sibling = varDecList();
    }

    return t;
}

// 规则 39-40: ProcDec ::= ε | ProcDeclaration
static TreeNode* procDec() {
    if (currentToken.lex == PROCEDURE) {
        return procDeclaration();
    }
    return nullptr;
}

// 规则 41-43: ProcDeclaration ::= PROCEDURE ProcName ( ParamList ) ; ProcDecPart ProcBody ProcDecMore
static TreeNode* procDeclaration() {
    TreeNode* t = newTreeNode(ProcDecK);
    t->lineno = currentToken.lineNo;

    match(PROCEDURE);

    // ProcName ::= ID
    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    } else {
        syntaxError("Expected procedure name");
    }

    match(LPAREN);

    // ParamList
    t->child[0] = paramList(); // 参数列表作为 child[0]

    match(RPAREN);
    match(SEMI);

    // ProcDecPart ::= DeclarePart (递归：声明部分)
    t->child[1] = declarePart();

    // 跳过声明和过程体之间可能存在的多余分号
    while (currentToken.lex == SEMI) {
        match(SEMI);
    }

    // ProcBody ::= ProgramBody
    t->child[2] = programBody();

    // ProcDecMore
    if (currentToken.lex == PROCEDURE) {
        t->sibling = procDeclaration();
    }

    return t;
}

// 规则 45-46: ParamList ::= ε | ParamDecList
static TreeNode* paramList() {
    if (currentToken.lex == RPAREN) {
        return nullptr; // ε
    }
    return paramDecList();
}

// 规则 47-49: ParamDecList ::= Param ParamMore
//             ParamMore ::= ε | ; ParamDecList
static TreeNode* paramDecList() {
    TreeNode* t = newTreeNode(DecK);
    t->lineno = currentToken.lineNo;

    // Param ::= TypeName FormList | VAR TypeName FormList
    if (currentToken.lex == VAR) {
        t->is_var_param = true;
        match(VAR);
    }

    // TypeName
    if (isTypeStart()) {
        TreeNode* typeNode = typeName();
        if (typeNode) {
            t->type_name = typeNode->type_name;
            delete typeNode;
        }
    }

    // FormList ::= ID FidMore
    if (currentToken.lex == ID) {
        t->id_list.push_back(currentToken.sem);
        match(ID);
        while (currentToken.lex == COMMA) {
            match(COMMA);
            if (currentToken.lex == ID) {
                t->id_list.push_back(currentToken.sem);
                match(ID);
            }
        }
    }

    // ParamMore
    if (currentToken.lex == SEMI) {
        match(SEMI);
        t->sibling = paramDecList();
    }

    return t;
}

// 规则 57: ProgramBody ::= BEGIN StmList END
static TreeNode* programBody() {
    TreeNode* t = newTreeNode(StmLk);
    t->lineno = currentToken.lineNo;

    match(BEGIN);
    t->child[0] = stmList();
    match(END);

    return t;
}

// 规则 58-60: StmList ::= Stm StmMore
//             StmMore ::= ε | ; StmList
static TreeNode* stmList() {
    TreeNode* t = statement();

    // StmMore
    if (currentToken.lex == SEMI) {
        match(SEMI);
        TreeNode* more = stmList();
        // 将 more 串联为兄弟
        if (t) {
            TreeNode* p = t;
            while (p->sibling) p = p->sibling;
            p->sibling = more;
        }
    }

    return t;
}

// 规则 61-66: Stm ::= ConditionalStm | LoopStm | InputStm | OutputStm | ReturnStm | ID AssCall
static TreeNode* statement() {
    TreeNode* t = nullptr;

    switch (currentToken.lex) {
        case IF:
            t = conditionalStm();
            break;
        case WHILE:
            t = loopStm();
            break;
        case READ:
            t = inputStm();
            break;
        case WRITE:
            t = outputStm();
            break;
        case RETURN:
            t = returnStm();
            break;
        case ID: {
            // ID AssCall
            TreeNode* idNode = makeIdExpNode(currentToken.sem, currentToken.lineNo);
            match(ID);

            if (currentToken.lex == ASSIGN || currentToken.lex == LBRACE || currentToken.lex == DOT) {
                // AssignmentRest
                t = assignmentRest(idNode);
            } else if (currentToken.lex == LPAREN) {
                // CallStmRest
                t = callStmRest(idNode);
            } else {
                syntaxError("Expected ':=' or '(' after identifier");
            }
            break;
        }
        default:
            syntaxError("Expected statement");
            break;
    }

    return t;
}

// 规则 69: AssignmentRest ::= VariMore := Exp
static TreeNode* assignmentRest(TreeNode* idNode) {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = AssignK;
    t->lineno = idNode->lineno;

    // VariMore ::= ε | [ Exp ] | . FieldVar
    if (currentToken.lex == LBRACE) {
        // 数组下标访问
        match(LBRACE);
        TreeNode* indexExp = exp();
        match(RBRACE);
        // 创建数组访问节点
        TreeNode* arrayNode = newTreeNode(ExpK);
        arrayNode->expkind = IdV;
        arrayNode->name = idNode->name;
        arrayNode->lineno = idNode->lineno;
        arrayNode->child[0] = indexExp;
        t->child[0] = arrayNode;
    } else if (currentToken.lex == DOT) {
        // 记录域访问
        match(DOT);
        TreeNode* fv = fieldVar();
        TreeNode* recNode = newTreeNode(ExpK);
        recNode->expkind = IdV;
        recNode->name = idNode->name + "." + (fv ? fv->name : "");
        recNode->lineno = idNode->lineno;
        t->child[0] = recNode;
        delete idNode;
        delete fv;
    } else {
        t->child[0] = idNode;
    }

    match(ASSIGN);
    t->child[1] = exp();

    return t;
}

// 规则 67-68: AssCall ::= AssignmentRest | CallStmRest
// 已在 statement() 中通过 lookahead 处理

// 规则 70: ConditionalStm ::= IF RelExp THEN StmList ELSE StmList FI
static TreeNode* conditionalStm() {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = IfK;
    t->lineno = currentToken.lineNo;

    match(IF);
    t->child[0] = relExp();    // 条件
    match(THEN);
    t->child[1] = stmList();   // THEN 分支
    match(ELSE);
    t->child[2] = stmList();   // ELSE 分支
    match(FI);

    return t;
}

// 规则 71: LoopStm ::= WHILE RelExp DO StmList ENDWH
static TreeNode* loopStm() {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = WhileK;
    t->lineno = currentToken.lineNo;

    match(WHILE);
    t->child[0] = relExp();
    match(DO);
    t->child[1] = stmList();
    match(ENDWH);

    return t;
}

// 规则 72-73: InputStm ::= READ ( Invar )
//             Invar ::= ID
static TreeNode* inputStm() {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = ReadK;
    t->lineno = currentToken.lineNo;

    match(READ);
    match(LPAREN);
    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    } else {
        syntaxError("Expected identifier in READ");
    }
    match(RPAREN);

    return t;
}

// 规则 74: OutputStm ::= WRITE ( Exp )
static TreeNode* outputStm() {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = WriteK;
    t->lineno = currentToken.lineNo;

    match(WRITE);
    match(LPAREN);
    t->child[0] = exp();
    match(RPAREN);

    return t;
}

// 规则 75: ReturnStm ::= RETURN ( Exp )
static TreeNode* returnStm() {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = ReturnK;
    t->lineno = currentToken.lineNo;

    match(RETURN);
    match(LPAREN);
    t->child[0] = exp();
    match(RPAREN);

    return t;
}

// 规则 76-80: CallStmRest ::= ( ActParamList )
//              ActParamList ::= ε | Exp ActParamMore
//              ActParamMore ::= ε | , ActParamList
static TreeNode* callStmRest(TreeNode* idNode) {
    TreeNode* t = newTreeNode(StmtK);
    t->stmtkind = CallK;
    t->lineno = idNode->lineno;
    t->name = idNode->name;
    delete idNode;

    match(LPAREN);

    // ActParamList
    if (currentToken.lex != RPAREN) {
        t->child[0] = exp();
        TreeNode* last = t->child[0];

        while (currentToken.lex == COMMA) {
            match(COMMA);
            TreeNode* next = exp();
            last->sibling = next;
            last = next;
        }
    }

    match(RPAREN);
    return t;
}

// 规则 81-82: RelExp ::= Exp OtherRelE
//             OtherRelE ::= CmpOp Exp
static TreeNode* relExp() {
    TreeNode* left = exp();

    // CmpOp ::= < | =
    if (currentToken.lex == LT || currentToken.lex == EQ) {
        TreeNode* t = newTreeNode(ExpK);
        t->expkind = OpK;
        t->op = currentToken.lex;
        t->lineno = currentToken.lineNo;
        match(currentToken.lex);
        t->child[0] = left;
        t->child[1] = exp();
        return t;
    }

    return left;
}

// 规则 83-85: Exp ::= Term OtherTerm
//             OtherTerm ::= ε | AddOp Exp
static TreeNode* exp() {
    TreeNode* left = term();

    // AddOp ::= + | -
    while (currentToken.lex == PLUS || currentToken.lex == MINUS) {
        TreeNode* t = newTreeNode(ExpK);
        t->expkind = OpK;
        t->op = currentToken.lex;
        t->lineno = currentToken.lineNo;
        match(currentToken.lex);
        t->child[0] = left;
        t->child[1] = term();
        left = t;
    }

    return left;
}

// 规则 86-88: Term ::= Factor OtherFactor
//             OtherFactor ::= ε | MultOp Term
static TreeNode* term() {
    TreeNode* left = factor();

    // MultOp ::= * | /
    while (currentToken.lex == TIMES || currentToken.lex == OVER) {
        TreeNode* t = newTreeNode(ExpK);
        t->expkind = OpK;
        t->op = currentToken.lex;
        t->lineno = currentToken.lineNo;
        match(currentToken.lex);
        t->child[0] = left;
        t->child[1] = factor();
        left = t;
    }

    return left;
}

// 规则 89-91: Factor ::= ( Exp ) | INTC | Variable
static TreeNode* factor() {
    TreeNode* t = nullptr;

    switch (currentToken.lex) {
        case LPAREN:
            match(LPAREN);
            t = exp();
            match(RPAREN);
            break;
        case INTC:
            t = makeConstExpNode(stoi(currentToken.sem), currentToken.lineNo);
            match(INTC);
            break;
        case CHARC: {
            // 字符常量，存储为 ASCII 值
            t = newTreeNode(ExpK);
            t->expkind = ConstK;
            t->lineno = currentToken.lineNo;
            t->val = currentToken.sem.empty() ? 0 : (int)currentToken.sem[0];
            match(CHARC);
            break;
        }
        case ID:
            t = variable();
            break;
        default:
            syntaxError("Expected expression factor");
            t = makeConstExpNode(0, currentToken.lineNo);
            break;
    }

    return t;
}

// 规则 92-95: Variable ::= ID VariMore
//             VariMore ::= ε | [ Exp ] | . FieldVar
static TreeNode* variable() {
    TreeNode* t = newTreeNode(ExpK);
    t->expkind = IdV;
    t->lineno = currentToken.lineNo;

    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    }

    // VariMore
    if (currentToken.lex == LBRACE) {
        // [ Exp ]
        match(LBRACE);
        t->child[0] = exp();
        match(RBRACE);
    } else if (currentToken.lex == DOT) {
        // . FieldVar
        match(DOT);
        TreeNode* fv = fieldVar();
        // 将域名追加到变量名
        if (fv) {
            t->name += "." + fv->name;
            t->child[0] = fv->child[0]; // 可能的嵌套下标
            delete fv;
        }
    }

    return t;
}

// 规则 96-98: FieldVar ::= ID FieldVarMore
//             FieldVarMore ::= ε | [ Exp ]
static TreeNode* fieldVar() {
    TreeNode* t = newTreeNode(ExpK);
    t->expkind = IdV;
    t->lineno = currentToken.lineNo;

    if (currentToken.lex == ID) {
        t->name = currentToken.sem;
        match(ID);
    } else {
        syntaxError("Expected field name");
    }

    // FieldVarMore
    if (currentToken.lex == LBRACE) {
        match(LBRACE);
        t->child[0] = exp();
        match(RBRACE);
    }

    return t;
}
