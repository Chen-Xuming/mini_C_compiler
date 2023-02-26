/*****************************************************
*	文件名:		parse.cpp
*	创建者：        BEAR
*	创建日期：	2020/4/12
*	最后修改：	2020/4/19
******************************************************
*	内容描述：
*		parse.cpp主要根据MiniC的语法规则定义一系列递归下降分析函数，
*	此外是语法分析入口函数parse()和几个辅助函数。
*
*		在分析过程中不必纠结于一些语义问题，确保语法正确就可以了，
*		生成的语法树可能包含一些语义问题，但这些语义问题留给语义分析部分。
******************************************************
*	修改记录：
*	2020/4/19	初步完成所有递归下降函数的定义。
*
*	2020/4/20	完成所有语法规则的测试（因此也完成词法分析测试），并修复测试过程中发现的错误。
*				对于正确的源代码，分析程序能够较好地识别； 对错误的识别，分析程序不一定能做到精准、明了，
*				但最低要求是尽量不要让程序崩溃，以下有两点防止分析程序崩溃的措施：
*				① 避免野指针，不访问NULL指针。对所有指针初始化为NULL，使用指针之前确保不是NULL。
*				② 防止陷入死循环。测试过程中发现，在某些情况下会循环报 “unexpected token -> EOF”的错误，
*				   因此在部分循环体内部要检查token == ENDFILE。只要程序能够正常执行结束，都不是很坏的结果。
*
*   2020/4/25   将文件迁移至Qt平台。
*
*   2020/5/13   在语法树生成过程中，为代码生成部分收集部分必要的信息。
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

/*
 *      静态变量定义
 */
int Mini_C::globalSize = 0;
int Mini_C::param_NUM = 0;
int Mini_C::local_offset = 0;
int Mini_C::param_offset = 0;

/*
*		语法分析入口函数。
*/
TreeNode *Mini_C::parse() {
    TreeNode *t;
    token = getToken();			// 词法分析入口
    t = declaration_list();		// 语法分析入口
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}

/*
*		输出语法错误的函数。
*/
void Mini_C::syntaxError(char * message) {
    ui->compile_textEdit->insertPlainText(tr(">>> Syntax error at line %1: %2").arg(lineno).arg(message));
    Error = TRUE;
}

/*
*		“单词”匹配函数。
*		如果当前已扫到的TokenType与期望的TokenType吻合，就继续进行词法分析扫描，
*		否则报出语法错误。
*/
void Mini_C::match(TokenType expected)
{
    if (token == expected) token = getToken();
    else {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        ui->compile_textEdit->insertPlainText("\n");
    }
}

/*
*		获取一个token是什么类型（int还是void）。
*		若非int或void，提示语法错误并返回Unknown类型。
*/
ExpType Mini_C::getTypeSpecifier(TokenType token) {
    ExpType et;
    if (token == INT || token == VOID) {
        et = token == INT ? Integer : Void;
        match(token);
    }
    else {
        syntaxError("Declarations should begin with a type-specifier.\n");
        et = Unknown;
    }
    return et;
}




/*************************************** 递归下降分析函数  *****************************************/


/*
 *		声明语句列表，至少要有一条声明语句。
 *		declaration-list → declaration-list  declaration  |  declaration
 *	    改造左递归:declaration-list → declaration  declaration*
 *
 *		follow = { EOF }
 *		declaration-list其实是一系列由sibling指针连接的declaration链表，因此整个流程本质是链表的尾插入
 *
 *      2020/5/13   代码生成
 *		此处的声明全是全局声明（变量、函数）， 因此在这里做isGlobal标记、全局变量offset标记以及globalSize的统计。
 */
