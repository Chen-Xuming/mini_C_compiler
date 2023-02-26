/*****************************************************
*	文件名:		symbolTable.cpp
*	创建者：        BEAR
*	创建日期：	2020/5/4
*	最后修改：	2020/5/6
******************************************************
*	内容描述：
*		symbolTable.cpp  实现了建立符号表相关的函数。
*
*       实际上，我更愿意把“建立符号表”这个过程称为“符号检查”， 因为在这个过程中，
*       每退出一个域（scope）就会把其中的局部变量符号从符号表中删除了，到最后符号表中只剩下全局符号（函数和全局变量）。
*       除了生成代码部分要用lookUpSymbol()查找函数之外，符号表并无其他作用了，所以符号表主要是为了符号检查而暂时需要的产物。
******************************************************
*	修改记录：
*		2020/5/6	初步完成符号检查和符号信息收集工作。
*
*       2020/5/18   迁移至Qt平台并做一些适应性修改
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

TreeNode* Mini_C::funDec_for_returnStmt = NULL;
TreeNode* Mini_C::last_global_declaration = NULL;

/*
*	输出语义错误
*/
void Mini_C::semanticError(int lineno, char *s) {
    ui->compile_textEdit->insertPlainText(tr(">>>Semantic error at line %1: %2.\n")
                                          .arg(lineno).arg((s)));
    Error = TRUE;
}


/*
*	初始化符号表，将所有头结点赋值为null
*/
void Mini_C::initSymbolTable() {
    for (int i = 0; i < BUCKETCOUNT; i++) {
        hashTable[i] = NULL;
    }
    scopeDepth = 0;
}


/*
*	哈希函数，使用的是电子教材上面介绍的那个
*/
int Mini_C::symbolHash(char *key) {
    int value = 0;
    int i = 0;
    while (key[i] != '\0'){
        value = ((value << 4) + key[i]) % BUCKETCOUNT;
        ++i;
    }
    return value;
}

/*
*	插入预定义函数
*	int input(void){...}
*	void output(int x){...}
*/
void Mini_C::systemFunctionsPredefine() {

    TreeNode *input = newDecNode(FuncDecK);
    if (input) {
        input->name = copyString("input");
        input->returnType = Integer;
        input->expressionType = Function;
        input->lineno = 0;
    }

    TreeNode *output = newDecNode(FuncDecK);
    if (output) {
        output->lineno = 0;
        output->name = copyString("output");
        output->returnType = Void;
        output->expressionType = Function;
        output->paramNum = 1;

        TreeNode *x = newDecNode(VarDecK);		// 参数
        if (x) {
            x->name = copyString("x");
            x->dataType = Integer;
            x->expressionType = Integer;
            output->child[0] = x;
        }
    }

    insertSymbol(input);
    insertSymbol(output);
}


/*
 *	插入符号。采用的是头插入法, 因此任意一个节点的scopeDepth总是大于或等于后一个节点
 */
void Mini_C::insertSymbol(TreeNode *defineNode) {
    /* 重定义 */
    if (isDeclared(defineNode->name)) {
        char errortips[50];
        sprintf(errortips, "multiple definition of '%s'", defineNode->name);
        semanticError(defineNode->lineno, errortips);
    }
    /* 创建新节点然后插入 */
    else {
        int hashValue = symbolHash(defineNode->name);

        HashListNode* newSymbol = new HashListNode();
        if (newSymbol) {
            newSymbol->name = defineNode->name;
            newSymbol->declaration = defineNode;
            newSymbol->scopeDepth = scopeDepth;

            HashListNode* temp = hashTable[hashValue];
            hashTable[hashValue] = newSymbol;
            newSymbol->next = temp;

            scope_bucket.insert(hashValue);		// 记录桶号
        }
    }
}

/*
*	删除符号：当一个作用域结束之后，删除这个作用域里的所有局部定义的变量
*	按照拉链节点的scopeDepth来判断一个符号属于当前作用域的局部定义符号。
*/
void Mini_C::deleteSymbols() {
    for (std::set<int>::iterator it = scope_bucket.begin(); it != scope_bucket.end();) {
        if (hashTable[*it] != NULL) {
            HashListNode *p = hashTable[*it];
            while (p) {
                if (p->scopeDepth == scopeDepth) {
                    hashTable[*it] = p->next;
                    delete p;
                    p = hashTable[*it];
                }
                else break;
            }
            if (p == NULL) {		// 该桶已清空
                scope_bucket.erase(it++);
                continue;
            }
        }
        it++;
    }
}


