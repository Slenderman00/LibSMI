/*
 * scanner-smi.h --
 *
 *      Definition of lexical tokens of the SMIv1/v2 MIB module language.
 *
 * Copyright (c) 1998 Technical University of Braunschweig.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: scanner-smi.h,v 1.1 1999/03/11 17:33:01 strauss Exp $
 */

#ifndef _SCANNER_SMI_H
#define _SCANNER_SMI_H

#include <stdio.h>

#include "parser-smi.h"

#define YY_NO_UNPUT



/* we need a reentrant parser, so yylex gets an argument */
#define YY_DECL int yylex YY_PROTO((YYSTYPE *lvalp, void *parser))

extern char *yytext;
extern int yyleng;

extern int smiEnterLexRecursion(FILE *file);
extern void smiLeaveLexRecursion();

#endif /* _SCANNER_SMI_H */
