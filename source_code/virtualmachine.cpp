/*****************************************************
*	文件名:		virtualmachine.cpp
*	创建者：        BEAR
*	创建日期：	2020/5/19
*	最后修改：	2020/5/19
******************************************************
*	内容描述：
*		virtualmachine.cpp 实现了虚拟机相关的函数。
*       Mini C代码生成使用的是TM机的指令集， 在命令行模式下，
*       生成的文件无需做任何修改就可以在TM.C程序上运行。
*       但是现在是在Qt平台，所以需要少许适应性的修改并去掉一些多余的部分。
******************************************************
*	修改记录：
*
*******************************************************/

#include "mini_c.h"
#include "ui_mini_c.h"

#include <QFile>
#include <QMessageBox>
#include <QInputDialog>

/*
 *  初始化虚拟机
 */
void Mini_C::init_virtual_machine(){
    for (int regNo = 0; regNo < NO_REGS; regNo++){
        reg[regNo] = 0;
    }

    dMem[0] = DADDR_SIZE - 1;
    for (int loc = 1; loc < DADDR_SIZE; loc++){
        dMem[loc] = 0;
    }

    for (int loc = 0; loc < IADDR_SIZE; loc++){
        iMem[loc].iop = opHALT;
        iMem[loc].iarg1 = 0;
        iMem[loc].iarg2 = 0;
        iMem[loc].iarg3 = 0;
    }

    in_Line[0] = '\0';
    lineLen = 0;
    inCol = 0;
    num = 0;
    word[0] = '\0';
    ch = '\0';
    done = 0;
}

/*
 *  获取指令类型
 */
int Mini_C::opClass(int c){
    if (c <= opRRLim) {
        return (opclRR);
    }
    else if (c <= opRMLim) {
        return (opclRM);
    }
    else{
        return (opclRA);
    }
}

/*
 *  读取字符
 */
void Mini_C::getCh(){
    if (++inCol < lineLen){
        ch = in_Line[inCol];
    }
    else ch = ' ';
}

/*
 * 跳过空格
 */
int Mini_C::nonBlank(){
    while ((inCol < lineLen) && (in_Line[inCol] == ' ')){
        inCol++;
    }
    if (inCol < lineLen){
        ch = in_Line[inCol];
        return TRUE;
    }
    else{
        ch = ' ';
        return FALSE;
    }
}

/*
 * 读取整数
 * */
int Mini_C::getNum(){
    int sign;
    int term;
    int temp = FALSE;
    num = 0;
    do{
        sign = 1;
        while (nonBlank() && ((ch == '+') || (ch == '-'))){
            temp = FALSE;
            if (ch == '-')  sign = -sign;
            getCh();
        }
        term = 0;
        nonBlank();
        while (isdigit(ch)){
            temp = TRUE;
            term = term * 10 + (ch - '0');
            getCh();
        }
        num = num + (term * sign);
    } while ((nonBlank()) && ((ch == '+') || (ch == '-')));
    return temp;
}

/*
 *  读取操作码
 * */
int Mini_C::getWord(){
    int temp = FALSE;
    int length = 0;
    if (nonBlank()){
        while (isalnum(ch)){
            if (length < WORDSIZE - 1) word[length++] = ch;
            getCh();
        }
        word[length] = '\0';
        temp = (length != 0);
    }
    return temp;
}

/*
 *  跳过字符
 * */
int Mini_C::skipCh(char c){
    int temp = FALSE;
    if (nonBlank() && (ch == c)){
        getCh();
        temp = TRUE;
    }
    return temp;
}

/*
 * 输出指令错误
 * */
int Mini_C::run_error(char * msg, int lineNo, int instNo){

    ui->run_textEdit->insertPlainText(tr("Line %1").arg(lineNo));
    if(instNo >= 0){
        ui->run_textEdit->insertPlainText(tr(" (Instruction %1)").arg(instNo));
    }
    ui->run_textEdit->insertPlainText(tr("   %1\n").arg(msg));

    return FALSE;
}

/*
 *  从文件读取指令
 * */
