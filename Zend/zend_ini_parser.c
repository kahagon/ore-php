
/*  A Bison parser, made from /home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse ini_parse
#define yylex ini_lex
#define yyerror ini_error
#define yylval ini_lval
#define yychar ini_char
#define yydebug ini_debug
#define yynerrs ini_nerrs
#define	TC_SECTION	257
#define	TC_RAW	258
#define	TC_CONSTANT	259
#define	TC_NUMBER	260
#define	TC_STRING	261
#define	TC_WHITESPACE	262
#define	TC_LABEL	263
#define	TC_OFFSET	264
#define	TC_DOLLAR_CURLY	265
#define	TC_VARNAME	266
#define	TC_QUOTED_STRING	267
#define	BOOL_TRUE	268
#define	BOOL_FALSE	269
#define	END_OF_LINE	270

#line 1 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"

/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2012 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   |          Jani Taskinen <jani@php.net>                                |
   +----------------------------------------------------------------------+
*/

/* $Id: zend_ini_parser.y 321634 2012-01-01 13:15:04Z felipe $ */

#define DEBUG_CFG_PARSER 0

#include "zend.h"
#include "zend_API.h"
#include "zend_ini.h"
#include "zend_constants.h"
#include "zend_ini_scanner.h"
#include "zend_extensions.h"

#define YYERROR_VERBOSE
#define YYSTYPE zval

#ifdef ZTS
#define YYPARSE_PARAM tsrm_ls
#define YYLEX_PARAM tsrm_ls
int ini_parse(void *arg);
#else
int ini_parse(void);
#endif

#define ZEND_INI_PARSER_CB	(CG(ini_parser_param))->ini_parser_cb
#define ZEND_INI_PARSER_ARG	(CG(ini_parser_param))->arg

/* {{{ zend_ini_do_op()
*/
static void zend_ini_do_op(char type, zval *result, zval *op1, zval *op2)
{
	int i_result;
	int i_op1, i_op2;
	char str_result[MAX_LENGTH_OF_LONG];

	i_op1 = atoi(Z_STRVAL_P(op1));
	free(Z_STRVAL_P(op1));
	if (op2) {
		i_op2 = atoi(Z_STRVAL_P(op2));
		free(Z_STRVAL_P(op2));
	} else {
		i_op2 = 0;
	}

	switch (type) {
		case '|':
			i_result = i_op1 | i_op2;
			break;
		case '&':
			i_result = i_op1 & i_op2;
			break;
		case '~':
			i_result = ~i_op1;
			break;
		case '!':
			i_result = !i_op1;
			break;
		default:
			i_result = 0;
			break;
	}

	Z_STRLEN_P(result) = zend_sprintf(str_result, "%d", i_result);
	Z_STRVAL_P(result) = (char *) malloc(Z_STRLEN_P(result)+1);
	memcpy(Z_STRVAL_P(result), str_result, Z_STRLEN_P(result));
	Z_STRVAL_P(result)[Z_STRLEN_P(result)] = 0;
	Z_TYPE_P(result) = IS_STRING;
}
/* }}} */

/* {{{ zend_ini_init_string()
*/
static void zend_ini_init_string(zval *result)
{
	Z_STRVAL_P(result) = malloc(1);
	Z_STRVAL_P(result)[0] = 0;
	Z_STRLEN_P(result) = 0;
	Z_TYPE_P(result) = IS_STRING;
}
/* }}} */

/* {{{ zend_ini_add_string()
*/
static void zend_ini_add_string(zval *result, zval *op1, zval *op2)
{
	int length = Z_STRLEN_P(op1) + Z_STRLEN_P(op2);

	Z_STRVAL_P(result) = (char *) realloc(Z_STRVAL_P(op1), length+1);
	memcpy(Z_STRVAL_P(result)+Z_STRLEN_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op2));
	Z_STRVAL_P(result)[length] = 0;
	Z_STRLEN_P(result) = length;
	Z_TYPE_P(result) = IS_STRING;
}
/* }}} */

