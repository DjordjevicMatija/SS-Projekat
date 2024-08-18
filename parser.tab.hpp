/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_PARSER_TAB_HPP_INCLUDED
# define YY_YY_PARSER_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "misc/parser.y"

#include <stdio.h>
#include <string.h>
#include "assembler.hpp"

using namespace std;

extern FILE *yyin;
int yyerror(const char *p);
int yylex();

#line 60 "parser.tab.hpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    STAR = 258,
    COLON = 259,
    PLUS = 260,
    LEFTBRACKET = 261,
    RIGHTBRACKET = 262,
    PERCENTAGE = 263,
    DOLLAR = 264,
    COMMA = 265,
    EOL = 266,
    GLOBALDIR = 267,
    EXTERNDIR = 268,
    SECTIONDIR = 269,
    WORDDIR = 270,
    SKIPDIR = 271,
    ASCIIDIR = 272,
    EQUDIR = 273,
    ENDDIR = 274,
    HALT = 275,
    INT = 276,
    IRET = 277,
    CALL = 278,
    RET = 279,
    JMP = 280,
    BEQ = 281,
    BNE = 282,
    BGT = 283,
    PUSH = 284,
    POP = 285,
    XCHG = 286,
    ADD = 287,
    SUB = 288,
    MUL = 289,
    DIV = 290,
    NOT = 291,
    AND = 292,
    OR = 293,
    XOR = 294,
    SHL = 295,
    SHR = 296,
    LD = 297,
    ST = 298,
    CSRRD = 299,
    CSRWR = 300,
    R0 = 301,
    R1 = 302,
    R2 = 303,
    R3 = 304,
    R4 = 305,
    R5 = 306,
    R6 = 307,
    R7 = 308,
    R8 = 309,
    R9 = 310,
    R10 = 311,
    R11 = 312,
    R12 = 313,
    R13 = 314,
    SP = 315,
    PC = 316,
    STATUS = 317,
    HANDLER = 318,
    CAUSE = 319,
    SYMBOL = 320,
    LITERAL = 321
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 13 "misc/parser.y"

  int val;
  string* stringType;
  JumpArgument* jumpArg;
  DataArguments* dataArgs;
  DirectiveArguments* directiveArgs;

#line 146 "parser.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_TAB_HPP_INCLUDED  */
