/*****************************************************
*	文件名:		codeGen.cpp
*	创建者：        BEAR
*	创建日期：	2020/5/13
*	最后修改：	2020/5/18
******************************************************
*	内容描述：
*		codeGen.cpp 实现了代码生成相关的函数。
******************************************************
*	修改记录：
*		2020/5/18  完成代码生成
*       2020/5/18  迁移至Qt平台
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

/*
    内存栈的结构示意：  （假设栈大小为MAX_ADDR = 1024） 下标往0方向是往栈顶方向

     global-var1					1023		<--- gp
     global-var2					1022
     global-var3[10]				1021
     ...(global variables)			1011

     一次活动记录（一次函数调用）：
     (假设一个函数 int fun(int a, int b[], int c)， 因此参数offset分别为0,1,2）
     ------------------ 装载参数 -----------------------
     a								1000(假设)
     b[]							 999
     c								 998
     ---------------------------------------------------
     return pc						 997			函数结束后将跳去的指令序号
     old bp						     996		<---- bp	（内存里存着主调函数的bp, 而正在执行的函数bp存在reg[bp]里）
     local variables				 995			局部变量存储区域
     other things					 995-localSize	其他中间结果
*/


/*
    写入目标代码的注释
*/
void Mini_C::emitComment(char *comment) {
    //printf("* %s\n", comment);
    codeGenResult->append(tr("* %1\n").arg(comment));
}


/*
    写入RO型（寄存器-寄存器型）指令
    格式如下：
    opcode r,s,t	comment
    opcode = 指令名
    r = 目标寄存器
    s = 寄存器1
    t = 寄存器2
    comment = 指令注释
*/
void Mini_C::emitRO(char *opcode, int r, int s, int t, char* comment) {
    //printf("%3d:  %5s  %d, %d, %d ", emitLoc++, opcode, r, s, t);
    //printf("\t%s\n", comment);

    codeGenResult->append(tr("%1:  %2  %3, %4, %5").arg(emitLoc++, 3).arg(opcode, 4).arg(r).arg(s).arg((t)));
    codeGenResult->append(tr("\t%1\n").arg(comment));

    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}


