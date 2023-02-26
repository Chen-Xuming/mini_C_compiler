/*****************************************************
 *	文件名:		globals.h
 *  	创建者：        BEAR
 *	创建日期：	2020/4/8
 *	最后修改：	2020/5/19
 ******************************************************
 *	内容描述：
 *		globals.h声明或定义一些作用于整个工程的常量和数据结构。
 ******************************************************
 *	修改记录：
 *		2020/4/13	给语法树节点添加2个属性，存储变量/函数返回值的类型。
 *
 *		2020/4/19	1. 给语法树节点添加三个属性：数组规模、数组下标、是否是函数形参。
 *					2. 放弃使用 union{...}attr, 原因：union中的属性不支持多个属性同时有效，
 *					   后续步骤中，一个变量可能要同时存储 name, val 两个属性。
 *      2020/4/25   将文件迁移至Qt平台
 *
 *      2020/5/4	增添语法树节点属性 declaration, expressionType
 *		2020/5/7	删除ExpType中的Boolean类型
 *
 *		2020/5/13	语法树节点添加4个用于代码生成的属性;
 *  				添加全局变量globalSize，也是用于生成代码。
 *
 *		2020/5/15	删除语法树节点属性 index(数组下标)，因为child[0]可以表示数组下标
 *
 *      2020/5/18   将符号表拉链节点结构体的定义放在这里
 *
 *      2020/5/19   添加虚拟机相关的数据结构定义
 *******************************************************/





#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#define FALSE 0
#define TRUE 1


/**********************************************************************/
/******************************词法分析相关******************************/
/**********************************************************************/

/* 最长单词长度 */
#define MAXTOKENLEN     64

/* 保留字（关键字）的数目 */
#define MAXRESERVED 6


/* 词法记号类型 */
enum TokenType {
    ENDFILE, ERROR,

    /* 关键字 */
    ELSE, IF, INT, RETURN, VOID, WHILE,

    /* 标志符和常数 */
    ID, NUM,

    /* 特殊符号 */
    PLUS, MINUS, TIMES, DIV,	// + - * /
    ASSIGN, EQ, NOTEQ,			// ASSIGN -> =(赋值)，EQ -> == ("is equal?")，NOTEQ -> !=
    LT, LTEQ,					// LT -> less-then  <
    GT, GTEQ,					// GT -> greater-than >
    COMMA,						// 逗号
    SEMI,						// 分号
    LPAREN, RPAREN,				// 左右括号( )
    LBRACKET, RBRACKET,			// 左右方括号[ ]
    LBRACE, RBRACE				// 左右花括号{ }
};


/* DFA中的状态 */
enum StateType {
    START, DONE, INERROR, INNUM, INID,
    INEQ,					// 扫描到=，可能是 = / ==
    INNOTEQ,				// 扫到!，可能是 != / error
    INLT,					// 扫到<，可能是 < / <=
    INGT,					// 扫到>，可能是 > / >=
    IN_COMMENT_OR_DIV,		// 扫到/，可能是 除号/ 或者 左注释/*
    INCOMMENT_L,			// 扫到/*，若再扫到*则进入INCOMMENT_R,否则不变
    INCOMMENT_R				//  扫到/*...*，若再扫到/则进入DONE即注释结束，
                            //扫到*则不变，其他则返回到INCOMMENT_L
};

/******************* 词法分析 END ***********************************/


/**************************************************************/
/****************语法树相关的数据结构******************************/
/**************************************************************/

/*
 *	语法树节点类型：语句类型、表达式类型、声明类型
 */
enum NodeKind{ StmtK, ExpK, DecK };

/*
 *	语句类型：if语句；while语句；赋值语句；return语句；函数调用语句；复合语句
 */
enum StmtKind{ IfK, WhileK, AssignK, ReturnK, CallK, CompoundK };

/*
 *	表达式类型：带操作符的表达式；单个id；常量
 */
enum ExpKind{ OpK, IdK, ConstK};

/*
 *	声明类型：普通变量声明；数组声明；函数声明。
 *  声明语句本质上属于StmtKind，但相对于普通语句又有特殊之处，所以暂定细分出DecKind（2020/4/8）。
 */
enum DecKind{ VarDecK, ArrayDecK, FuncDecK };

/* 用于类型检查 */
enum  ExpType{ Void, Integer, Array, Function, Unknown };

#define MAXCHILDREN 3		// 语法树的最大孩子数，暂定为3，需要时再作改变。（2020/4/8）