/* {{{ zend_ini_get_constant()
*/
static void zend_ini_get_constant(zval *result, zval *name TSRMLS_DC)
{
	zval z_constant;

	/* If name contains ':' it is not a constant. Bug #26893. */
	if (!memchr(Z_STRVAL_P(name), ':', Z_STRLEN_P(name))
		   	&& zend_get_constant(Z_STRVAL_P(name), Z_STRLEN_P(name), &z_constant TSRMLS_CC)) {
		/* z_constant is emalloc()'d */
		convert_to_string(&z_constant);
		Z_STRVAL_P(result) = zend_strndup(Z_STRVAL(z_constant), Z_STRLEN(z_constant));
		Z_STRLEN_P(result) = Z_STRLEN(z_constant);
		Z_TYPE_P(result) = Z_TYPE(z_constant);
		zval_dtor(&z_constant);
		free(Z_STRVAL_P(name));
	} else {
		*result = *name;
	}
}
/* }}} */

/* {{{ zend_ini_get_var()
*/
static void zend_ini_get_var(zval *result, zval *name TSRMLS_DC)
{
	zval curval;
	char *envvar;

	/* Fetch configuration option value */
	if (zend_get_configuration_directive(Z_STRVAL_P(name), Z_STRLEN_P(name)+1, &curval) == SUCCESS) {
		Z_STRVAL_P(result) = zend_strndup(Z_STRVAL(curval), Z_STRLEN(curval));
		Z_STRLEN_P(result) = Z_STRLEN(curval);
	/* ..or if not found, try ENV */
	} else if ((envvar = zend_getenv(Z_STRVAL_P(name), Z_STRLEN_P(name) TSRMLS_CC)) != NULL ||
			   (envvar = getenv(Z_STRVAL_P(name))) != NULL) {
		Z_STRVAL_P(result) = strdup(envvar);
		Z_STRLEN_P(result) = strlen(envvar);
	} else {
		zend_ini_init_string(result);
	}
}
/* }}} */

/* {{{ ini_error()
*/
static void ini_error(char *msg)
{
	char *error_buf;
	int error_buf_len;
	char *currently_parsed_filename;
	TSRMLS_FETCH();

	currently_parsed_filename = zend_ini_scanner_get_filename(TSRMLS_C);
	if (currently_parsed_filename) {
		error_buf_len = 128 + strlen(msg) + strlen(currently_parsed_filename); /* should be more than enough */
		error_buf = (char *) emalloc(error_buf_len);

		sprintf(error_buf, "%s in %s on line %d\n", msg, currently_parsed_filename, zend_ini_scanner_get_lineno(TSRMLS_C));
	} else {
		error_buf = estrdup("Invalid configuration directive\n");
	}

	if (CG(ini_parser_unbuffered_errors)) {
#ifdef PHP_WIN32
		MessageBox(NULL, error_buf, "PHP Error", MB_OK|MB_TOPMOST|0x00200000L);
#else
		fprintf(stderr, "PHP:  %s", error_buf);
#endif
	} else {
		zend_error(E_WARNING, "%s", error_buf);
	}
	efree(error_buf);
}
/* }}} */

