/*****************************************************
 *	文件名:		mini_c.cpp
 *	创建者：        BEAR
 *	创建日期：	2020/4/25
 *	最后修改：	2020/5/18
 ******************************************************
 *	内容描述：
 *		mini_c.cpp对系统进行初始化，并实现所有关于界面操作的函数，
 ******************************************************
 *	修改记录：
 *      2020/5/18 添加语义分析、生成代码相关的UI操作
 *******************************************************/



#include "mini_c.h"
#include "ui_mini_c.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QStyleFactory>

#include <QDebug>

/*
 *  Mini_C的构造函数，用于初始化系统。
 */
Mini_C::Mini_C(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Mini_C)
{
    ui->setupUi(this);

    createActions();    // 添加按钮
    createMenus();      // 添加菜单
    createToolBars();   // 添加工具栏
    codeEditor_init();  // 代码编辑框初始化
    codeGenResult_init();   // 生成代码结果框初始化

    setCurrentFile("");   // 源文件名
    syntaxTreeFile = "";  // 语法树文件名
    genCodeFile = "";     // 生成代码文件名

    showMaximized();    //最大化

    syntaxTree = NULL;

    ui->treeWidget->setStyle(QStyleFactory::create("windows"));

    ui->compile_textEdit->setReadOnly(true);
    ui->run_textEdit->setReadOnly(true);

    connect(ui->run_textEdit, SIGNAL(textChanged()), this, SLOT(moveToEnd()));
}

/*
 *  Mini_C的析构函数
 * */
Mini_C::~Mini_C()
{
    delete ui;
}

/*
 *  点击“新建文件”
 * */
void Mini_C::newFile(){
    if (maybeSave()) {
        codeEditor->clear();
        setCurrentFile("");
        syntaxTreeFile = "";
        genCodeFile = "";
        init_data();
    }
}

/*
 *  点击“打开文件”
 *  */
void Mini_C::open(){
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this,tr("打开文件"),tr("./Testcase"), tr("文本文件(*.txt)"));
        if (!fileName.isEmpty()){
            init_data();
            QString temp = sourceFile, temp2 = sourceFile;
            syntaxTreeFile = temp.replace(tr(".txt"), tr("_SYNTAXTREE.txt"));
            genCodeFile = temp2.replace(tr(".txt"), tr("_CODE.txt"));
            loadFile(fileName);
        }
    }
}

/*
 *  加载文件内容到代码编辑框显示
 * */
