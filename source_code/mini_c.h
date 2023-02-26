/*****************************************************
 *	文件名:		mini_c.h
 *	创建者：        BEAR
 *	创建日期：	2020/4/25
 *	最后修改：	2020/5/18
 ******************************************************
 *	内容描述：
 *		mini_c.h定义了一个Mini_C类，继承于主窗口界面类QMainQindow，负责完成所有关于界面交互的工作。
 *      由于Qt的特性，在界面类以外访问控件比较繁琐，因此所有编译过程使用的函数都声明在Mini_C类了，以便于输出显示各种信息，
 *      但函数实现则写在各个cpp文件，mini_c.cpp只实现界面操作相关的函数。
 ******************************************************
 *	修改记录：
 *      2020/4/25   将项目迁移至Qt平台，并对程序作了少许为了适应Qt平台特性的修改。
 *      2020/5/13   添加语法分析部分一些用于收集信息的变量。
 *      2020/5/18   添加语义分析和代码生成部分的函数声明。
 *      2020/5/19   添加虚拟机部分的变量和函数。
 *******************************************************/


#ifndef MINI_C_H
#define MINI_C_H

#include <QMainWindow>
#include <Qsci/qsciscintilla.h>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QTreeWidgetItem>
#include <set>

#include "globals.h"
#include "keyword_hash.h"

namespace Ui {
class Mini_C;
}

class Mini_C : public QMainWindow
{
    Q_OBJECT

public:
    explicit Mini_C(QWidget *parent = 0);
    ~Mini_C();

protected:
    void closeEvent(QCloseEvent *event);    // 关闭（退出系统）时，检查是否需要保存文件

/*
 *   一些按钮触发的事件
 * */
private slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    bool saveTree();
    bool saveGenCode();
    void about();
    void documentWasModified();
    void compile();
    void run();
    void moveToEnd();

private:
    Ui::Mini_C *ui;

    // 全路径文件名，文件操作使用QFile类
    QString sourceFile;      // 源文件
    QString syntaxTreeFile;  // 语法树文件
    QString genCodeFile;     // 生成代码文件

    /*************************
     *   界面控件
     *************************/
    QsciScintilla *codeEditor;
    QsciScintilla *codeGenResult;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *compileAct;
    QAction *runAct;
    //QAction *saveSyntaxTreeAct;
    //QAction *saveGenCodeAct;
    QMenu *fileMenu;
    //QMenu *midMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *runToolBar;

private:

    /************************************
     *      界面操作相关的函数
     ************************************/
    bool maybeSave();       // 询问是否保存修改
    void createActions();   // 一些按键
    void createMenus();     // 设置菜单
    void createToolBars();  // 设置工具栏
    void codeEditor_init();     // 代码编辑框初始化设置
    void codeGenResult_init();  // 代码生成结果显示框初始化
    void setCurrentFile(const QString &fileName);       // 设置当前文件名
    void loadFile(const QString &fileName);             // 加载文件内容到代码编辑框显示
    bool saveFile(const QString &fileName);             // 将代码编辑框的内容写入文件
    void saveSyntaxTree(QTreeWidgetItem *item, QTextStream &out);       //用于保存语法树


    /***********************************************
     *      编译过程使用的一些辅助工具
     * *********************************************/

    QTreeWidgetItem *startItem = NULL;  // 用于绘制语法树
    int indentno = 0;   // 用于保存语法树

    char* typeToString(ExpType);
    bool isDigit(int);
    bool isAlpha(int);
    QString printToken(TokenType, const char*);
    TreeNode * newTreeNode();
    TreeNode * newStmtNode(StmtKind);
    TreeNode * newExpNode(ExpKind);
    TreeNode * newDecNode(DecKind);
    char * copyString(char *);
    void printTree(TreeNode *, QTreeWidgetItem *);
    void init_data();

    int Error = FALSE;      //编译错误标志

    /***********************************
     *      词法分析相关函数和变量
     ***********************************/

