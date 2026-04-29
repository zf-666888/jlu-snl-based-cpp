#include "globals.h"
#include "scan.h"
#include "parse.h"
#include "tree.h"
#include "analyze.h"

void printUsage(const char* prog) {
    cout << "Usage: " << prog << " <filename.snl> [options]" << endl;
    cout << "Options:" << endl;
    cout << "  -l    Lexical analysis only (print tokens)" << endl;
    cout << "  -p    Syntax analysis only (print syntax tree)" << endl;
    cout << "  -s    Semantic analysis only (check errors)" << endl;
    cout << "  -a    All phases (default)" << endl;
}

// 仅词法分析
void lexicalAnalysis(const string& filename) {
    cout << "===== Lexical Analysis =====" << endl;
    initScanner(filename);

    Token token;
    do {
        token = getToken();
        cout << "(" << token.lineNo << ", "
             << lexTypeToString(token.lex) << ", "
             << "\"" << token.sem << "\")" << endl;
    } while (token.lex != ENDFILE);

    cout << "===== End of Lexical Analysis =====" << endl << endl;
}

// 语法分析
TreeNode* syntaxAnalysis(const string& filename) {
    cout << "===== Syntax Analysis =====" << endl;
    initParser(filename);
    TreeNode* tree = parse();
    cout << endl << "Syntax Tree:" << endl;
    printSyntaxTree(tree);
    cout << "===== End of Syntax Analysis =====" << endl << endl;
    return tree;
}

// 语义分析
void semanticAnalysis(TreeNode* tree) {
    cout << "===== Semantic Analysis =====" << endl;
    initAnalyzer();
    analyze(tree);

    if (getSemanticErrorCount() == 0) {
        cout << "No semantic errors found." << endl;
    } else {
        cout << getSemanticErrorCount() << " semantic error(s) found." << endl;
    }
    cout << "===== End of Semantic Analysis =====" << endl << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    string filename = argv[1];
    string option = "-a";

    if (argc >= 3) {
        option = argv[2];
    }

    if (option == "-l") {
        lexicalAnalysis(filename);
    } else if (option == "-p") {
        syntaxAnalysis(filename);
    } else if (option == "-s") {
        TreeNode* tree = syntaxAnalysis(filename);
        semanticAnalysis(tree);
        freeTree(tree);
    } else {
        // 全部分析
        lexicalAnalysis(filename);
        TreeNode* tree = syntaxAnalysis(filename);
        semanticAnalysis(tree);
        freeTree(tree);
    }

    return 0;
}
