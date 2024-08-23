%code requires{
#include <stdio.h>
#include <string.h>
#include "assembler.hpp"

using namespace std;

extern FILE *yyin;
int yyerror(const char *p);
int yylex();
}

%union{
  int val;
  string* stringType;
  JumpArgument* jumpArg;
  DataArguments* dataArgs;
  DirectiveArguments* directiveArgs;
}

%token COLON PLUS LEFTBRACKET RIGHTBRACKET PERCENTAGE DOLLAR COMMA EOL
%token GLOBALDIR EXTERNDIR SECTIONDIR WORDDIR SKIPDIR ASCIIDIR EQUDIR ENDDIR
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR LD ST CSRRD CSRWR

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
%token <stringType> R14
%token <stringType> R15
%token <stringType> STATUS
%token <stringType> HANDLER
%token <stringType> CAUSE

%token <stringType> SYMBOL
%token <stringType> LITERAL

%type <directiveArgs> symbolList
%type <directiveArgs> symbolOrLiteralList
%type <jumpArg> jumpOperand
%type <dataArgs> dataOperand
%type <stringType> gpr
%type <stringType> csr
%type <stringType> reg

%%

program:
  | program statement
  ;

statement:
  EOL {}
  | label {}
  | label directive {}
  | label instruction {}
  | directive {}
  | instruction {}
  ;

label:
  SYMBOL COLON {asmLabel($1);}
  ;

directive:
  GLOBALDIR symbolList {asmGlobalDir($2);}
  | EXTERNDIR symbolList {asmExternDir($2);}
  | SECTIONDIR SYMBOL {asmSectionDir($2);}
  | WORDDIR symbolOrLiteralList {asmWordDir($2);}
  | SKIPDIR LITERAL {asmSkipDir($2);}
  | ENDDIR {asmEndDir(); return -1;}
  ;

instruction:
  HALT {asmHALT();}
  | INT {asmINT ();}
  | IRET {asmIRET();}
  | CALL jumpOperand {asmCALL($2);}
  | RET {asmRET ();}
  | JMP jumpOperand {asmJMP($2);}
  | BEQ gpr COMMA gpr COMMA jumpOperand {asmBEQ($2, $4, $6);}
  | BNE gpr COMMA gpr COMMA jumpOperand {asmBNE($2, $4, $6);}
  | BGT gpr COMMA gpr COMMA jumpOperand {asmBGT($2, $4, $6);}
  | PUSH gpr {asmPUSH($2);}
  | POP gpr {asmPOP($2);}
  | XCHG gpr COMMA gpr {asmXCHG($2, $4);}
  | ADD gpr COMMA gpr {asmADD($2, $4);}
  | SUB gpr COMMA gpr {asmSUB($2, $4);}
  | MUL gpr COMMA gpr {asmMUL($2, $4);}
  | DIV gpr COMMA gpr {asmDIV($2, $4);}
  | NOT gpr {asmNOT($2);}
  | AND gpr COMMA gpr {asmAND($2, $4);}
  | OR gpr COMMA gpr {asmOR($2, $4);}
  | XOR gpr COMMA gpr {asmXOR($2, $4);}
  | SHL gpr COMMA gpr {asmSHL($2, $4);}
  | SHR gpr COMMA gpr {asmSHR($2, $4);}
  | LD dataOperand COMMA gpr {asmLD($2, $4);}
  | ST gpr COMMA dataOperand {asmST($2, $4);}
  | CSRRD csr COMMA gpr {asmCSRRD($2, $4);}
  | CSRWR gpr COMMA csr {asmCSRWR($2, $4);}
  ;

symbolList:
  SYMBOL {$$ = new DirectiveArguments($1, OperandType::TYPE_SYMBOL);}
  | symbolList COMMA SYMBOL {$1->operand->push_back($3); $1->operandType->push_back(OperandType::TYPE_SYMBOL); $$ = $1;}
  ;

symbolOrLiteralList:
  SYMBOL {$$ = new DirectiveArguments($1, OperandType::TYPE_SYMBOL);}
  | LITERAL {$$ = new DirectiveArguments($1, OperandType::TYPE_LITERAL);}
  | symbolOrLiteralList COMMA SYMBOL {$1->operand->push_back($3); $1->operandType->push_back(OperandType::TYPE_SYMBOL); $$ = $1;}
  | symbolOrLiteralList COMMA LITERAL {$1->operand->push_back($3); $1->operandType->push_back(OperandType::TYPE_LITERAL); $$ = $1;}
  ;

jumpOperand:
  LITERAL {$$ = new JumpArgument($1, OperandType::TYPE_LITERAL);}
  | SYMBOL {$$ = new JumpArgument($1, OperandType::TYPE_SYMBOL);}
  ;

dataOperand:
  DOLLAR LITERAL {$$ = new DataArguments($2, OperandType::TYPE_LITERAL, AddressType::VALUE_LITERAL);}
  | DOLLAR SYMBOL {$$ = new DataArguments($2, OperandType::TYPE_SYMBOL, AddressType::VALUE_SYMBOL);}
  | LITERAL {$$ = new DataArguments($1, OperandType::TYPE_LITERAL, AddressType::MEM_LITERAL);}
  | SYMBOL {$$ = new DataArguments($1, OperandType::TYPE_SYMBOL, AddressType::MEM_SYMBOL);}
  | reg {$$ = new DataArguments($1, OperandType::REG, AddressType::VALUE_REG);}
  | LEFTBRACKET reg RIGHTBRACKET {$$ = new DataArguments($2, OperandType::REG, AddressType::MEM_REG);}
  | LEFTBRACKET reg PLUS LITERAL RIGHTBRACKET {$$ = new DataArguments($2, OperandType::REG, AddressType::MEM_REG_LITERAL); $$->operand->push_back($4); $$->operandType->push_back(OperandType::TYPE_LITERAL);}
  | LEFTBRACKET reg PLUS SYMBOL RIGHTBRACKET {$$ = new DataArguments($2, OperandType::REG, AddressType::MEM_REG_SYMBOL); $$->operand->push_back($4); $$->operandType->push_back(OperandType::TYPE_SYMBOL);}
  ;

gpr:
  PERCENTAGE R0 {$$ = $2;}
  | PERCENTAGE R1 {$$ = $2;}
  | PERCENTAGE R2 {$$ = $2;}
  | PERCENTAGE R3 {$$ = $2;}
  | PERCENTAGE R4 {$$ = $2;}
  | PERCENTAGE R5 {$$ = $2;}
  | PERCENTAGE R6 {$$ = $2;}
  | PERCENTAGE R7 {$$ = $2;}
  | PERCENTAGE R8 {$$ = $2;}
  | PERCENTAGE R9 {$$ = $2;}
  | PERCENTAGE R10 {$$ = $2;}
  | PERCENTAGE R11 {$$ = $2;}
  | PERCENTAGE R12 {$$ = $2;}
  | PERCENTAGE R13 {$$ = $2;}
  | PERCENTAGE R14 {$$ = $2;}
  | PERCENTAGE R15 {$$ = $2;}
  ;

csr:
  STATUS {$$ = $1;}
  | HANDLER {$$ = $1;}
  | CAUSE {$$ = $1;}
  ;

reg:
  gpr
  | csr
  ;

  %%

  int start_parser(int argc, char **argv) {
  FILE * fp;
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