    char tokenString[MAXTOKENLEN + 1];  /* 存储一个被扫描的单词 */

    QString lineBuf;            /* 存储一行源代码 */
    int lineno = 0;             //行号
    int linepos = 0;			/* 当前字符在当前行中的下标 */
    int bufsize = 0;			/* 当前源代码行的长度 */
    int EOF_flag = FALSE;       /* 是否到达文件尾 */

    int getNextChar(void);      // 获取下一字符
    void ungetNextChar(void);   // 回退一个字符
    TokenType getToken(void);   // 词法分析入口函数



    /*************************************
     *      语法分析相关函数和变量
     *************************************/

    TreeNode *syntaxTree;                           // 语法树根节点， 最重要的变量，贯穿语法、语义分析及代码生成
    TokenType token;                                // 存储扫描到的“单词”类型

    TreeNode *parse();                              // 语法分析入口函数
    void syntaxError(char * message);               // 输出语法错误
    void match(TokenType expected);                 // “单词”匹配函数
    ExpType getTypeSpecifier(TokenType token);      // 获取类型的工具函数

    // 一系列语法分析函数
    TreeNode* declaration_list();
    TreeNode* declaration();
    TreeNode* var_declaration(char* id, ExpType type_specifier);
    TreeNode* fun_declaration(char* id, ExpType type_specifier);
    TreeNode* params();
    TreeNode* param_list();
    TreeNode* param();
    TreeNode* compound_stmt();
    TreeNode* local_declarations();
    TreeNode* statement_list();
    TreeNode* statement();
    TreeNode* expression_stmt();
    TreeNode* selection_stmt();
    TreeNode* iteration_stmt();
    TreeNode* return_stmt();
    TreeNode* expression();
    TreeNode* simple_expression(TreeNode* look_ahead);
    TreeNode* additive_expression(TreeNode* look_ahead);
    TreeNode* term(TreeNode* look_ahead);
    TreeNode* factor(TreeNode* look_ahead);
    TreeNode* args();
    TreeNode* arg_list();

    void freeSyntaxTree(TreeNode*&);        // 释放语法树的空间

    /* 2020/5/13 添加：建立语法树过程中收集一些为了代码生成部分的信息 */
    static int globalSize;      // 全局变量占用的空间大小
    static int local_offset;    // 局部变量偏移量
    static int param_NUM;       // 函数参数个数（用于函数调用时为参数分配空间）
    static int param_offset;    // 函数参数偏移量



    /*************************************
     *      语义分析相关函数和变量
     *          包括符号检查、类型检查两部分
     *************************************/

    /* ------------ 符号检查 --------------- */

    // 符号表（哈希表）的拉链节点结构体定义放在globals.h了

    HashListNode* hashTable[BUCKETCOUNT];       // 符号表
    int scopeDepth = 0;                         // 作用域深度
    std::set<int> scope_bucket;                 // 记录哪些桶里是存了东西的
    static TreeNode *funDec_for_returnStmt;		// 为return语句记录下函数声明的信息
    static TreeNode *last_global_declaration;	// 记录最后一个全局声明，最后一个全局声明必须是一个叫main的函数声明
    bool first_compound = true;                 // 见函数buildSymbolTable(TreeNode*)
    bool has_return_stmt = false;               // 记录函数是否有return语句

    void initSymbolTable();                     // 初始化哈希表
    int symbolHash(char *key);                  // 哈希函数
    void systemFunctionsPredefine();            // 标准系统函数预定义
    void insertSymbol(TreeNode *defineNode);    // 插入符号
    void deleteSymbols();                       // 删除符号
    bool isDeclared(char *name);                // 检查一个符号是否已定义
    HashListNode* lookUpSymbol(char *name);     // 在符号表里查询一个符号
    void buildSymbolTable();                    // 建立符号表（模块入口函数）
    void buildSymbolTable(TreeNode* syntaxTree);// 真正的建立符号表函数