/* {{{ zend_parse_ini_file()
*/
ZEND_API int zend_parse_ini_file(zend_file_handle *fh, zend_bool unbuffered_errors, int scanner_mode, zend_ini_parser_cb_t ini_parser_cb, void *arg TSRMLS_DC)
{
	int retval;
	zend_ini_parser_param ini_parser_param;

	ini_parser_param.ini_parser_cb = ini_parser_cb;
	ini_parser_param.arg = arg;
	CG(ini_parser_param) = &ini_parser_param;

	if (zend_ini_open_file_for_scanning(fh, scanner_mode TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	CG(ini_parser_unbuffered_errors) = unbuffered_errors;
	retval = ini_parse(TSRMLS_C);
	zend_file_handle_dtor(fh TSRMLS_CC);

	shutdown_ini_scanner(TSRMLS_C);
	
	if (retval == 0) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

/* {{{ zend_parse_ini_string()
*/
ZEND_API int zend_parse_ini_string(char *str, zend_bool unbuffered_errors, int scanner_mode, zend_ini_parser_cb_t ini_parser_cb, void *arg TSRMLS_DC)
{
	int retval;
	zend_ini_parser_param ini_parser_param;

	ini_parser_param.ini_parser_cb = ini_parser_cb;
	ini_parser_param.arg = arg;
	CG(ini_parser_param) = &ini_parser_param;

	if (zend_ini_prepare_string_for_scanning(str, scanner_mode TSRMLS_CC) == FAILURE) {
		return FAILURE;
	}

	CG(ini_parser_unbuffered_errors) = unbuffered_errors;
	retval = ini_parse(TSRMLS_C);

	shutdown_ini_scanner(TSRMLS_C);

	if (retval == 0) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
/* }}} */

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		69
#define	YYFLAG		-32768
#define	YYNTBASE	43

#define YYTRANSLATE(x) ((unsigned)(x) <= 270 ? yytranslate[x] : 55)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    39,    21,     2,    29,    28,    38,    22,    41,
    42,    27,    24,    19,    25,    20,    26,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    18,     2,    31,
    17,    32,    33,    34,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    40,    23,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    35,    37,    36,    30,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     4,     8,    12,    18,    20,    22,    24,    25,
    27,    29,    31,    33,    35,    36,    39,    42,    43,    45,
    47,    51,    54,    57,    62,    64,    66,    70,    73,    76,
    81,    83,    87,    91,    94,    97,   101,   105,   107,   109,
   111,   113,   115,   117,   119,   121,   123
};

static const short yyrhs[] = {    43,
    44,     0,     0,     3,    45,    40,     0,     9,    17,    46,
     0,    10,    47,    40,    17,    46,     0,     9,     0,    16,
     0,    49,     0,     0,    51,     0,    14,     0,    15,     0,
    16,     0,    50,     0,     0,    48,    52,     0,    48,    13,
     0,     0,    52,     0,    53,     0,    21,    48,    21,     0,
    49,    52,     0,    49,    53,     0,    49,    21,    48,    21,
     0,    52,     0,    54,     0,    21,    48,    21,     0,    50,
    52,     0,    50,    54,     0,    50,    21,    48,    21,     0,
    50,     0,    51,    37,    51,     0,    51,    38,    51,     0,
    30,    51,     0,    39,    51,     0,    41,    51,    42,     0,
    11,    12,    36,     0,     5,     0,     4,     0,     6,     0,
     7,     0,     8,     0,     5,     0,     4,     0,     6,     0,
     7,     0,     8,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   272,   274,   277,   285,   293,   302,   303,   306,   308,   311,
   313,   314,   315,   318,   320,   323,   325,   326,   329,   331,
   332,   333,   334,   335,   338,   340,   341,   342,   343,   344,
   347,   349,   350,   351,   352,   353,   356,   360,   362,   363,
   364,   365,   368,   370,   371,   372,   373
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","TC_SECTION",
"TC_RAW","TC_CONSTANT","TC_NUMBER","TC_STRING","TC_WHITESPACE","TC_LABEL","TC_OFFSET",
"TC_DOLLAR_CURLY","TC_VARNAME","TC_QUOTED_STRING","BOOL_TRUE","BOOL_FALSE","END_OF_LINE",
"'='","':'","','","'.'","'\\\"'","'\\''","'^'","'+'","'-'","'/'","'*'","'%'",
"'$'","'~'","'<'","'>'","'?'","'@'","'{'","'}'","'|'","'&'","'!'","']'","'('",
"')'","statement_list","statement","section_string_or_value","string_or_value",
"option_offset","encapsed_list","var_string_list_section","var_string_list",
"expr","cfg_var_ref","constant_literal","constant_string", NULL
};
#endif

static const short yyr1[] = {     0,
    43,    43,    44,    44,    44,    44,    44,    45,    45,    46,
    46,    46,    46,    47,    47,    48,    48,    48,    49,    49,
    49,    49,    49,    49,    50,    50,    50,    50,    50,    50,
    51,    51,    51,    51,    51,    51,    52,    53,    53,    53,
    53,    53,    54,    54,    54,    54,    54
};

static const short yyr2[] = {     0,
     2,     0,     3,     3,     5,     1,     1,     1,     0,     1,
     1,     1,     1,     1,     0,     2,     2,     0,     1,     1,
     3,     2,     2,     4,     1,     1,     3,     2,     2,     4,
     1,     3,     3,     2,     2,     3,     3,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1
};

static const short yydefact[] = {     2,
     0,     9,     6,    15,     7,     1,    39,    38,    40,    41,
    42,     0,    18,     0,     8,    19,    20,     0,    44,    43,
    45,    46,    47,    18,     0,    14,    25,    26,     0,     0,
     3,    18,    22,    23,    11,    12,    13,     0,     0,     0,
     4,    31,    10,     0,     0,    18,    28,    29,    37,    17,
    21,    16,     0,    34,    35,     0,     0,     0,    27,     0,
     0,    24,    36,    32,    33,     5,    30,     0,     0
};

static const short yydefgoto[] = {     1,
     6,    14,    41,    25,    30,    15,    42,    43,    27,    17,
    28
};

static const short yypact[] = {-32768,
    93,    39,    -5,    65,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,     8,-32768,   -18,    73,-32768,-32768,     0,-32768,-32768,
-32768,-32768,-32768,-32768,   -15,    84,-32768,-32768,    -9,    41,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    27,    27,    27,
-32768,    84,   -35,    42,    12,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    54,-32768,-32768,    45,    27,    27,-32768,     0,
    86,-32768,-32768,-32768,-32768,-32768,-32768,    49,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,    -4,-32768,   -23,-32768,    57,   -21,    -2,    43,
   -16
};


#define	YYLAST		109


static const short yytable[] = {    16,
    44,    57,    58,    19,    20,    21,    22,    23,    53,    48,
    12,    18,    33,    35,    36,    37,    54,    55,    56,    29,
    24,    31,    61,    47,    45,    48,    49,    52,    60,    38,
    19,    20,    21,    22,    23,    64,    65,    12,    39,    47,
    40,    52,     7,     8,     9,    10,    11,    24,    69,    12,
    52,    12,    12,    50,    50,    66,    38,    34,    52,    13,
    26,    51,    59,     0,    12,    39,    50,    40,    19,    20,
    21,    22,    23,     0,    62,    12,     7,     8,     9,    10,
    11,    57,    58,    12,     0,    24,    63,    19,    20,    21,
    22,    23,    68,    32,    12,     2,    12,     0,    50,     0,
     0,     3,     4,     0,    46,     0,    67,     0,     5
};

static const short yycheck[] = {     2,
    24,    37,    38,     4,     5,     6,     7,     8,    32,    26,
    11,    17,    15,    14,    15,    16,    38,    39,    40,    12,
    21,    40,    46,    26,    40,    42,    36,    30,    17,    30,
     4,     5,     6,     7,     8,    57,    58,    11,    39,    42,
    41,    44,     4,     5,     6,     7,     8,    21,     0,    11,
    53,    11,    11,    13,    13,    60,    30,    15,    61,    21,
     4,    21,    21,    -1,    11,    39,    13,    41,     4,     5,
     6,     7,     8,    -1,    21,    11,     4,     5,     6,     7,
     8,    37,    38,    11,    -1,    21,    42,     4,     5,     6,
     7,     8,     0,    21,    11,     3,    11,    -1,    13,    -1,
    -1,     9,    10,    -1,    21,    -1,    21,    -1,    16
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/local/share/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/local/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
#line 278 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{
#if DEBUG_CFG_PARSER
			printf("SECTION: [%s]\n", Z_STRVAL(yyvsp[-1]));
#endif
			ZEND_INI_PARSER_CB(&yyvsp[-1], NULL, NULL, ZEND_INI_PARSER_SECTION, ZEND_INI_PARSER_ARG TSRMLS_CC);
			free(Z_STRVAL(yyvsp[-1]));
		;
    break;}
case 4:
#line 285 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{
#if DEBUG_CFG_PARSER
			printf("NORMAL: '%s' = '%s'\n", Z_STRVAL(yyvsp[-2]), Z_STRVAL(yyvsp[0]));
#endif
			ZEND_INI_PARSER_CB(&yyvsp[-2], &yyvsp[0], NULL, ZEND_INI_PARSER_ENTRY, ZEND_INI_PARSER_ARG TSRMLS_CC);
			free(Z_STRVAL(yyvsp[-2]));
			free(Z_STRVAL(yyvsp[0]));
		;
    break;}
case 5:
#line 293 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{
#if DEBUG_CFG_PARSER
			printf("OFFSET: '%s'[%s] = '%s'\n", Z_STRVAL(yyvsp[-4]), Z_STRVAL(yyvsp[-3]), Z_STRVAL(yyvsp[0]));
#endif
			ZEND_INI_PARSER_CB(&yyvsp[-4], &yyvsp[0], &yyvsp[-3], ZEND_INI_PARSER_POP_ENTRY, ZEND_INI_PARSER_ARG TSRMLS_CC);
			free(Z_STRVAL(yyvsp[-4]));
			free(Z_STRVAL(yyvsp[-3]));
			free(Z_STRVAL(yyvsp[0]));
		;
    break;}
case 6:
#line 302 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ ZEND_INI_PARSER_CB(&yyvsp[0], NULL, NULL, ZEND_INI_PARSER_ENTRY, ZEND_INI_PARSER_ARG TSRMLS_CC); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 8:
#line 307 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 9:
#line 308 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_init_string(&yyval); ;
    break;}
