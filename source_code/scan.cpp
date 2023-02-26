/******************************************************
*	文件名:		scan.cpp
*	创建者：      	BEAR
*	创建日期：	2020/4/9
*	最后修改：	2020/4/10
*******************************************************
*	内容描述：
*		scan.cpp是词法分析模块，其中最主要的是
*		getToken函数，它的作用是通过扫描识别一个单词并
*		返回该单词的TokenType。此外是一些辅助函数。
*		词法分析DFA图放在doc文件里面。
*******************************************************
*	修改记录：
*	2020/4/10  初步完成词法分析。
*			   发现Tiny词法分析程序的一处缺陷：
*			   在注释未闭合的情况下扫描到EOF，不会报出错误（假设前面的语句正确），如： a:=1 { assign ....
*
*			   虽然这和前面的所有语句无关，但仍是一个错误，造成这个错误的原因是:
*			   前面赋值语句语法分析的最后是match(ID), 然后getToken()进行继续扫描，此后再也没有match(TokenType)语句了，
*			   注释部分不会返回TokenType, 最后扫到EOF返回值为ENDFILE的TokenType，而stmt-sequence()函数中ENDFILE是while
*			   循环的终结条件之一，到此为止语法分析部分结束了，但没有报错。
*
*			   这种情况在VS2017中报以下错误：
*				fatal error C1071: unexpected end of file found in comment. (在注释中遇到意外的文件结束。)
*			   在MiniC中也会输出这个错误提示， 并设置 Error为true，不再继续语义分析。
*
*	2020/4/11  修复两处bug：未处理逗号；关于注释的bug.
*
*   2020/4/20   完成词法分析测试。添加isDigit()、isAlpha()函数以替代系统函数，因为系统接口不支持中文字符，
*				接受中文字符时会导致系统崩溃。
*
*   2020/4/25   将文件迁移至Qt平台
*******************************************************/


#include "mini_c.h"
#include "ui_mini_c.h"


/*
 *	 从源代码中获取下一个字符, 是Tiny程序源代码
 */
int Mini_C::getNextChar() {
    if (!(linepos < bufsize))
    {
        QString line = codeEditor->text(lineno++);
        if(line.isNull() != true){      // 读取下一行
            lineBuf = line;
            bufsize = line.size();
            linepos = 0;
            return lineBuf.at(linepos++).unicode();
        }

        else	// 文件已读完
        {
            EOF_flag = TRUE;
            return EOF;
        }
    }
    else return lineBuf.at(linepos++).unicode();		// 读取当前行的下一个字符
}


/*
*   从LineBuf中回退一个字符。是Tiny程序源代码。
*   由于“超前查看”的扫描机制，某些情况需要回退一个字符，
*	 例如在INNUM状态中，需要扫到一个非数字字符才跳出此状态，
*	 但多扫描的一个字符不能抛弃，因此需要回退。
*/
void Mini_C::ungetNextChar() {
    if (!EOF_flag) linepos--;
}

/*
 *	  判断字符是否是数字/字母。替代系统的函数，因为系统的函数接受非ascii字符会报错。
 */