TreeNode* Mini_C::declaration_list() {
    TreeNode *t = declaration();
        TreeNode *p = t;

        int offset = 0;

        if (p) {
            if (p->nodekind == DecK) {
                if (p->kind.dec == VarDecK) {
                    p->isGlobal = true;
                    p->offset = offset++;
                }
                if (p->kind.dec == ArrayDecK) {
                    p->isGlobal = true;
                    p->offset = offset;
                    offset += p->arraySize;
                }
            }
        }

        while (p && token != ENDFILE) {
            TreeNode *temp = declaration();
            if (temp) {
                if (temp->nodekind == DecK) {
                    if (temp->kind.dec == VarDecK) {
                        temp->isGlobal = true;
                        temp->offset = offset++;
                    }
                    if (temp->kind.dec == ArrayDecK) {
                        temp->isGlobal = true;
                        temp->offset = offset;
                        offset += temp->arraySize;
                    }
                }

                p->sibling = temp;
                p = temp;
            }
        }

        globalSize = offset;

        return t;
}

/*
 *		declaration		→ var-declaration | fun-declaration
 *		var-declaration → type-specifier ID; | type-specifier ID[NUM];
 *		fun-declaration → type-specifier ID( params ) compound-stmt
 *		type-specifier  →  int  |  void
 *
 *		提取左公因子： declaration -> type-specifier ID S
 *								S  -> ; | [NUM] | (params) compound-stmt
 *
 *		由于存在两个左公因子，即第三个单词才会出现区别，
 *		同时前两个单词 类型、ID 很重要，所以要事先记录下来.
 *
 *		First = { int , void }
 */
TreeNode* Mini_C::declaration() {
    TreeNode *t = NULL;
    ExpType type_specifier = getTypeSpecifier(token);
    char *id = copyString(tokenString);

    match(ID);

    switch (token) {
    case SEMI:			// 普通变量声明
    case LBRACKET:		// 数组变量声明
        t = var_declaration(id, type_specifier);
        break;
    case LPAREN:		// 函数声明
        t = fun_declaration(id, type_specifier);
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        ui->compile_textEdit->insertPlainText("\n");
        token = getToken();
        break;
    }	/* switch(token) */

    return t;
}

/*
 *		var-declaration → type-specifier ID; | type-specifier ID[NUM];
 *
 *		因为declaration()中存在左公因子 type-specifier 和 ID, 且都是要记录的属性，
 *		所以此函数设计为带参函数，fun_declaration()也一样。
 *
 *      2020/5/13 代码生成
 *		在此计算变量（局部）的偏移量
 */
TreeNode* Mini_C::var_declaration(char* id, ExpType type_specifier) {
    TreeNode *t = NULL;

    if (token == SEMI) {		//普通变量
        t = newDecNode(VarDecK);
        if (t) {
            t->name = id;
            t->dataType = type_specifier;
            t->offset = local_offset++;
        }
        match(SEMI);
    }
    else {						// 数组变量
        t = newDecNode(ArrayDecK);
        if (t) {
            t->name = id;
            t->dataType = type_specifier;
        }
        match(LBRACKET);
        if (t) {
            t->arraySize = atoi(tokenString);
            t->offset = local_offset;
            local_offset += t->arraySize;
        }
        match(NUM);
        match(RBRACKET);
        match(SEMI);
    }
    return t;
}

/*
 *		fun-declaration → type-specifier ID( params ) compound-stmt
 *
 *      2020/5/13  代码生成
 *		在此统计函数参数个数（paramNum）、 参数偏移量； 局部变量总大小（function_size）、 局部变量偏移量
 */
TreeNode* Mini_C::fun_declaration(char* id, ExpType type_specifier){
    local_offset = 0;

    TreeNode *t = newDecNode(FuncDecK);
    if (t) {
        t->name = id;
        t->returnType = type_specifier;
    }
    match(LPAREN);
    if (t) {
        t->child[0] = params();
        t->paramNum = param_NUM;
        param_NUM = 0;
        param_offset = 0;
    }
    match(RPAREN);
    if (t) {
        t->child[1] = compound_stmt();
    }

    if (t) {
        t->function_size = local_offset;
    }
    local_offset = 0;

    return t;
}

/*
 *		params -> params_list |  void
 */
TreeNode* Mini_C::params() {
    TreeNode *t = NULL;

    if (token == VOID) {	// void代表无参数，因此也可以不生成树节点
        match(VOID);
    }else {
        t = param_list();
    }

    return t;
}

/*
 *		param_list -> param (,param)*
 *
 *      2020/5/13   代码生成
 *		在此统计函数参数个数paramNum
 */
