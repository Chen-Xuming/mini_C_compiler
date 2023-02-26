/*****************************************************
*	文件名:		util.cpp
*	创建者：        BEAR
*	创建日期：	2020/4/8
*	最后修改：	2020/4/19
******************************************************
*	内容描述：
*		util.h定义一些与编译过程关系不大的辅助型
*		工具函数，例如打印语法树、创建节点等。
******************************************************
*	修改记录：
*	2020/4/19	添加函数printType()；完成打印语法树函数。
*   2020/4/25   将文件迁移至Qt平台
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

/*
 *		获取类型的字符串: int / void / unknown, 用于打印语法树
 */
static char str_int[] = "int";
static char str_void[] = "void";
static char str_unknown[] = "unknown";
char* Mini_C::typeToString(ExpType et) {
    if (et == Integer) return str_int;
    if (et == Void) return str_void;
    else return str_unknown;
}

/*
 *  返回+-/*等对应的字符串
 * */
QString opToString(TokenType token){
    QString op = "";
    switch(token){
    case PLUS: op = "+"; break;
    case MINUS: op = "-"; break;
    case TIMES: op = "*"; break;
    case DIV: op = "/"; break;
    case LT: op = "<"; break;
    case LTEQ: op = "<="; break;
    case EQ: op = "=="; break;
    case NOTEQ: op = "!="; break;
    case GT: op = ">"; break;
    case GTEQ: op = ">="; break;
    default: break;
    }
    return op;
}

/*
 *  打印TokenType所对应的具体符号
 */
QString Mini_C::printToken(TokenType token, const char* tokenString) {
    QString print;

    switch (token) {
    case IF:
    case ELSE:
    case INT:
    case RETURN:
    case VOID:
    case WHILE:
        print = tr("reserved word \"%1\"\n").arg(tokenString);
        break;
    case PLUS:		print = "+"; break;
    case MINUS:		print = "-"; break;
    case TIMES:		print = "*"; break;
    case DIV:		print = "/"; break;
    case LT:		print = "<"; break;
    case GT:		print = ">"; break;
    case LTEQ:      print = "<="; break;
    case GTEQ:      print = ">="; break;
    case ASSIGN:	print = "="; break;
    case EQ:		print = "=="; break;
    case NOTEQ:		print = "!=";break;
    case SEMI:		print = ";"; break;
    case COMMA:		print = ","; break;
    case LPAREN:	print = "("; break;
    case RPAREN:	print = ")"; break;
    case LBRACKET:	print = "["; break;
    case RBRACKET:	print = "]"; break;
    case LBRACE:	print = "{"; break;
    case RBRACE:	print = "}"; break;
    case ENDFILE:	print = "EOF"; break;
    case NUM:
        print = tr("NUM, value = %1").arg(tokenString);
        break;
    case ID:
        print = tr("ID, name = \"%1\"").arg(tokenString);
        break;
    case ERROR:
        print = tr("ERROR. \n\t Maybe meeting a Non-ASCII character or an illegal ASCII character(out of the comments).\n");
        break;
    default:
        print = tr("UNKNOWN TOKEN %1").arg(tokenString);
    }

    ui->compile_textEdit->insertPlainText(print);
    return print;
}

/*
 *  创建通用类型的树节点，将被下面三个创建具体类型树节点的函数调用。
 */
TreeNode * Mini_C::newTreeNode() {
    TreeNode *t = new TreeNode();
    if (!t){
        ui->compile_textEdit->insertPlainText(tr("Out of memory at line %1.\n").arg(lineno));
    }
    else{
        for (int i = 0; i < MAXCHILDREN; ++i) t->child[i] = NULL;
        t->sibling = NULL;
        t->lineno = lineno;
    }
    return t;
}

/*
 *  创建StmtKind类型的树节点
 */
TreeNode * Mini_C::newStmtNode(StmtKind stmtkind) {
    TreeNode *t = newTreeNode();
    if (t){
        t->nodekind = StmtK;
        t->kind.stmt = stmtkind;
    }
    return t;
}