bool Mini_C::isDigit(int ch) {
    return ch >= '0' && ch <= '9';
}
bool Mini_C::isAlpha(int ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/*
*  基于DFA图，扫描源代码、识别出每一个单词并返回它对应的TokenType。
*  是词法分析部分最重要的函数。
*/
TokenType Mini_C::getToken() {
    int tokenStringIndex = 0;		// tokenString 尾部下标
    TokenType currentToken;			// 扫描成功的TokenType, 将作为返回值
    StateType state = START;		// DFA起始状态
    int save;						// 当前扫描到的字符是否要存入tokenString

    while (state != DONE) {
        int c = (int)(getNextChar());
        save = TRUE;

        switch (state) {
        case START:
            if (isDigit(c))
                state = INNUM;
            else if (isAlpha(c))
                state = INID;
            else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                save = FALSE;
            else if (c == '=')
                state = INEQ;
            else if (c == '!')
                state = INNOTEQ;
            else if (c == '<')
                state = INLT;
            else if (c == '>')
                state = INGT;
            else if (c == '/')
                state = IN_COMMENT_OR_DIV;
            else {
                state = DONE;
                switch (c) {
                case EOF:
                    save = FALSE;
                    currentToken = ENDFILE;
                    break;
                case ',':
                    currentToken = COMMA;
                    break;
                case ';':
                    currentToken = SEMI;
                    break;
                case '(':
                    currentToken = LPAREN;
                    break;
                case ')':
                    currentToken = RPAREN;
                    break;
                case '[':
                    currentToken = LBRACKET;
                    break;
                case ']':
                    currentToken = RBRACKET;
                    break;
                case '{':
                    currentToken = LBRACE;
                    break;
                case '}':
                    currentToken = RBRACE;
                    break;
                case '+':
                    currentToken = PLUS;
                    break;
                case '-':
                    currentToken = MINUS;
                    break;
                case '*':
                    currentToken = TIMES;
                    break;
                default:
                    currentToken = ERROR;
                    break;
                } // switch(c)
            } // else
            break;	/* case START */

        case INNUM:
            if (!isDigit(c)) {
                state = DONE;
                ungetNextChar();
                save = FALSE;
                currentToken = NUM;
            }
            break;	/* case INNUM */

        case INID:
            if (!isAlpha(c)) {
                state = DONE;
                ungetNextChar();
                save = FALSE;
                currentToken = ID;
            }
            break;	/* case INID */

        case INEQ:
            state = DONE;
            if (c == '=') {
                currentToken = EQ;
            }else {
                ungetNextChar();
                save = FALSE;
                currentToken = ASSIGN;
            }
            break; /* case INEQ */

        case INNOTEQ:
            state = DONE;
            if (c == '=') {
                currentToken = NOTEQ;
            }else {
                currentToken = ERROR;
                ungetNextChar();
                save = FALSE;
            }
            break;	/* case INNOTEQ */

        case IN_COMMENT_OR_DIV:
            save = FALSE;
            if (c == '*') {
                state = INCOMMENT_L;
                tokenStringIndex -= 1;		/* 去掉tokenString里的 ‘/’ */
            }else {
                ungetNextChar();
                state = DONE;
                currentToken = DIV;
            }
            break;	/* case IN_COMMENT_OR_DIV */

        case INCOMMENT_L:
            save = FALSE;
            if (c == '*') {
                state = INCOMMENT_R;
            }else if (c == EOF) {		//
                state = DONE;
                currentToken = ENDFILE;
                ui->compile_textEdit->insertPlainText(
                            tr(">>> Syntax error at line %1: unexpected end of file found in comment.\n").arg(lineno));
                Error = TRUE;
            }// else do nothing
            break;		/* case INCOMMENT_L */

        case INCOMMENT_R:
            save = FALSE;
            if (c == '*') {
                //do nothing
            }else if (c == '/') {
                state = START;
            }else if (c == EOF) {
                state = DONE;
                currentToken = ENDFILE;
                ui->compile_textEdit->insertPlainText(
                            tr(">>> Syntax error at line %1: unexpected end of file found in comment.\n").arg(lineno));
                Error = TRUE;
            }else {
                state = INCOMMENT_L;
            }
            break;		/* case INCOMMENT_R */

        case INLT:
            state = DONE;
            if (c == '=') {
                currentToken = LTEQ;
            }else {
                currentToken = LT;
                ungetNextChar();
                save = FALSE;
            }
            break;		/* case INLT */

        case INGT:
            state = DONE;
            if (c == '=') {
                currentToken = GTEQ;
            }else {
                currentToken = GT;
                ungetNextChar();
                save = FALSE;
            }
            break;

        case DONE:

        default: /* should never happen */
            ui->compile_textEdit->insertPlainText(tr("Scanner Bug: state= %1\n").arg(state));
            state = DONE;
            currentToken = ERROR;
            break;
        }	/* switch(state) */

        if ((save) && (tokenStringIndex <= MAXTOKENLEN))
            tokenString[tokenStringIndex++] = (char)c;		// 保存当前扫到的字符
        if (state == DONE){
            tokenString[tokenStringIndex] = '\0';
            if (currentToken == ID)
                currentToken = reservedLookup(tokenString);		// 查看当前扫到的标识符是否是关键字
        }
    } /* while(state != DONE) */

    return currentToken;
}