TreeNode* Mini_C::param_list() {
    TreeNode *t = param();
    TreeNode *p = t;

    if (p) {
        param_NUM++;
    }

    while (p && token == COMMA) {
        match(COMMA);
        TreeNode *temp = param();
        if (temp) {
            param_NUM++;
            p->sibling = temp;
            p = temp;
        }
    }
    return t;
}

/*
 *		 param -> type-specifier ID | type-specifier ID []
 *
 *      2020/5/13  代码生成
 *		在此记录参数偏移量
 */
TreeNode* Mini_C::param() {
    TreeNode *t = NULL;
    ExpType type_specifier = getTypeSpecifier(token);	// 此处可能会得到Void型，符合语法不符合语义，因此不做处理
    char *id = copyString(tokenString);
    match(ID);

    //t = token == LBRACKET ? newDecNode(ArrayDecK) : newDecNode(VarDecK);

    // 参数是数组变量
    if (token == LBRACKET) {
        match(LBRACKET);
        match(RBRACKET);
        t = newDecNode(ArrayDecK);
    }

    // 参数是普通变量
    else {
        t = newDecNode(VarDecK);
    }

    if (t) {
        t->name = id;
        t->dataType = type_specifier;
        t->isParam = true;
        t->offset = param_offset++;
    }

    return t;
}

/*
 *		compoud-stmt -> { local-declarations  statement-list }
 */
TreeNode* Mini_C::compound_stmt() {
    TreeNode *t = newStmtNode(CompoundK);
    match(LBRACE);
    if (t) {
        t->child[0] = local_declarations();
        t->child[1] = statement_list();
    }
    match(RBRACE);
    return t;
}

/*
 *		local-declarations → var-declaration*
 */
TreeNode* Mini_C::local_declarations() {
    TreeNode *t = NULL;
    TreeNode *p = NULL;
    ExpType type_specifier;
    char *id = NULL;

    if (token == INT || token == VOID) {
        type_specifier = getTypeSpecifier(token);
        id = copyString(tokenString);
        match(ID);
        t = var_declaration(id, type_specifier);
    }
    p = t;
    while (p && (token == INT || token == VOID)) {
        type_specifier = getTypeSpecifier(token);
        id = copyString(tokenString);
        match(ID);
        TreeNode *temp = var_declaration(id, type_specifier);
        if (temp) {
            p->sibling = temp;
            p = temp;
        }

        // 这里是防止某些情况下，无限循环报 " unexpected token -> EOF " 的错误导致程序崩溃。
        if (token == ENDFILE) break;
    }
    return t;
}

/*
 *		statement-list → statement*
 *		Follow = {  '}' }
 */
TreeNode* Mini_C::statement_list() {
    /*
     *		expression-stmt -> ;   会导致statement()返回NULL；
     *		为了保证list的头部不为空，可以先创建一个节点（实则无意义）。后面若遇到NULL，在while中可丢弃。
     */
    TreeNode *t = new TreeNode();
    TreeNode *p = t;
    while (p && token != RBRACE) {
        TreeNode *temp = statement();
        if (temp) {
            p->sibling = temp;
            p = temp;
        }

        // 这里是防止某些情况下，无限循环报 " unexpected token -> EOF " 的错误导致程序崩溃。
        if (token == ENDFILE) break;
    }
    p = t->sibling;
    delete t;
    return p;
}

/*
 *		statement → expression-stmt  |  compound-stmt  |  selection-stmt  |  iteration - stmt  |  return-stmt
 *		First = { ';', ID, NUM, '(',	// exp-stmt
 *				  '{',					// comp-stmt
 *				   if,					// if-stmt
 *				   while,				// while-stmt
 *				   return				// return-stmt
 *				}
 */
TreeNode* Mini_C::statement() {
    TreeNode *t = NULL;
    switch (token) {
    case SEMI:
    case ID:
    case NUM:
    case LPAREN:
        t = expression_stmt();
        break;
    case LBRACE:
        t = compound_stmt();
        break;
    case IF:
        t = selection_stmt();
        break;
    case WHILE:
        t = iteration_stmt();
        break;
    case RETURN:
        t = return_stmt();
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        ui->compile_textEdit->insertPlainText("\n");
        token = getToken();
        break;
    }
    return t;
}