    /* ----------- 类型检查 --------------- */
    void typeCheck(TreeNode *syntaxTree);       // 类型检查（既是模块入口函数，也是功能实现函数）


    void semanticError(int lineno, char *s);    // 输出语义分析错误



    /*************************************
     *      代码生成相关的函数和变量
     *************************************/

    /* 数据寄存器 */
    const int zero = 0;     // 0寄存器，存放常数0
    const int ax = 1;       // 数据寄存器1
    const int bx = 2;       // 数据寄存器2

    /* 指针寄存器 */
    const int sp = 4;       // 栈顶指针寄存器（stack-pointer）
    const int bp = 5;       // 活动记录基址指针寄存器（based-pointer）
    const int gp = 6;       // 全局变量区基址指针寄存器（global-pointer）
    const int pc = 7;       // 程序计数器（下一指令地址）（program-counter）

    int emitLoc = 0;        // 当前指令序号
    int highEmitLoc = 0;    // 目前为止最大的指令序号
    TreeNode *param_stack[20];	// 用于CallK中的参数加载
    int isGetValue = 1;     // 对于一个id, 是否要取其值？ 0：只取地址（存在reg[bx]里）	1：取地址后再取其值（存在reg[ax]里）
    int isGenParam = 0;     // 生成函数参数(sibling指针连接)时，不要重复递归调用cGen

    void emitComment(char *comment);        // 生成注释
    void emitRO(char *opcode, int r, int s, int t, char* comment);      // 生成RO型指令
    void emitRM(char *opcode, int r, int d, int s, char *comment);      // 生成RM型指令
    int emitSkip(int n);                    // 跳过若干个指令序号
    void emitBackup(int loc);               // 跳到给定指令序号
    void emitRestore(void);                 // 恢复到当前最高序号
    void emitPrelude();                     // 生成一些必要的用于程序初始化的指令
    void emitInput();                       // 生成input()的指令
    void emitOutput();                      // 生成output(int x)的指令
    void emitCall(TreeNode *function);      // 生成函数调用指令
    void emitGetVariableAddress(TreeNode *variable);    // 生成寻找变量地址的指令

    void cGen(TreeNode *tree);              // “生成代码”模块的核心函数，递归生成各类型节点对应的代码
    void codeGen(TreeNode *syntaxTree);     // “生成代码”模块的入口函数



    /*************************************
     *         虚拟机相关的函数和变量
     *************************************/

    INSTRUCTION iMem[IADDR_SIZE];       // 指令
    int dMem[DADDR_SIZE];               // 内存
    int reg[NO_REGS];                   // 寄存器

    char * opCodeTab[20] = {              // 操作码
        /* RR opcodes */
        "HALT","IN","OUT","ADD","SUB","MUL","DIV","????",

        /* RM opcodes */
        "LD","ST","????",

        /* RA opcodes */
        "LDA","LDC","JLT","JLE","JGT","JGE","JEQ","JNE","????"
    };

    char * stepResultTab[5] = {          // 指令运行结果
        "OK",
        "Halted",
        "Instruction Memory Fault",
        "Data Memory Fault",
        "Division by 0"
    };

    char in_Line[LINESIZE];
    int lineLen;
    int inCol;
    int num;
    char word[WORDSIZE];
    char ch;
    int done;

    void init_virtual_machine();    // 初始化虚拟机
    int opClass(int c);             // 获取指令类型
    void getCh();                   // 读取字符
    int nonBlank();                 // 跳过空格
    int getNum();                   // 读取整数
    int getWord();                  // 读取操作码
    int skipCh(char c);             // 跳过字符
    int run_error(char * msg, int lineNo, int instNo);  //输出指令错误
    int readInstructions();         // 从文件读取指令
    STEPRESULT stepTM();            // 执行一条指令
    void run_virtual_machine();     // 运行虚拟机，功能模块入口函数
};

#endif // MINI_C_H