/*
*	当遇到声明语句时，检查该作用域下这个符号是否已声明过了。（排查符号重定义的错误）
*/
bool Mini_C::isDeclared(char *name) {
    int hashValue = symbolHash(name);
    HashListNode *p = hashTable[hashValue];
    bool found = false;
    while (p) {
        if (p->scopeDepth == scopeDepth) {
            if (strcmp(name, p->name) == 0) {
                found = true;
                break;
            }
            p = p->next;
        }
        else break;
    }
    return found;
}

/*
*	当遇到一个符号时，检查其是否已经被声明。（排查使用未声明符号的错误）
*	若符号已被声明，则以“就近原则”返回一个拉链节点, 因为当前作用域下定义的符号将覆盖所有外面定义的同名符号。
*/
HashListNode* Mini_C::lookUpSymbol(char *name) {
    int hashValue = symbolHash(name);
    HashListNode *p = hashTable[hashValue];
    while (p) {
        if (strcmp(name, p->name) == 0) {
            break;
        }
        p = p->next;
    }
    return p;	/* 没找到时，p将为NULL */
}


/*
*	建立符号表(模块入口函数)
*/
void Mini_C::buildSymbolTable() {
    initSymbolTable();
    systemFunctionsPredefine();
    buildSymbolTable(syntaxTree);

    if (!(last_global_declaration && last_global_declaration->kind.dec == FuncDecK	// 有短路规则
        && strcmp(last_global_declaration->name, "main") == 0)) {
        ui->compile_textEdit->insertPlainText(
                    ">>>Semantic error: the last declaration should be a function named 'main'.\n");
        Error = TRUE;
    }
}


/*
*	真正的建立符号表函数
*	主要的工作：
*		1. 遇到声明语句，检查是否重定义，若无则插入符号表
*		2. 遇到符号使用，则检查其是否已经声明，以及记录它的声明是哪个（存在不同作用域但同名的声明）
*		3. 标记好作用域的深度：进入函数、复合语句 scopeDepth 加1， 退出则 scopeDepth 减1,并从符号表删除局部符号
*
*	first_compound用于查看当前扫描到的compound-stmt是不是直属于fun-declaration的。
*	由于函数形参的作用域是函数主体， 如果进入函数时以及进入函数{}的时候都进行 scopeDepth++， 显然是不对的。
*/
void Mini_C::buildSymbolTable(TreeNode* syntaxTree) {
    TreeNode *p = syntaxTree;
    char errortips[50];

    while (p != NULL) {
        /*
            插入符号，实现细节详见函数insertSymbol
        */
        if (p->nodekind == DecK) {
            insertSymbol(p);
            if (scopeDepth == 0) {		// 全局声明
                last_global_declaration = p;
            }
        }

        /*
            进入函数, 创建新的作用域.
        */
        if (p->nodekind == DecK && p->kind.dec == FuncDecK) {
            ++scopeDepth;
            funDec_for_returnStmt = p;
        }

        /*
            进入compoud-stmt {...}, 如果不是fun-declaration的，则创建新的作用域
        */
        if (p->nodekind == StmtK && p->kind.stmt == CompoundK) {
            if (first_compound == true) {
                first_compound = false;		// 忽略该{}并改变标志
            }
            else {
                ++scopeDepth;
            }
        }

        /*
            符号使用前检查其是否已声明（作为表达式的ID; 作为函数调用的ID）
        */
        if ((p->nodekind == ExpK && p->kind.exp == IdK) || p->nodekind == StmtK && p->kind.stmt == CallK) {
            HashListNode* lookup = lookUpSymbol(p->name);
            if (lookup) {
                p->declaration = lookup->declaration;
            }
            else {
                sprintf(errortips, "undeclared identifier '%s'", p->name);
                semanticError(p->lineno, errortips);
            }
        }

        /*
            为return语句记录下函数声明的信息，以便于随后的类型检查中return语句跟函数返回值类型匹配。
            这本身不属于建立符号表的内容，只是借着建立符号表的先序遍历过程，完成一些为后续步骤提供便利的工作
        */
        if (p->nodekind == StmtK && p->kind.stmt == ReturnK) {
            p->declaration = funDec_for_returnStmt;
            has_return_stmt = true;
        }

        for (int i = 0; i < MAXCHILDREN; i++) {
            buildSymbolTable(p->child[i]);
        }

        /*
            作用域结束, 删除局部声明的变量
        */
        if (p->nodekind == StmtK && p->kind.stmt == CompoundK) {
            deleteSymbols();
            scopeDepth--;
        }
        if (p->nodekind == DecK && p->kind.dec == FuncDecK) {
            first_compound = true;
            funDec_for_returnStmt = NULL;

            if (p->returnType == Integer && has_return_stmt == false) {		// 有返回值的函数丢失return语句
                sprintf(errortips, "missing return statement in function '%s'", p->name);
                semanticError(p->lineno, errortips);
            }
            has_return_stmt = false;
        }

        p = p->sibling;
    }
}