/*
 *		expression-stmt → expression; | ;
 */
TreeNode* Mini_C::expression_stmt() {
    TreeNode *t = NULL;

    if (token == SEMI) {		// 只有一个分号，符合语法但无实际意义，故不生成节点。
        match(SEMI);
    }
    else {
        t = expression();
        match(SEMI);
    }
    return t;
}

/*
 *		selection-stmt  →  if ( expression ) statement  | if ( expression ) statement  else  statement
 */
TreeNode* Mini_C::selection_stmt() {
    TreeNode *t = newStmtNode(IfK);

    match(IF);
    match(LPAREN);
    if (t) {
        t->child[0] = expression();
        match(RPAREN);
        t->child[1] = statement();
    }
    if (token == ELSE) {
        match(ELSE);
        if (t) {
            t->child[2] = statement();
        }
    }
    return t;
}

/*
 *		iteration-stmt → while ( expression ) statement
 */
TreeNode* Mini_C::iteration_stmt() {
    TreeNode *t = newStmtNode(WhileK);

    match(WHILE);
    match(LPAREN);
    if (t) {
        t->child[0] = expression();
    }
    match(RPAREN);
    if (t) {
        t->child[1] = statement();
    }
    return t;
}

/*
 *		return-stmt →  return;  |  return expression;
 */
TreeNode* Mini_C::return_stmt() {
    TreeNode *t = newStmtNode(ReturnK);

    match(RETURN);
    if (token != SEMI) {
        if (t) {
            t->child[0] = expression();
        }
    }
    match(SEMI);
    return t;
}

/*
 *		expression  →  var = expression  |  simple-expression
 *		first = { '(', ID, NUM }
 *
 *		存在左公因子 var -> ID | ID[expression], 要前探若干个单词，因此要对ID的信息做记录，
 *		对于simple-expression还要将这些信息传递给后面的文法规则函数。
 */
TreeNode* Mini_C::expression() {
    TreeNode *t = NULL;
    TreeNode *look_ahead = NULL;
    char *id_string = NULL;

    /*
     *	是否获取 var ->ID | ID[expression] 的标志，用于判断expression是赋值语句还是简单表达式，因为var是左公因子
     */
    bool flag = false;

    if (token == ID) {
        flag = true;

        id_string = copyString(tokenString);
        match(ID);

        // 函数调用语句
        if (token == LPAREN) {
            match(LPAREN);
            t = newStmtNode(CallK);
            if (t) {
                t->child[0] = args();
                t->name = id_string;
                match(RPAREN);
            }
            // return t;	// 错误。原因： 函数调用也可以是simple-expression的一部分
            look_ahead = t;
        }

        /*
         *		var -> ID | ID[expression],
         *		可能属于	expression  →  var = expression
         *		也可能属于	expression  →	simple-expression，
         *		需要记录下来。
         */
        else {
            TreeNode *index = NULL;			// 记录数组下标（如果id是数组的话）
            if (token == LBRACKET) {		// ID[]
                match(LBRACKET);
                index = expression();
                match(RBRACKET);
            }

            // 若扫到的是其他符号，则判定ID是普通变量
            t = newExpNode(IdK);
            if (t) {
                t->name = id_string;
                t->child[0] = index;			// 如果不是数组，child[0]本身就是null
                look_ahead = t;
            }
        }
    }  /* if(token == ID) */

    // 赋值语句
    if (flag && token == ASSIGN) {
        if (t && t->nodekind == ExpK && t->kind.exp == IdK) {
            match(ASSIGN);
            TreeNode *assign = newStmtNode(AssignK);
            if (assign) {
                assign->child[0] = t;
                assign->child[1] = expression();
                return assign;
            }
        }

        // 避免函数调用语句之后出现 “ = ”.
        else {
            syntaxError("Illegal assignment: the left-value should be a variable.\n");		// 非法赋值：左值必须是一个var
            token = getToken();
        }
    }

    //  simple-expression
    else {
        t = simple_expression(look_ahead);
    }

    return t;
}

