%code requires {
int yylex(); 

#include <stdio.h>
#include <string.h>
using namespace std;

extern FILE *yyin;
int yyerror(const char *p);
int yylex();
}

%token STAR COLON PLUS LEFTBRACKET RIGHTBRACKET PERCENTAGE DOLLAR COMMA EOL
%token GLOBALDIR EXTERNDIR SECTIONDIR WORDDIR SKIPDIR ASCIIDIR EQUDIR ENDDIR
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST

%token <stringType> R0 
%token <stringType> R1 
%token <stringType> R2 
%token <stringType> R3 
%token <stringType> R4
%token <stringType> R5
%token <stringType> R6
%token <stringType> R7
%token <stringType> R8 
%token <stringType> R9 
%token <stringType> R10 
%token <stringType> R11
%token <stringType> R12
%token <stringType> R13
%token <stringType> SP
%token <stringType> PC
%token <stringType> STATUS
%token <stringType> HANDLER
%token <stringType> CAUSE

%token <stringType> SYMBOL
%token <stringType> LITERAL

%type <stringVector> symbolList
%type <stringVector> symbolOrLiteralList
%type <stringType> jumpOperand
%type <stringVector> dataOperand
%type <stringType> gpr
%type <stringType> csr
%type <stringType> reg

program:
  | program statement EOL
  ;

statement:
  label 

  | label directive {}
  | label instruction {}
  | directive {}
  | instruction {}
  ;

label:
  SYMBOL COLON {}
  ;

directive:
  GLOBALDIR symbolList {}
  | EXTERNDIR symbolList {}
  | SECTIONDIR SYMBOL {}
  | WORDDIR symbolOrLiteralList {}
  | SKIPDIR LITERAL {}
  | ENDDIR {return -1;}
  ;

instruction:
  HALT {}
  | INT {}
  | IRET {}
  | CALL jumpOperand {}
  | RET {}
  | JMP jumpOperand {}
  | BEQ gpr COMMA gpr COMMA jumpOperand {}
  | BNE gpr COMMA gpr COMMA jumpOperand {}
  | BGT gpr COMMA gpr COMMA jumpOperand {}
  | PUSH gpr {}
  | POP gpr {}
  | XCHG gpr COMMA gpr {}
  | ADD gpr COMMA gpr {}
  | SUB gpr COMMA gpr {}
  | MUL gpr COMMA gpr {}
  | DIV gpr COMMA gpr {}
  | NOT gpr {}
  | AND gpr COMMA gpr {}
  | OR gpr COMMA gpr {}
  | XOR gpr COMMA gpr {}
  | SHL gpr COMMA gpr {}
  | SHR gpr COMMA gpr {}
  | LD dataOperand COMMA gpr {}
  | ST gpr COMMA dataOperand {}
  | CSRRD csr COMMA gpr {}
  | CSRWR gpr COMMA csr {}
  ;

symbolList:
  SYMBOL {}
  | symbolList COMMA SYMBOL {}
  ;

symbolOrLiteralList:
  SYMBOL {}
  | LITERAL {}
  | symbolOrLiteralList COMMA SYMBOL {}
  | symbolOrLiteralList COMMA LITERAL {}
  ;

jumpOperand:
  LITERAL {}
  | SYMBOL {}
  ;

dataOperand:
  DOLLAR LITERAL {}
  | DOLLAR SYMBOL {}
  | LITERAL {}
  | SYMBOL {}
  | reg {}
  | LEFTBRACKET reg RIGHTBRACKET
  | LEFTBRACKET reg PLUS LITERAL RIGHTBRACKET
  | LEFTBRACKET reg PLUS SYMBOL RIGHTBRACKET
  ;

gpr:
  PERCENTAGE R0 {}
  | PERCENTAGE R1 {}
  | PERCENTAGE R2 {}
  | PERCENTAGE R3 {}
  | PERCENTAGE R4 {}
  | PERCENTAGE R5 {}
  | PERCENTAGE R6 {}
  | PERCENTAGE R7 {}
  | PERCENTAGE R8 {}
  | PERCENTAGE R9 {}
  | PERCENTAGE R10 {}
  | PERCENTAGE R11 {}
  | PERCENTAGE R12 {}
  | PERCENTAGE R13 {}
  | PERCENTAGE SP {}
  | PERCENTAGE PC {}
  ;

csr:
  PERCENTAGE STATUS {}
  | PERCENTAGE HANDLER {}
  | PERCENTAGE CAUSE {}
  ;

reg:
  gpr
  | csr
  ;

int parser(int argc, char **argv) {
  FILE * fp
  const char* filename = (argc == 2) ? argv[1] : (argc == 4) ? argv[3] : NULL;
  if (filename) {
    fp = fopen(filename, "r");
  }

  if(fp == NULL){
    perror("Failed to open file.");
    return -1;
  }
  else{
    yyin = fp;
  }

  while(yyparse() != -1){}

  if(fp!=NULL){
    fclose(fp);
  }

  return 0;

}

int yyerror(const char *p) {
    fprintf(stderr, "Error: %s\n", p);
    return 0;
}