void Mini_C::loadFile(const QString &fileName){
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("提示"),
                             tr("无法读取文件 %1:\n错误原因：%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    codeEditor->setText(in.readAll());      // 在代码框载入文件内容

    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
}

/*
 *  点击“保存文件”，这里保存的是源代码，即代码编辑框里的内容
 * */
bool Mini_C::save(){
    if (sourceFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(sourceFile);
    }
}

/*
 * 点击“另存为”
 * */
bool Mini_C::saveAs(){
    QString fileName = QFileDialog::getSaveFileName(this,tr("保存文件"),tr("./Testcase"), tr("文本文件(*.txt)"));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

/*
 * 将代码编辑框的内容写入文件
 */
bool Mini_C::saveFile(const QString &fileName){
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("提示"),
                             tr("无法写入文件 %1:\n错误原因：%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);    // 强制鼠标为等待状态

    out << codeEditor->text();         // 将代码框的内容写入文件

    /*if(syntaxTree && !syntaxTreeFile.isEmpty() && startItem){
        saveTree();
    }*/

    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);

    return true;
}


/*
 *  保存语法树
 */
bool Mini_C::saveTree(){
    if(syntaxTree){
        if(syntaxTreeFile.isEmpty()){
            QString temp = sourceFile;
            syntaxTreeFile = temp.replace(tr(".txt"), tr("_SYNTAXTREE.txt"));
        }
        if(startItem){
            QFile file(syntaxTreeFile);
            if (!file.open(QFile::WriteOnly | QFile::Text)) {
                QMessageBox::warning(this, tr("提示"),
                                     tr("无法写入文件 %1:\n错误原因：%2.")
                                     .arg(syntaxTreeFile)
                                     .arg(file.errorString()));
                return false;
            }
            QTextStream out(&file);
            out << startItem->text(0) << "\n";
            saveSyntaxTree(startItem, out);
            return true;
        }
    }
    return false;
}

/*
 *  遍历输出语法树到文件
 */
void Mini_C::saveSyntaxTree(QTreeWidgetItem *item, QTextStream &out){
    indentno += 8;

    for(int i = 0; i < item->childCount(); i++){
        for(int i = 0; i < indentno; i++){
            out << " ";
        }
        out << item->child(i)->text(0) << "\n";
        saveSyntaxTree(item->child(i), out);
    }

    indentno -= 8;
}



/*
 *  保存生成代码
 * */
bool Mini_C::saveGenCode(){
    if(genCodeFile.isEmpty()){
        QString temp = sourceFile;
        genCodeFile = temp.replace(tr(".txt"), tr("_CODE.txt"));
    }

    QFile file(genCodeFile);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("提示"),
                             tr("无法写入文件 %1:\n错误原因：%2.")
                             .arg(genCodeFile)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);    // 强制鼠标为等待状态

    out << codeGenResult->text();         // 将代码框的内容写入文件

    QApplication::restoreOverrideCursor();

    return true;
}


/*  点击“编译”
 *
 * */
void Mini_C::compile(){
    if(save()){
        init_data();
        ui->compile_textEdit->append(tr("正在编译 %1 .\n").arg(sourceFile));
        ui->tabWidget->setCurrentIndex(0);

        /* 词法分析、语法分析 */
        syntaxTree = parse();
        ui->compile_textEdit->append(tr("词法分析、语法分析结束."));
        if(!Error){
            ui->compile_textEdit->append(tr("未发现词法/语法错误.\n"));            // Error = true ---> 编译出错
            // 绘制语法树
            ui->tabWidget_2->setCurrentIndex(0);
            startItem = new QTreeWidgetItem(ui->treeWidget, QStringList(QString("start")));
            printTree(syntaxTree, startItem);
            ui->treeWidget->expandAll();
            if(syntaxTree && !syntaxTreeFile.isEmpty() && startItem){
                saveTree();
            }
        }else{
            freeSyntaxTree(syntaxTree);
            ui->treeWidget->clear();
            startItem = NULL;
        }

        /* 语义分析 */
        if(!Error){
            buildSymbolTable();
            ui->compile_textEdit->append(tr("符号检查结束."));
            if(!Error){
                ui->compile_textEdit->append(tr("未发现符号使用错误.\n"));
            }
        }
        if(!Error){
            typeCheck(syntaxTree);
            ui->compile_textEdit->append(tr("类型检查结束."));
            if(!Error){
                ui->compile_textEdit->append(tr("未发现类型错误.\n"));
            }
        }

        /* 生成代码 */
        if(!Error){
            ui->tabWidget_2->setCurrentIndex(1);
            codeGen(syntaxTree);
            ui->compile_textEdit->append(tr("代码生成结束."));
            if(!Error){
                ui->compile_textEdit->append(tr("未发现代码生成错误."));
                ui->compile_textEdit->append(tr("编译结束.\n"));
                saveGenCode();
            }
        }
    }
}

/*
 *  数据初始化/重置
 * */
void Mini_C::init_data(){
    ui->compile_textEdit->clear();
    lineno = 0;
    Error = FALSE;
    tokenString[0] = '\0';
    lineBuf.clear();
    linepos = 0;
    bufsize = 0;
    EOF_flag = FALSE;
    token = ERROR;
    freeSyntaxTree(syntaxTree);
    ui->treeWidget->clear();
    startItem = NULL;

    globalSize = 0;
    local_offset = 0;
    param_NUM = 0;
    param_offset = 0;

    initSymbolTable();
    scope_bucket.clear();
    funDec_for_returnStmt = NULL;
    last_global_declaration = NULL;
    first_compound = true;
    has_return_stmt = false;

    emitLoc = 0;
    highEmitLoc = 0;
    for(int i = 0; i < 20; i++) param_stack[i] = NULL;
    isGetValue = 1;
    isGenParam = 0;

    ui->run_textEdit->clear();
    ui->compile_textEdit->clear();
    codeGenResult->clear();
}

/*
 *   点击“运行”
 * */
void Mini_C::run(){
    compile();

    if(!Error){
        ui->tabWidget->setCurrentIndex(1);
        run_virtual_machine();
    }
}

/*
 *      运行框实时滚动
 * */
void Mini_C::moveToEnd(){
    ui->run_textEdit->moveCursor(QTextCursor::End);
}

/*
 *  点击“关于”
 */
void Mini_C::about(){
    QMessageBox::about(this, tr("关于"),
             tr("<b>Mini C </b>是一个微型的C语言的子集，具有C语言的一些基本特点。 \n"
                "<b>Mini C </b>编译器则是它的编译器，核心功能包含词法分析、语法分析、语义分析、\n"
                "生成代码以及运行代码。"
                "其他的功能包括基本的文件操作、编译信息输出、运行时的输入输出以及"
                "显示/保存语法树和生成代码。"));
}

/*
 *  修改文本编辑框为“内容已被修改”状态
 */
void Mini_C::documentWasModified(){
    setWindowModified(codeEditor->isModified());
}

/*
 *  退出系统时检查是否需要保存文件
 * */
void Mini_C::closeEvent(QCloseEvent *event){
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}


/* 一些按键  */
// 0. 新建文件
// 1. 打开文件
// 2. 保存文件
// 3. 另存为
// 4. 退出
//
// 5. 保存语法树     (2020/5/18取消了)
// 6. 保存生成代码    （取消了）
// 7. 编译
// 8. 运行
//
// 9. 关于
void Mini_C::createActions(){

    newAct = new QAction(QIcon(":/image/images/new.png"), tr("新建文件"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/image/images/open.png"), tr("打开文件"), this);
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/image/images/save.png"), tr("保存文件"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("另存为..."), this);
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    exitAct = new QAction(tr("退出"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    aboutAct = new QAction(tr("关于"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    //saveSyntaxTreeAct = new QAction(tr("保存语法树"), this);
    //connect(saveSyntaxTreeAct, SIGNAL(triggered()), this, SLOT(saveTree()));

    //saveGenCodeAct = new QAction(tr("保存生成代码"), this);
    //connect(saveGenCodeAct, SIGNAL(triggered()), this, SLOT(saveGenCode()));

    compileAct = new QAction(QIcon(":/image/images/compile.png"), tr("编译"), this);
    compileAct->setShortcut(tr("Ctrl+E"));
    connect(compileAct, SIGNAL(triggered()), this, SLOT(compile()));

    runAct = new QAction(QIcon(":/image/images/run.png"), tr("运行"), this);
    runAct->setShortcut(tr("Ctrl+R"));
    connect(runAct, SIGNAL(triggered()), this, SLOT(run()));

}

// 设置菜单
void Mini_C::createMenus(){

    fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    /*midMenu = menuBar()->addMenu(tr("中间文件"));
    midMenu->addAction(saveSyntaxTreeAct);
    midMenu->addAction(saveGenCodeAct);*/

    helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(aboutAct);
}

// 设置工具栏
void Mini_C::createToolBars(){

    fileToolBar = addToolBar(tr("文件"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);

    runToolBar = addToolBar(tr("运行"));
    runToolBar->addAction(compileAct);
    runToolBar->addAction(runAct);
}

// 代码编辑框初始化设置
void Mini_C::codeEditor_init(){

    codeEditor = new QsciScintilla();

    ui->horizontalLayout_6->addWidget(codeEditor);

    ui->splitter->setStretchFactor(0,3);
    ui->splitter->setStretchFactor(1,2);


    // 设置字体
    QFont font("consolas", 14, QFont::Normal);
    codeEditor->setFont(font);
    codeEditor->setMarginsFont(font);

    // 设置行号、行号栏
    codeEditor->setMarginType(0,QsciScintilla::NumberMargin);
    codeEditor->setMarginLineNumbers(0,true);
    QFontMetrics fontmetrics = QFontMetrics(font);
    codeEditor->setMarginWidth(0, fontmetrics.width("0000"));

    // 自动缩进
    codeEditor->setAutoIndent(true);
    codeEditor->setTabWidth(4);

    // 括号匹配
    codeEditor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

    //设置编码
    codeEditor->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);

    // 光标
    codeEditor->setCaretWidth(2);

    // 当文档内容改变的时候，修改文档的状态为“已改变”
    connect(codeEditor, SIGNAL(textChanged()),
            this, SLOT(documentWasModified()));
}

// 代码生成结果显示框初始化
void Mini_C::codeGenResult_init(){
    codeGenResult = new QsciScintilla();
    ui->horizontalLayout->addWidget(codeGenResult);

    // 设置字体
    QFont font("consolas", 12, QFont::Normal);
    codeGenResult->setFont(font);
    codeGenResult->setMarginsFont(font);

    //设置编码
    codeGenResult->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);

    //不可编辑
    codeGenResult->setReadOnly(true);

    // 行号
    codeGenResult->setMarginType(0,QsciScintilla::NumberMargin);
    codeGenResult->setMarginLineNumbers(0,true);
    QFontMetrics fontmetrics = QFontMetrics(font);
    codeGenResult->setMarginWidth(0, fontmetrics.width("0000"));
}

/*
 *      询问是否保存已经被修改的源文件
 *      若被修改且选择确定，则保存文件。
 */
bool Mini_C::maybeSave()
{
    if (codeEditor->isModified()) {
        int ret = QMessageBox::warning(this, tr("提示"),
                     tr("源文件已被修改，是否需要保存？"),
                     QMessageBox::Yes | QMessageBox::Default,
                     QMessageBox::No,
                     QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

/*
 *  设置当前文件名
 * */
void Mini_C::setCurrentFile(const QString &fileName){
    sourceFile = fileName;
    codeEditor->setModified(false);
    setWindowModified(false);

    QString shownName;
    if (sourceFile.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = QFileInfo(sourceFile).fileName();

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Mini_c 编译器")));

    QString temp1 = sourceFile, temp2 = sourceFile;
    syntaxTreeFile = temp1.replace(tr(".txt"), tr("_SYNTAXTREE.txt"));
    genCodeFile = temp2.replace(tr(".txt"), tr("_CODE.txt"));
}


/*
 *  释放语法树的存储空间
 */
void Mini_C::freeSyntaxTree(TreeNode*& tree){
    if(tree != NULL){
        for(int i = 0; i < MAXCHILDREN; i++){
            if(tree->child[i] != NULL)
                freeSyntaxTree(tree->child[i]);
        }
        delete tree;
        tree = NULL;
    }
}