/*
    写入RM型（寄存器-存储器型）指令
    格式如下：
    opcode r, d(s)    comment
    opcode = 指令名
    r = 目标寄存器
    d = 偏移量
    s = 基址寄存器
    comment = 指令注释
*/
void Mini_C::emitRM(char *opcode, int r, int d, int s, char *comment) {
    //printf("%3d:  %5s  %d, %d(%d)", emitLoc++, opcode, r, d, s);
    //printf("\t%s\n", comment);

    codeGenResult->append(tr("%1:  %2  %3, %4(%5)").arg(emitLoc++, 3).arg(opcode, 4).arg(r).arg(d).arg(s));
    codeGenResult->append(tr("\t%1\n").arg(comment));

    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/*
    跳转若干个指令序号，并返回当前指令序号，以备回填的时候使用
*/
int Mini_C::emitSkip(int n) {
    int i = emitLoc;
    emitLoc += n;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
    return i;
}

/*
    将指令序号恢复为Skip之前的序号
*/
void Mini_C::emitBackup(int loc) {
    if (loc > highEmitLoc) {
        emitComment("BUG in emitBackup");
        Error = TRUE;
    }
    emitLoc = loc;
}

/*
    将指令序号恢复到最高指令序号
*/
void Mini_C::emitRestore(void) {
    emitLoc = highEmitLoc;
}

/*
    生成一些必要的运行时初始化的指令
*/
void Mini_C::emitPrelude() {
    emitComment("-> Prelude");

    // 设置gp为栈底位置（ADDRSIZE - 1）
    emitRM("LD", gp, 0, zero, "load maxaddress from location 0");

    // 清空dMem[0] (dMEM[0]在TM机中初始化为ADDRSIZE-1)
    emitRM("ST", zero, 0, zero, "clear location 0");

    // 设置sp初始位置：gp偏移全局变量消耗的存储空间
    emitRM("LDA", sp, -globalSize, gp, "allocate for global variables");

    emitComment("<- Perlude");
}

/*
    生成input()函数的指令
*/
void Mini_C::emitInput() {
    emitComment("-> function: input()");
    TreeNode *input = lookUpSymbol("input")->declaration;
    if (input) {
        input->offset = emitSkip(0);	// 使当前pc最为函数入口
    }

    emitRO("IN", ax, 0, 0, "input an integer from keyboard into ax");
    emitRM("LDA", sp, 1, sp, "prepare to pop");
    emitRM("LD", pc, -1, sp, "pop return pc");		// 弹出函数结束后应该跳转的地方，存放pc里面

    emitComment("<- function: input()");
}

/*
    生成output()函数的指令
*/
void Mini_C::emitOutput() {
    emitComment("-> function: output()");

    TreeNode *output = lookUpSymbol("output")->declaration;
    if (output) {
        output->offset = emitSkip(0);
    }

    emitRM("LD", ax, 1, sp, "load parameter into ax");		// ?
    emitRO("OUT", ax, 0, 0, "output the integer in ax");
    emitRM("LDA", sp, 1, sp, "prepare to pop");
    emitRM("LD", pc, -1, sp, "pop return pc");

    emitComment("<- function: output()");
}

/*
    生成调用函数需要的指令
*/
void Mini_C::emitCall(TreeNode *function) {
    // 将函数返回时要跳转的地方存好
    emitRM("LDA", ax, 3, pc, "store return pc");
    emitRM("LDA", sp, -1, sp, "prepare to push");
    emitRM("ST", ax, 0, sp, "push return pc");

    emitRM("LDC", pc, function->offset, 0, "jump to function");		// 跳入函数

    emitRM("LDA", sp, function->paramNum, sp, "release parameters");	// 释放参数占用的空间
}

/*
    寻找变量符号的地址
*/
void Mini_C::emitGetVariableAddress(TreeNode *variable) {
    TreeNode *dec = variable->declaration;

    if (dec) {
        /*
            全局变量
        */
        if (dec->isGlobal) {
            if (dec->nodekind == DecK && dec->kind.dec == ArrayDecK) {
                emitRM("LDA", bx, -(dec->offset), gp, "get global array address");
            }
            if (dec->nodekind == DecK && dec->kind.dec == VarDecK) {
                emitRM("LDA", bx, -(dec->offset), gp, "get global array address");
            }
        }

        /*
            参数变量
        */
        if (dec->isParam) {
            if (dec->nodekind == DecK && dec->kind.dec == ArrayDecK) {
                emitRM("LD", bx, 2 + dec->offset, bp, "get param array address");
            }
            if (dec->nodekind == DecK && dec->kind.dec == VarDecK) {
                emitRM("LDA", bx, 2 + dec->offset, bp, "get param variable address");
            }
        }

        /*
            局部变量
        */
        if (!dec->isGlobal && !dec->isParam) {
            if (dec->nodekind == DecK && dec->kind.dec == ArrayDecK) {
                emitRM("LDA", bx, -1 - (dec->offset), bp, "get local array address");
            }
            if (dec->nodekind == DecK && dec->kind.dec == VarDecK) {
                emitRM("LDA", bx, -1 - (dec->offset), bp, "get local variable address");
            }
        }

    }
}

/* ----------------------------------------------------------------------- */

/*
    “生成代码”模块的入口
*/
void Mini_C::codeGen(TreeNode *syntaxTree) {
    for (int i = 0; i < 20; i++) {
        param_stack[i] = NULL;
    }

    emitPrelude();
    emitComment("Jump to function: main()");

    // 此时loc = 3。
    // call(main)的生成在最后， 需要消耗5条指令， 但执行的的时候call(main)在Prelude前面
    int loc = emitSkip(6);

    emitInput();
    emitOutput();

    cGen(syntaxTree);

    emitBackup(loc);
    TreeNode* fun_main = lookUpSymbol("main")->declaration;
    emitCall(fun_main);
    emitRO("HALT", 0, 0, 0, "END OF PROGRAM");
}

/*
    真正的生成代码。 递归函数。
*/
void Mini_C::cGen(TreeNode *tree) {
    TreeNode *p = NULL;
    int loc1, loc2, loc3;			// 用于if和while语句
    int params_stack_top = 0;		// 用于CallK
    int temp;
    char *op_name = NULL;			// 用于OpK
    char *op_comment = NULL;		// 用于OpK

    while (tree) {
        switch (tree->nodekind) {
        case DecK:

            switch (tree->kind.dec) {

                /*
                    函数内部的代码生成
                */
            case FuncDecK:

                emitComment("-> function");

                tree->offset = emitSkip(0);		// 令当前pc为函数入口

                // 设置bp, sp
                emitRM("LDA", sp, -1, sp, "prepare to push");
                emitRM("ST", bp, 0, sp, "push old bp");
                emitRM("LDA", bp, 0, sp, "let bp = sp");
                emitRM("LDA", sp, -tree->function_size, sp, "allocate for local variables");	// 为函数的局部变量分配存储空间

                //cGen(tree->child[0]);	// parameters. 实际上这部分不会生成代码，因为是形参都是变量声明类型
                cGen(tree->child[1]);	// compound-stmt

                // 为返回类型为void的函数生成return 部分的代码
                // 因为void型返回值类型时， return语句不是必须的，所以return-statement不一定存在。
                // 在ReturnK中， 只负责返回值为int的情况。
                if (tree->returnType == Void) {
                    emitRM("LDA", sp, 0, bp, "let sp = bp");
                    emitRM("LDA", sp, 2, sp, "prepare to pop");	// 栈退2：1. old bp  2. return pc
                    emitRM("LD", bp, -2, sp, "pop old bp");
                    emitRM("LD", pc, -1, sp, "pop return pc");
                }

                emitComment("<- function");
                break;

            // 变量声明为变量的使用提供信息，但变量声明不用生成代码
            case VarDecK:
            case ArrayDecK:
                break;

            default:
                emitComment("BUG: unknown declaration kind.\n");
                Error = TRUE;
            }

            break;

        case StmtK:

            switch (tree->kind.stmt) {
            case CompoundK:

                emitComment("-> compound-statement");

                cGen(tree->child[0]);		// local-declarations. 实际上没有代码需要生成
                cGen(tree->child[1]);		// statement-list

                emitComment("<- compound-statement");

                break;

            case IfK:

                /*
                    if-statement
                        - child[0] : conditional-expression
                        - child[1] : true statements
                        - child[2] : false statements (else part statements)
                */

                emitComment("-> if-statement");

                emitComment("-> conditional expression");
                cGen(tree->child[0]);
                emitComment("<- conditional expression");

                // 若表达式为false，则要跳转到else part,
                // 但目前还不知道else part的入口是哪里， 所以预留一个位置给跳转指令，
                // 等到知道else入口再生成这个指令。
                loc1 = emitSkip(1);

                emitComment("-> 'then' part");
                cGen(tree->child[1]);
                emitComment("<- 'then' part");

                loc2 = emitSkip(1);		// 预留一个位置给跳出if语句的指令，到现在还不知道出口在哪

                loc3 = emitSkip(0);		// eles 入口
                emitBackup(loc1);
                emitRM("JEQ", ax, loc3, zero, "jump to else part");
                emitRestore();

                emitComment("-> 'else' part");
                cGen(tree->child[2]);
                emitComment("<- 'else' part'");
                loc3 = emitSkip(0);
                emitBackup(loc2);
                emitRM("LDA", pc, loc3, zero, "'then' part end and jump out if-statement");	// 回填：then部分结束后，跳出if语句
                emitRestore();

                emitComment("<- if-statement");

                break;

            case WhileK:

                /*
                    while-statement
                        - child[0] : conditional expression
                        - child[1] : statements
                */

                emitComment("-> while-statement");

                loc1 = emitSkip(0);		// statements结束后回到条件表达式部分
                emitComment("-> conditional expression");
                cGen(tree->child[0]);
                emitComment("<- conditional expression");

                loc2 = emitSkip(1);		// 预留一个指令位置，当条件表达式为false时跳出while

                emitComment("-> while-body");
                cGen(tree->child[1]);
                emitComment("<- while-body");

                emitRM("LDA", pc, loc1, zero, "jump to while-test");
                loc3 = emitSkip(0);		//while出口
                emitBackup(loc2);
                emitRM("JEQ", ax, loc3, zero, "get 'false' and jump out while-statement");
                emitRestore();

                emitComment("<- while-statement");

                break;

            case ReturnK:
                /*
                    返回值存在reg[ax]里面
                */

                emitComment("-> return");
                if (tree->declaration) {
                    if (tree->declaration->returnType == Integer) {
                        cGen(tree->child[0]);
                    }
                }

                emitRM("LDA", sp, 0, bp, "let sp = bp");	// 让sp回到函数基址那里
                emitRM("LDA", sp, 2, sp, "prepare to pop");
                emitRM("LD", bp, -2, sp, "pop old bp");		// dMem[reg[bp]]存着old bp
                emitRM("LD", pc, -1, sp, "pop return pc");
                emitComment("<- return");

                break;

            case AssignK:
                /*
                    assign
                        - child[0] : variable | array element
                        - child[1] : expression
                */
                emitComment("-> assign");

                /* 生成左部（赋值语句的左边是求其地址） */
                emitComment("-> left part");
                isGetValue = 0;
                cGen(tree->child[0]);
                emitComment("<- left part");

                /* 保护左部变量的地址 */
                emitRM("LDA", sp, -1, sp, "prepare to push");
                emitRM("ST", bx, 0, sp, "protect the left-part's address");

                /* 生成右部（右边是求其值） */
                emitComment("-> right part");
                isGetValue = 1;
                cGen(tree->child[1]);
                emitComment("<- right part");

                /* 恢复左部变量地址 */
                emitRM("LDA", sp, 1, sp, "pop prepare");
                emitRM("LD", bx, -1, sp, "recover the left-part's address");

                // 赋值
                emitRM("ST", ax, 0, bx, "here is our goal: ASSIGN");

                emitComment("<- assign");

                break;

            case CallK:
                /*
                    call
                        - child[0] : arguement
                            - next argument = child[0]->sibling....
                */
                p = tree->child[0];

                emitComment("-> call function");

                // 1.装载参数(后面的参数在递归时生成)
                // 2.调用函数

                while (p) {
                    param_stack[params_stack_top++] = p;
                    p = p->sibling;
                }

                isGenParam = 1;	//不重复生成参数
                while (params_stack_top > 0) {
                    cGen(param_stack[--params_stack_top]);
                    emitRM("LDA", sp, -1, sp, "prepare to push");
                    emitRM("ST", ax, 0, sp, "push parameter");
                }
                isGenParam = 0;

                emitCall(lookUpSymbol(tree->name)->declaration);
                emitComment("<- call function");

                break;

            default:
                emitComment("BUG: unknown statement kind.\n");
                Error = TRUE;
            }

            break;

        case ExpK:

            switch (tree->kind.exp) {

            case IdK:
                /*
                    id
                        - child[0] : expression ( 若child[0] = NULL，则id是普通变量或数组名；
                                                    否则id[expression]是数组的一个元素）
                */

                p = tree->declaration;
                if (p) {
                    if (tree->child[0] == NULL) {
                        /*
                            普通变量
                        */
                        if (p->nodekind == DecK && p->kind.dec == VarDecK) {
                            emitComment("-> variable");
                            emitGetVariableAddress(tree);	// 找地址
                            if (isGetValue) {
                                emitRM("LD", ax, 0, bx, "get variable value");
                            }
                            emitComment("<- variable");
                        }
                        /*
                            找数组地址（先存进bx， 若isGetValue则再存进ax）
                        */
                        if (p->nodekind == DecK && p->kind.dec == ArrayDecK) {
                            emitComment("-> array address");
                            emitGetVariableAddress(tree);
                            if (isGetValue) {
                                emitRM("LDA", ax, 0, bx, "put array address into ax");
                            }
                            emitComment("<- array address");
                        }
                    }
                    else {
                        /*
                            数组元素
                        */
                        if (p->nodekind == DecK && p->kind.dec == ArrayDecK) {
                            emitComment("-> array element");
                            emitGetVariableAddress(tree);

                            /* 后面的操作将使用到寄存器bx, 因此要把此时bx中的数组地址保存到别的地方，以防丢失 */
                            emitRM("LDA", sp, -1, sp, "prepare to push");
                            emitRM("ST", bx, 0, sp, "protect the array address");

                            temp = isGetValue;
                            isGetValue = 1;
                            emitComment("-> array index");
                            cGen(tree->child[0]);	// 最终的下标值将存进ax里
                            emitComment("<- array idnex");
                            isGetValue = temp;

                            /* 恢复数组地址到bx */
                            emitRM("LDA", sp, 1, sp, "pop prepare");
                            emitRM("LD", bx, -1, sp, "recover the array address into bx");

                            /* 取数组元素的地址； 取数组元素的值 */
                            emitRO("SUB", bx, bx, ax, "get array element address");
                            if (isGetValue) {
                                emitRM("LD", ax, 0, bx, "get array element value");
                            }

                            emitComment("<- array element");
                        }
                    }
                }

                break;

            case OpK:
                /*
                    op
                        - child[0] : left part
                        - child[1] : right part
                */

                emitComment("-> op");

                cGen(tree->child[0]);
                emitRM("LDA", sp, -1, sp, "prepare to push");
                emitRM("ST", ax, 0, sp, "store left part into data memory temporarily");	// 暂时把左部结果放到dMem[]里
                cGen(tree->child[1]);	// 这一步将右部的结果放在reg[ax]
                emitRM("LDA", sp, 1, sp, "prepare to pop");
                emitRM("LD", bx, -1, sp, "store left part into register now");	// 将暂存的左部放入reg[bx]

                // 上面把左右两个操作数分别存在reg[bx]和reg[ax]了，
                // 下面根据op进行对应的操作
                switch (tree->op) {
                    /*
                        算术表达式
                    */

                case PLUS:
                    emitRO("ADD", ax, bx, ax, "op +");
                    break;
                case MINUS:
                    emitRO("SUB", ax, bx, ax, "op -");
                    break;
                case TIMES:
                    emitRO("MUL", ax, bx, ax, "op *");
                    break;
                case DIV:
                    emitRO("DIV", ax, bx, ax, "op /");
                    break;

                    /*
                        条件表达式
                    */
                case EQ:
                case NOTEQ:
                case LT:
                case LTEQ:
                case GT:
                case GTEQ:
                    if (tree->op == EQ) { op_name = copyString("JEQ"); op_comment = copyString("op =="); }
                    if (tree->op == NOTEQ) { op_name = copyString("JNE"); op_comment = copyString("op !="); }
                    if (tree->op == LT) { op_name = copyString("JLT"); op_comment = copyString("op <"); }
                    if (tree->op == LTEQ) { op_name = copyString("LTE"); op_comment = copyString("op <="); }
                    if (tree->op == GT) { op_name = copyString("JGT"); op_comment = copyString("op >"); }
                    if (tree->op == GTEQ) { op_name = copyString("JGE"); op_comment = copyString("op >="); }

                    /*
                        emitRM("JEQ", ax, loc3, zero, "jump to else part");		-- if-stmt
                        emitRM("JEQ", ax, loc3, zero, "get 'false' and jump out while-statement");	-- while-stmt
                        下面一系列指令之后的一条指令将是上述指令中的一条, 是一个if或while条件判定为false时的跳转指令
                        因此下述指令的意图是：
                        1. 若判定为true, 则令reg[ax]不是0， 那么跳转指令的JEQ判定为false, 也就是说不进行跳转
                        2. 若判定为false, 则令reg[ax]是0， 那么跳转指令的JEQ判定为true, 进行跳转
                    */
                    emitRO("SUB", ax, bx, ax, op_comment);
                    emitRM(op_name, ax, 2, pc, "skip 2 pc if true");		// 如果判定为true， 跳过后两条指令, 为true case做准备
                    emitRM("LDC", ax, 0, 0, "set ax for jumping to false case");
                    emitRM("LDA", pc, 1, pc, "skip 1 pc if false");
                    emitRM("LDC", ax, 1, 0, "set ax for iumping to true case");

                    break;

                default:
                    emitComment("BUG: unknown operator");
                    Error = TRUE;
                }

                emitComment("<- op");
                break;

            case ConstK:

                emitComment("-> const");
                emitRM("LDC", ax, tree->val, 0, "store the number");
                emitComment("<- const");

                break;

            default:
                emitComment("BUG: unknown expression kind.");
                Error = TRUE;
            }

            break;

        default:
            emitComment("BUG: unknown nodekind.\n");
            Error = TRUE;
        }/* switch nodekind */

        if (isGenParam == 0) {
            tree = tree->sibling;
        }
        else break;

    }/* while */
}
























