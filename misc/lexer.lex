%{
#include "parser.tab.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

%}

%option noyywrap

%%
\* {return STAR;}
":" {return COLON;}
"+" {return PLUS;}
"[" {return LEFTBRACKET;}
"]" {return RIGHTBRACKET;}
"%" {return PERCENTAGE;}
"$" {return DOLLAR;}
, {return COMMA;}

#.* {}

\.[gG][lL][oO][bB][aA][lL] {return GLOBALDIR;}
\.[eE][xX][tT][eE][rR][nN] {return EXTERNDIR;}
\.[sS][eE][cC][tT][iI][oO][nN] {return SECTIONDIR;}
\.[wW][oO][rR][dD] {return WORDDIR;}
\.[sS][kK][iI][pP] {return SKIPDIR;}
\.[aA][sS][cC][iI][iI] {return ASCIIDIR}
\.[eE][qQ][uU] {return EQUDIR}
\.[eE][nN][dD] {return ENDDIR;}

[hH][aA][lL][tT] {return HALT;}
[iI][nN][tT] {return INT;}
[iI][rR][eE][tT] {return IRET;}
[cC][aA][lL][lL] {return CALL;}
[rR][eE][tT] {return RET;}
[jJ][mM][pP] {return JMP;}
[bB][eE][qQ] {return BEQ;}
[bB][nN][eE] {return BNE;}
[bB][gG][tT] {return BGT;}
[pP][uU][sS][hH] {return PUSH;}
[pP][oO][pP] {return POP;}
[xX][cC][hH][gG] {return XCHG;}
[aA][dD][dD] {return ADD;}
[sS][uU][bB] {return SUB;}
[mM][uU][lL] {return MUL;}
[dD][iI][vV] {return DIV;}
[nN][oO][tT] {return NOT;}
[aA][nN][dD] {return AND;}
[oO][rR] {return OR;}
[xX][oO][rR] {return XOR;}
[sS][hH][lL] {return SHL;}
[sS][hH][rR] {return SHR;}
[lL][dD] {return LD;}
[sS][tT] {return ST;}

[rR]0 {yylval.stringType = new std::string("R0"); return R0;}
[rR]1 {yylval.stringType = new std::string("R1"); return R1;}
[rR]2 {yylval.stringType = new std::string("R2"); return R2;}
[rR]3 {yylval.stringType = new std::string("R3"); return R3;}
[rR]4 {yylval.stringType = new std::string("R4"); return R4;}
[rR]5 {yylval.stringType = new std::string("R5"); return R5;}
[rR]6 {yylval.stringType = new std::string("R6"); return R6;}
[rR]7 {yylval.stringType = new std::string("R7"); return R7;}
[rR]8 {yylval.stringType = new std::string("R8"); return R8;}
[rR]9 {yylval.stringType = new std::string("R9"); return R9;}
[rR]10 {yylval.stringType = new std::string("R10"); return R10;}
[rR]11 {yylval.stringType = new std::string("R11"); return R11;}
[rR]12 {yylval.stringType = new std::string("R12"); return R12;}
[rR]13 {yylval.stringType = new std::string("R13"); return R13;}
([rR]14|[sS][pP]) {yylval.stringType = new std::string("SP"); return SP;}
([rR]15|[pP][cC]) {yylval.stringType = new std::string("PC"); return PC;}
[sS][tT][aA][tT][uU][sS] {yylval.stringType = new std::string("STATUS"); return STATUS;}
[hH][aA][nN][dD][lL][eE][rR] {yylval.stringType = new std::string("HANDLER"); return HANDLER;}
[cC][aA][uU][sS][eE] {yylval.stringType = new std::string("CAUSE"); return CAUSE;}

(([-]?[0-9]*\.[0-9]+)|([-]?[0-9]+)|([-]?0x[0-9a-fA-F]+)|([-]?0[0-7]+)) {yylval.stringType = new std::string(yytext); return LITERAL;}
[a-zA-Z][a-zA-Z0-9_]* {yylval.stringType = new std::string(yytext); return SYMBOL;}

[ \t] {}
\n {return EOL;}
. {}
%%