/*
 *  创建ExpKind类型的树节点
 */
TreeNode * Mini_C::newExpNode(ExpKind expkind) {
    TreeNode *t = newTreeNode();
    if (t) {
        t->nodekind = ExpK;
        t->kind.exp = expkind;
    }
    return t;
}

/*
 *  创建DecKind类型的树节点
 */
TreeNode * Mini_C::newDecNode(DecKind deckind) {
    TreeNode *t = newTreeNode();
    if (t) {
        t->nodekind = DecK;
        t->kind.dec = deckind;
    }
    return t;
}

/*
 *  复制给定的字符串并返回新建的字符串
 */
char * Mini_C::copyString(char *s) {
    int n;
    char * t;
    if (s == NULL) return NULL;
    n = strlen(s) + 1;
    t = (char *)malloc(n);
    if (t == NULL)
        ui->compile_textEdit->insertPlainText(tr("Out of memory at line %1.\n").arg(lineno));
    else strcpy(t, s);
    return t;
}


/*
 *	以目录结构的形式打印语法树
 *		NodeKind { StmtK, ExpK, DecK };
 *		StmtKind { IfK, WhileK, AssignK, ReturnK, CallK, CompoundK };
 *		ExpKind { OpK, IdK, ConstK };
 *		DecKind { VarDecK, ArrayDecK, FuncDecK };
 */
void Mini_C::printTree(TreeNode *tree, QTreeWidgetItem *parentItem) {

    while (tree != NULL) {

        QString print = "";

        switch (tree->nodekind) {

        case StmtK:
            switch (tree->kind.stmt) {
            case IfK:
                print = "if";
                break;
            case WhileK:
                print = "while";
                break;
            case AssignK:
                print = "assign";
                break;
            case ReturnK:
                print = "return";
                break;
            case CallK:
                print = tr("call(%1)").arg(tree->name);
                break;
            case CompoundK:
                print = "compound";
                break;
            default:		/*should never happen*/
                print = "Unknown StmtNode kind";
                break;
            }
            break;

        case ExpK:
            switch (tree->kind.exp) {
            case OpK:
                print = tr("op(%1)").arg(opToString(tree->op));
                break;
            case ConstK:
                print = tr("const(%1)").arg(tree->val);
                break;
            case IdK:
                print = tr("id(%1").arg(tree->name);
                if(tree->child[0]){
                    print.append("[]");
                }
                print.append(")");
                break;
            default:		/*should never happen*/
                print = "Unknown ExpNode kind";
                break;
            }
            break;

        case DecK:
            switch (tree->kind.dec) {
            case VarDecK:
                // var-dec(名称, 类型)
                print = tr("var-dec(%1, %2)").arg(tree->name).arg(typeToString(tree->dataType));
                break;
            case ArrayDecK:
                // 数组变量：arr-dec(名称, 类型, 长度)
                // 函数形参：arr-dec(名称，类型)
                if (tree->isParam) {
                    print = tr("arr-dec(%1, %2)").arg(tree->name).arg(typeToString(tree->dataType));
                }
                else {
                    print = tr("arr-dec(%1, %2, %3)").arg(tree->name).arg(typeToString(tree->dataType)).arg(tree->arraySize);
                }
                break;
            case FuncDecK:
                // var-dec(名称, 返回类型)
                print = tr("fun-dec(%1, %2)").arg(tree->name).arg(typeToString(tree->returnType));
                break;
            default:		/*should never happen*/
                print = "Unknown DecNode kind";
                break;
            }
            break;
        default:
            print = "Unknown node kind";
            break;
        }

        QTreeWidgetItem *newItem = NULL;
        if(parentItem) {
            newItem = new QTreeWidgetItem(parentItem, QStringList(print));
            parentItem->addChild(newItem);
        }

        for (int i = 0; i < MAXCHILDREN; i++){
            if(tree->child[i] && newItem){
                printTree(tree->child[i], newItem);
            }
        }
        tree = tree->sibling;
    }

}



