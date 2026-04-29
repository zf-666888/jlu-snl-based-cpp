#include "tree.h"
#include "scan.h"

TreeNode* newTreeNode(NodeKind kind) {
    TreeNode* t = new TreeNode();
    t->nodekind = kind;
    return t;
}

static string nodeKindToString(NodeKind k) {
    switch (k) {
        case ProK: return "ProK";
        case PheadK: return "PheadK";
        case TypeK: return "TypeK";
        case VarK: return "VarK";
        case ProcDecK: return "ProcDecK";
        case DecK: return "DecK";
        case StmLk: return "StmLk";
        case StmtK: return "StmtK";
        case ExpK: return "ExpK";
        default: return "Unknown";
    }
}

static string stmtKindToString(StmtKind k) {
    switch (k) {
        case IfK: return "If";
        case WhileK: return "While";
        case AssignK: return "Assign";
        case ReadK: return "Read";
        case WriteK: return "Write";
        case CallK: return "Call";
        case ReturnK: return "Return";
        default: return "Unknown";
    }
}

static string expKindToString(ExpKind k) {
    switch (k) {
        case OpK: return "Op";
        case ConstK: return "Const";
        case IdV: return "IdV";
        default: return "Unknown";
    }
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) cout << "  ";
}

void printSyntaxTree(TreeNode* tree, int indent) {
    if (tree == nullptr) return;

    printIndent(indent);

    switch (tree->nodekind) {
        case ProK:
            cout << "ProK" << endl;
            break;
        case PheadK:
            cout << "PheadK  " << tree->name << endl;
            break;
        case TypeK:
            cout << "TypeK" << endl;
            break;
        case VarK:
            cout << "VarK" << endl;
            break;
        case ProcDecK:
            cout << "ProcDecK  " << tree->name << endl;
            break;
        case DecK:
            cout << "DecK  " << tree->type_name;
            for (auto& id : tree->id_list) cout << "  " << id;
            if (tree->is_var_param) cout << "  (var param)";
            cout << endl;
            break;
        case StmLk:
            cout << "StmLk" << endl;
            break;
        case StmtK:
            cout << "StmtK  " << stmtKindToString(tree->stmtkind);
            if (tree->stmtkind == ReadK || tree->stmtkind == CallK) {
                cout << "  " << tree->name;
            }
            cout << endl;
            break;
        case ExpK:
            cout << "ExpK  ";
            if (tree->expkind == OpK) {
                cout << "Op  ";
                switch (tree->op) {
                    case PLUS: cout << "+"; break;
                    case MINUS: cout << "-"; break;
                    case TIMES: cout << "*"; break;
                    case OVER: cout << "/"; break;
                    case LT: cout << "<"; break;
                    case EQ: cout << "="; break;
                    default: cout << lexTypeToString(tree->op); break;
                }
            } else if (tree->expkind == ConstK) {
                cout << "Const  " << tree->val;
            } else if (tree->expkind == IdV) {
                cout << tree->name << "  IdV";
            }
            cout << endl;
            break;
    }

    // 打印子节点
    for (int i = 0; i < 4; i++) {
        if (tree->child[i]) {
            printSyntaxTree(tree->child[i], indent + 1);
        }
    }

    // 打印兄弟节点
    if (tree->sibling) {
        printSyntaxTree(tree->sibling, indent);
    }
}

void freeTree(TreeNode* tree) {
    if (tree == nullptr) return;
    for (int i = 0; i < 4; i++) {
        freeTree(tree->child[i]);
    }
    freeTree(tree->sibling);
    delete tree;
}