int Mini_C::readInstructions(){
    init_virtual_machine();

    int op;
    int arg1, arg2, arg3;
    int loc, lineNo;

    lineNo = 0;

    QFile file(genCodeFile);

    // 打开文件失败
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        QMessageBox::warning(this, tr("提示"),
                             tr("无法读取文件 %1:\n错误原因：%2.")
                             .arg(genCodeFile)
                             .arg(file.errorString()));
        return FALSE;
    }

    while(!file.atEnd()){
        file.readLine(in_Line, LINESIZE - 2);   // 读取行
        inCol = 0;
        lineNo++;
        lineLen = strlen(in_Line) - 1;
        if (in_Line[lineLen] == '\n') in_Line[lineLen] = '\0';
        else in_Line[++lineLen] = '\0';
        if ((nonBlank()) && (in_Line[inCol] != '*'))
        {
            if (!getNum())
                return run_error("Bad location", lineNo, -1);
            loc = num;
            if (loc > IADDR_SIZE)
                return run_error("Location too large", lineNo, loc);
            if (!skipCh(':'))
                return run_error("Missing colon", lineNo, loc);
            if (!getWord())
                return run_error("Missing opcode", lineNo, loc);
            op = 0;
            while ((op < opRALim)
                && (strncmp(opCodeTab[op], word, 4) != 0))
                op++;
            if (strncmp(opCodeTab[op], word, 4) != 0)
                return run_error("Illegal opcode", lineNo, loc);
            switch (opClass(op))
            {
            case opclRR:
                /***********************************/
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return run_error("Bad first register", lineNo, loc);
                arg1 = num;
                if (!skipCh(','))
                    return run_error("Missing comma", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return run_error("Bad second register", lineNo, loc);
                arg2 = num;
                if (!skipCh(','))
                    return run_error("Missing comma", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return run_error("Bad third register", lineNo, loc);
                arg3 = num;
                break;

            case opclRM:
            case opclRA:
                /***********************************/
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return run_error("Bad first register", lineNo, loc);
                arg1 = num;
                if (!skipCh(','))
                    return run_error("Missing comma", lineNo, loc);
                if (!getNum())
                    return run_error("Bad displacement", lineNo, loc);
                arg2 = num;
                if (!skipCh('(') && !skipCh(','))
                    return run_error("Missing LParen", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return run_error("Bad second register", lineNo, loc);
                arg3 = num;
                break;
            }
            iMem[loc].iop = op;
            iMem[loc].iarg1 = arg1;
            iMem[loc].iarg2 = arg2;
            iMem[loc].iarg3 = arg3;
        }
    }

    return TRUE;
}

/*
 *  执行一条指令
 *
 *  这里只修改输入输出方式（IN, OUT指令）和结束方式HALT就可以了
 * */
STEPRESULT Mini_C::stepTM(){
    INSTRUCTION currentinstruction;
    int pc;
    int r, s, t, m;
    bool ok;
    int get_num;

    pc = reg[PC_REG];
    if ((pc < 0) || (pc > IADDR_SIZE))
        return srIMEM_ERR;
    reg[PC_REG] = pc + 1;
    currentinstruction = iMem[pc];
    switch (opClass(currentinstruction.iop))
    {
    case opclRR:
        /***********************************/
        r = currentinstruction.iarg1;
        s = currentinstruction.iarg2;
        t = currentinstruction.iarg3;
        break;

    case opclRM:
        /***********************************/
        r = currentinstruction.iarg1;
        s = currentinstruction.iarg3;
        m = currentinstruction.iarg2 + reg[s];
        if ((m < 0) || (m > DADDR_SIZE))
            return srDMEM_ERR;
        break;

    case opclRA:
        /***********************************/
        r = currentinstruction.iarg1;
        s = currentinstruction.iarg3;
        m = currentinstruction.iarg2 + reg[s];
        break;
    } /* case */

    switch (currentinstruction.iop)
    { /* RR instructions */
    case opHALT:
        /***********************************/
        //printf("HALT: %1d,%1d,%1d\n", r, s, t);
        ui->run_textEdit->insertPlainText(tr("\n-------END-------\n"));
        return srHALT;
        /* break; */

    case opIN:
        /***********************************/
        /*do
        {
            printf("Enter value for IN instruction: ");
            fflush(stdin);
            fflush(stdout);
            gets(in_Line);
            lineLen = strlen(in_Line);
            inCol = 0;
            ok = getNum();
            if (!ok) printf("Illegal value\n");
            else reg[r] = num;
        } while (!ok);*/

        do{
            get_num = QInputDialog::getInt(this, "提示", "请输入一个整数", 0, INT_MIN, INT_MAX, 1, &ok);
            if(ok){
                reg[r] = get_num;
                ui->run_textEdit->insertPlainText(tr("input: %1\n").arg(get_num));
                break;
            }
        }while(!ok);

        break;

    case opOUT:
        /*printf("OUT instruction prints: %d\n", reg[r]);*/
        ui->run_textEdit->insertPlainText(tr("output: %1\n").arg(reg[r]));
        break;
    case opADD:  reg[r] = reg[s] + reg[t];  break;
    case opSUB:  reg[r] = reg[s] - reg[t];  break;
    case opMUL:  reg[r] = reg[s] * reg[t];  break;

    case opDIV:
        /***********************************/
        if (reg[t] != 0) reg[r] = reg[s] / reg[t];
        else return srZERODIVIDE;
        break;

        /*************** RM instructions ********************/
    case opLD:    reg[r] = dMem[m];  break;
    case opST:    dMem[m] = reg[r];  break;

        /*************** RA instructions ********************/
    case opLDA:    reg[r] = m; break;
    case opLDC:    reg[r] = currentinstruction.iarg2;   break;
    case opJLT:    if (reg[r] < 0) reg[PC_REG] = m; break;
    case opJLE:    if (reg[r] <= 0) reg[PC_REG] = m; break;
    case opJGT:    if (reg[r] > 0) reg[PC_REG] = m; break;
    case opJGE:    if (reg[r] >= 0) reg[PC_REG] = m; break;
    case opJEQ:    if (reg[r] == 0) reg[PC_REG] = m; break;
    case opJNE:    if (reg[r] != 0) reg[PC_REG] = m; break;

        /* end of legal instructions */
    } /* case */
    return srOKAY;
}


/* ---------------------------------------------------------------------------------- */

/*
 *  运行虚拟机，功能模块入口函数
 */
void Mini_C::run_virtual_machine(){
    if(readInstructions()){
        int stepResult = srOKAY;
        ui->run_textEdit->insertPlainText(tr("-------START-------\n\n"));
        while (stepResult == srOKAY){
            stepResult = stepTM();
        }
    }
}