case 10:
#line 312 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 11:
#line 313 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 12:
#line 314 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 13:
#line 315 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_init_string(&yyval); ;
    break;}
case 14:
#line 319 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 15:
#line 320 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_init_string(&yyval); ;
    break;}
case 16:
#line 324 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 17:
#line 325 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 18:
#line 326 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_init_string(&yyval); ;
    break;}
case 19:
#line 330 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 20:
#line 331 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 21:
#line 332 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[-1]; ;
    break;}
case 22:
#line 333 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 23:
#line 334 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 24:
#line 335 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-3], &yyvsp[-1]); free(Z_STRVAL(yyvsp[-1])); ;
    break;}
case 25:
#line 339 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 26:
#line 340 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 27:
#line 341 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[-1]; ;
    break;}
case 28:
#line 342 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 29:
#line 343 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-1], &yyvsp[0]); free(Z_STRVAL(yyvsp[0])); ;
    break;}
case 30:
#line 344 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_add_string(&yyval, &yyvsp[-3], &yyvsp[-1]); free(Z_STRVAL(yyvsp[-1])); ;
    break;}
case 31:
#line 348 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 32:
#line 349 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_do_op('|', &yyval, &yyvsp[-2], &yyvsp[0]); ;
    break;}
case 33:
#line 350 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_do_op('&', &yyval, &yyvsp[-2], &yyvsp[0]); ;
    break;}
case 34:
#line 351 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_do_op('~', &yyval, &yyvsp[0], NULL); ;
    break;}
case 35:
#line 352 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_do_op('!', &yyval, &yyvsp[0], NULL); ;
    break;}
case 36:
#line 353 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[-1]; ;
    break;}
case 37:
#line 357 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_get_var(&yyval, &yyvsp[-1] TSRMLS_CC); free(Z_STRVAL(yyvsp[-1])); ;
    break;}
case 38:
#line 361 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; ;
    break;}
case 39:
#line 362 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_RAW: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 40:
#line 363 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_NUMBER: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 41:
#line 364 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_STRING: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 42:
#line 365 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_WHITESPACE: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 43:
#line 369 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ zend_ini_get_constant(&yyval, &yyvsp[0] TSRMLS_CC); ;
    break;}
case 44:
#line 370 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_RAW: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 45:
#line 371 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_NUMBER: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 46:
#line 372 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_STRING: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
case 47:
#line 373 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
{ yyval = yyvsp[0]; /*printf("TC_WHITESPACE: '%s'\n", Z_STRVAL($1));*/ ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/local/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 383 "/home/oasynnoum/Projects/NetBeans/php-src-trunk/Zend/zend_ini_parser.y"