/*
 *  语法树节点结构体
 *	语法树在整个编译过程中起到很关键的作用，更具体地说，树节点携带的信息十分重要。
 *  但对于没有编译器开发经验的人来说，很难完全确定树节点需要定义哪些属性，因此不断对语法树节点进行改造是难以避免的。
 */
struct TreeNode
{
    TreeNode * child[MAXCHILDREN];		// 孩子节点
    TreeNode * sibling;					// 兄弟节点
    int lineno;							// 所在行号
    NodeKind nodekind;					// 节点类型
    union {								// 具体节点类型
        StmtKind stmt;
        ExpKind exp;
        DecKind dec;
    } kind;


    // exp类型（op;const;id）存储的信息
    TokenType op;
    int val;
    char * name;

    // 数组规模
    int arraySize;

    // 是否是函数形参		（用于代码生成）
    bool isParam = false;

    // 是否是全局变量		（用于代码生成）
    bool isGlobal = false;

    // 函数局部变量的size	（用于代码生成）
    int function_size = 0;

    // 函数的参数个数		（用于代码生成）
    int paramNum = 0;

    // 偏移量				(用于代码生成)
    int offset = 0;

    ExpType dataType;			// 存储变量的类型
    ExpType returnType;			// 存储函数返回值的类型

    TreeNode *declaration = NULL;	// 记录一个id是在哪里声明的（通过声明节点可以得到一个id的许多信息）

    ExpType  expressionType;	// 用于类型检查
};

/******************* 语法树 END *******************************/


/**************************************************************/
/******************** 符号表（哈希表）拉链节点 ********************/
/**************************************************************/
#define BUCKETCOUNT 211     // 哈希表桶数

struct HashListNode {
    char *name = NULL;					// id名
    TreeNode *declaration = NULL;		// 记录符号声明的节点
    HashListNode *next = NULL;			// 下一节点指针
    int scopeDepth = 0;					// 作用域深度（用于区分同名符号）
};





/**************************************************************/
/******************** 虚拟机相关的数据结构（和TM机无异） ************/
/**************************************************************/

#define   IADDR_SIZE  1024 /* increase for large programs */
#define   DADDR_SIZE  1024 /* increase for large programs */
#define   NO_REGS 8
#define   PC_REG  7

#define   LINESIZE  121
#define   WORDSIZE  20

/******* type  *******/

enum OPCLASS{
    opclRR,     /* reg operands r,s,t */
    opclRM,     /* reg r, mem d+s */
    opclRA      /* reg r, int d+s */
};

enum OPCODE{
    /* RR instructions */
    opHALT,    /* RR     halt, operands are ignored */
    opIN,      /* RR     read into reg(r); s and t are ignored */
    opOUT,     /* RR     write from reg(r), s and t are ignored */
    opADD,    /* RR     reg(r) = reg(s)+reg(t) */
    opSUB,    /* RR     reg(r) = reg(s)-reg(t) */
    opMUL,    /* RR     reg(r) = reg(s)*reg(t) */
    opDIV,    /* RR     reg(r) = reg(s)/reg(t) */
    opRRLim,   /* limit of RR opcodes */

    /* RM instructions */
    opLD,      /* RM     reg(r) = mem(d+reg(s)) */
    opST,      /* RM     mem(d+reg(s)) = reg(r) */
    opRMLim,   /* Limit of RM opcodes */

    /* RA instructions */
    opLDA,     /* RA     reg(r) = d+reg(s) */
    opLDC,     /* RA     reg(r) = d ; reg(s) is ignored */
    opJLT,     /* RA     if reg(r)<0 then reg(7) = d+reg(s) */
    opJLE,     /* RA     if reg(r)<=0 then reg(7) = d+reg(s) */
    opJGT,     /* RA     if reg(r)>0 then reg(7) = d+reg(s) */
    opJGE,     /* RA     if reg(r)>=0 then reg(7) = d+reg(s) */
    opJEQ,     /* RA     if reg(r)==0 then reg(7) = d+reg(s) */
    opJNE,     /* RA     if reg(r)!=0 then reg(7) = d+reg(s) */
    opRALim    /* Limit of RA opcodes */
};

enum STEPRESULT{
    srOKAY,
    srHALT,
    srIMEM_ERR,
    srDMEM_ERR,
    srZERODIVIDE
};

struct INSTRUCTION{
    int iop;
    int iarg1;
    int iarg2;
    int iarg3;
};

#endif