/*
 *		simple-expression → additive-expression  relop  additive-expression  |  additive - expression
 */
TreeNode* Mini_C::simple_expression(TreeNode* look_ahead) {
    TreeNode *t = additive_expression(look_ahead);

    if(token == LTEQ || token == LT || token == GT || token == GTEQ || token == EQ || token == NOTEQ){
        TokenType op = token;
        match(token);
        TreeNode *r_addexp = additive_expression(NULL);

        TreeNode *simple_exp = newExpNode(OpK);
        if (simple_exp) {
            simple_exp->op = op;
            simple_exp->child[0] = t;
            simple_exp->child[1] = r_addexp;
            t = simple_exp;
        }
    }

    return t;
}

/*
 *		additive-expression  →  additive-expression  addop  term  |  term
 *
 *	 => additive-expression  →  term term*
 */
TreeNode* Mini_C::additive_expression(TreeNode* look_ahead) {
    TreeNode *t = term(look_ahead);
    TreeNode *p = NULL;

    while (token == PLUS || token == MINUS) {
        p = newExpNode(OpK);

        if (p) {
            p->op = token;
            match(token);
            p->child[0] = t;
            p->child[1] = term(NULL);
            t = p;
        }
    }

    return t;
}

/*
 *		term  →  term  mulop  factor  |  factor
 *
 *	=>  term  →  factor factor*
 */
TreeNode* Mini_C::term(TreeNode* look_ahead) {
    TreeNode *t = factor(look_ahead);
    TreeNode *p = NULL;

    while (token == TIMES || token == DIV) {
        p = newExpNode(OpK);

        if (p) {
            p->op = token;
            match(token);
            p->child[0] = t;
            p->child[1] = factor(NULL);
            t = p;
        }
    }

    return t;
}

/*
 *		factor → (expression) |  var  |  call  |  NUM
 *
 *		若 look_ahead != NULL, 那么它携带的是 var 的信息，直接返回即可,
 *		但并非所有 var 都由 look_ahead 指针携带，look_ahead 仅携带来自 expression() 的var.
 */
TreeNode* Mini_C::factor(TreeNode* look_ahead) {
    if (look_ahead) {
        return look_ahead;
    }

    TreeNode *t = NULL;
    char *id_string = NULL;

    switch (token) {

    // var | call, 与expression()中的情形几乎一样
    case ID:
        id_string = copyString(tokenString);
        match(ID);
        if (token == LPAREN) {
            match(LPAREN);
            t = newStmtNode(CallK);
            if (t) {
                t->child[0] = args();
                t->name = id_string;
                match(RPAREN);
            }
        }
        else {
            TreeNode *index = NULL;			// 记录数组下标（如果id是数组的话）
            if (token == LBRACKET) {		// ID[]
                match(LBRACKET);
                index = expression();
                match(RBRACKET);
            }

            // 若扫到的是其他符号，则判定ID是普通变量
            t = newExpNode(IdK);
            if (t) {
                t->name = id_string;
                t->child[0] = index;			// 如果不是数组，child[0]本身就是null
            }
        }
        break;


    // NUM
    case NUM:
        t = newExpNode(ConstK);
        if (t) {
            t->val = atoi(tokenString);
        }
        match(NUM);
        break;

    // (expression)
    case LPAREN:
        match(LPAREN);
        t = expression();
        match(RPAREN);
        break;

    default:
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        ui->compile_textEdit->insertPlainText("\n");
        token = getToken();
    }

    return t;
}

/*
 *		args → arg-list  |  empty
 *		follow = { ')' }
 */
TreeNode* Mini_C::args() {
    TreeNode *t = NULL;
    if (token != RPAREN) {
        t = arg_list();
    }
    return t;
}


/*
 *		arg-list  →  arg-list, expression | expression
 *	=>  arg-list  →  expression {, expression}
 */
TreeNode* Mini_C::arg_list() {
    TreeNode *t = expression();
    TreeNode *p = t;
    while (p && token == COMMA) {
        match(COMMA);
        TreeNode *temp = expression();
        if (temp) {
            p->sibling = temp;
            p = temp;
        }
    }
    return t;
}

