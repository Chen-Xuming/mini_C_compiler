/*****************************************************
*	文件名:		typeCheck.cpp
*	创建者：        BEAR
*	创建日期：	2020/5/4
*	最后修改：	2020/5/6
******************************************************
*	内容描述：
*		typeCheck.cpp实现了函数typeCheck()，负责类型检查功能
******************************************************
*	修改记录：
*		2020/5/6	完成类型检查功能。
*       2020/5/18   迁移至Qt平台并做一些适应性修改
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

/*
*		类型检查函数.
*/
void Mini_C::typeCheck(TreeNode *syntaxTree) {
    TreeNode *p = syntaxTree;
    char errortips[120];

    while (p != NULL){

        /*
            后序遍历
        */
        for (int i = 0; i < MAXCHILDREN; ++i)
            typeCheck(p->child[i]);

        /*
            处理不同类型的节点
        */
        switch (p->nodekind) {

        /* 声明类型: 数组、普通变量只能声明为整型 */
        case DecK:
            switch (p->kind.dec) {

            case VarDecK:
                if (p->dataType != Integer) {
                    sprintf(errortips, "the type of variable '%s' should be 'int'", p->name);
                    semanticError(p->lineno, errortips);
                }
                p->expressionType = p->dataType;
                break;

            case ArrayDecK:
                if (p->dataType != Integer) {
                    sprintf(errortips, "the type of array '%s' should be 'int'", p->name);
                    semanticError(p->lineno, errortips);
                }
                p->expressionType = Array;
                break;

            case FuncDecK:
                p->expressionType = Function;
                break;

            default:
                sprintf(errortips, "unknown declaration kind");
                semanticError(p->lineno, errortips);
                break;
            }

            break;		/* DecK */

        /* 语句类型 */
        case StmtK:

            switch (p->kind.stmt) {

            /*
                if语句和while语句： 检查其条件表达式(child[0])的类型是否是Integer型（非0则表示true）
            */
            case IfK:
            case WhileK:
                if (p->child[0] && p->child[0]->expressionType != Integer) {
                    sprintf(errortips, "the conditional expreesion's type should be 'int'");
                    semanticError(p->lineno, errortips);
                }
                break;

            /*
                赋值语句： 检查赋值符两边的表达式类型是否相同。
                注意： 左表达式只能是var -> ID | ID[expreesion]， 即不能给call()、复合表达式等赋值，
                        这个错误已经在语法分析的时候处理了。
            */
            case AssignK:
                if (p->child[0] && p->child[1]) {
                    if (p->child[0]->expressionType == Integer && p->child[1]->expressionType == Integer) {
                        p->expressionType = p->child[0]->expressionType;
                    }
                    else {
                        sprintf(errortips, "both left-expreesion's type and right-expression's should be 'int' in assign statement");
                        semanticError(p->lineno, errortips);
                    }
                }
                else {
                    sprintf(errortips, "illegal assign statement");
                    semanticError(p->lineno, errortips);
                }
                break;

            /*
                return语句：返回的表达式类型必须和函数返回值类型一致。
                如果是“return;”，根据语法分析的设定，child[0]为NULL。
                另外， 对于返回值为int而缺失return语句的错误，已经在symbolTable部分进行排查和报告
            */
            case ReturnK:
                if (p->declaration) {
                    if (p->declaration->returnType == Integer) {
                        if (p->child[0] && p->child[0]->expressionType != Integer || p->child[0] == NULL) {
                            sprintf(errortips, "the type of return-expreesion should be 'int' in function '%s'", p->declaration->name);
                            semanticError(p->lineno, errortips);
                        }
                    }
                    if (p->declaration->returnType == Void) {
                        if (p->child[0] != NULL) {
                            sprintf(errortips, "unexpected return type in function '%s'", p->declaration->name);
                            semanticError(p->lineno, errortips);
                        }
                    }
                }
                break;

            /*
                函数调用语句： 检查函数形参和实参的个数以及类型是否一致
            */
            case CallK:
            {
                TreeNode* actual = p->child[0];	// 形参
                TreeNode* formal = NULL;		// 实参
                if (p->declaration) {
                    formal = p->declaration->child[0];
                }

                bool flag = true;
                while (actual && formal) {
                    if (actual->expressionType != formal->expressionType) {		// 形参和实参类型不一致
                        flag = false;
                    }
                    actual = actual->sibling;
                    formal = formal->sibling;
                }
                if (!(actual == NULL && formal == NULL)) {		// 不全为NULL代表形参和实参在数量上不一致
                    flag = false;
                }

                if (!flag) {
                    sprintf(errortips, "different types or number for actual and formal parameter(s) of function '%s'", p->name);
                    semanticError(p->lineno, errortips);
                }
                else {
                    p->expressionType = p->declaration->returnType;
                }
                break;
            }
            /*
                复合语句不需要也无法进行类型检查
            */
            case CompoundK:
                p->expressionType = Unknown;
                break;

            default:
                sprintf(errortips, "unknown statement kind");
                semanticError(p->lineno, errortips);
                break;
            }

            break;		/* StmtK */

        /* 表达式类型 */
        case ExpK:

            switch (p->kind.exp) {

            /*
                标志符类型: 普通变量、数组、数组元素
            */
            case IdK:
                if (p->declaration) {
                    if (p->declaration->expressionType == Integer) {
                        if (p->child[0] == NULL) {
                            p->expressionType = Integer;
                        }
                        else {
                            sprintf(errortips, "illegal usage of Integer '%s'", p->name);
                            semanticError(p->lineno, errortips);
                        }
                    }
                    else {
                        if (p->declaration->expressionType == Array) {
                            // 数组
                            if (p->child[0] == NULL) {
                                p->expressionType = Array;
                            }
                            // 数组元素
                            else if (p->child[0]->expressionType == Integer) {
                                p->expressionType = p->declaration->dataType;
                            }
                            else {
                                sprintf(errortips, "illegal usage of Array '%s'", p->name);
                                semanticError(p->lineno, errortips);
                            }
                        }
                        else {
                            sprintf(errortips, "unknown type of identifier '%s'", p->name);
                            semanticError(p->lineno, errortips);
                        }
                    }
                }
                break;

            /*
                常量类型
            */
            case ConstK:
                p->expressionType = Integer;
                break;

            /*
                运算符类型: 算术运算符、关系运算符
            */
            case OpK:
                switch (p->op) {
                case PLUS:
                case MINUS:
                case TIMES:
                case DIV:
                case LT:
                case LTEQ:
                case EQ:
                case NOTEQ:
                case GT:
                case GTEQ:
                    if (p->child[0] && p->child[1]) {
                        if (p->child[0]->expressionType == Integer && p->child[1]->expressionType == Integer) {
                            p->expressionType = Integer;
                        }
                        else {
                            sprintf(errortips, "the operands' types of a(n) arithmetic/relational operator should be 'int'");
                            semanticError(p->lineno, errortips);
                        }
                    }
                    else {
                        sprintf(errortips, "there should be an expression on each side of a(n) arithmetic/relational operator");
                        semanticError(p->lineno, errortips);
                    }
                    break;

                default:
                    sprintf(errortips, "unknown operator");
                    semanticError(p->lineno, errortips);
                }
                break;

            default:
                sprintf(errortips, "unknown expression kind");
                semanticError(p->lineno, errortips);
                break;
            }

            break;		/* ExpK */

        default:
            sprintf(errortips, "unknown kind");
            semanticError(p->lineno, errortips);
            break;
        }	/* switch(p->nodekind) */

        p = p->sibling;
    }
}
