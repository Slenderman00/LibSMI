/*
 * dump-stools.c --
 *
 *      Operations to generate MIB module stubs for the stools package
 *
 * Copyright (c) 2001 J. Schoenwaelder, Technical University of Braunschweig.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: dump-stools.c,v 1.17 2001/04/11 13:43:45 schoenw Exp $
 */

/*
 * TODO:
 *	  - generate #defines for deprecated and obsolete objects
 *	  - generate range/size checking code
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WIN_H
#include "win.h"
#endif

#include "smi.h"
#include "smidump.h"



static char *
getStringTime(time_t t)
{
    static char   s[27];
    struct tm	  *tm;

    tm = gmtime(&t);
    sprintf(s, "%04d-%02d-%02d %02d:%02d",
	    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	    tm->tm_hour, tm->tm_min);
    return s;
}



static void
printCommentString(FILE *f, char *s)
{
    int i, len;

    if (s) {
	fprintf(f, " *   ");
	len = strlen(s);
	for (i = 0; i < len; i++) {
	    fputc(s[i], f);
	    if (s[i] == '\n') {
		fprintf(f, " *   ");
	    }
	}
	fputc('\n', f);
    }
}



static void
printTopComment(FILE *f, SmiModule *smiModule)
{
    SmiRevision *smiRevision;
    char *date;

    fprintf(f,
	    "/*	\t\t\t\t\t\t-- DO NOT EDIT --\n"
	    " * This file has been generated by smidump\n"
	    " * version " SMI_VERSION_STRING " for the stools package.\n"
	    " *\n"
	    " * Derived from %s:\n",
	    smiModule->name);

    printCommentString(f, smiModule->description);

    for (smiRevision = smiGetFirstRevision(smiModule);
	 smiRevision;
	 smiRevision = smiGetNextRevision(smiRevision)) {
	date = getStringTime(smiRevision->date);
	fprintf(f,
		" *\n"
		" * Revision %s:\n", date);
	printCommentString(f, smiRevision->description);
    }

    fprintf(f,
	    " *\n * $I" "d$\n"
	    " */\n"
	    "\n");
}



static char*
translate(char *m)
{
    char *s;
    int i;

    s = xstrdup(m);
    for (i = 0; s[i]; i++) {
	if (s[i] == '-') s[i] = '_';
    }
  
    return s;
}



static char*
translateUpper(char *m)
{
    char *s;
    int i;

    s = xstrdup(m);
    for (i = 0; s[i]; i++) {
	if (s[i] == '-') s[i] = '_';
	if (islower((int) s[i])) {
	    s[i] = toupper(s[i]);
	}
    }
  
    return s;
}



static char*
translateLower(char *m)
{
    char *s;
    int i;

    s = xstrdup(m);
    for (i = 0; s[i]; i++) {
	if (s[i] == '-') s[i] = '_';
	if (isupper((int) s[i])) {
	    s[i] = tolower(s[i]);
	}
    }
  
    return s;
}



static char*
translateFileName(char *m)
{
    char *s;
    int i;

    s = xstrdup(m);
    for (i = 0; s[i]; i++) {
	if (s[i] == '_') s[i] = '-';
	if (isupper((int) s[i])) {
	    s[i] = tolower(s[i]);
	}
    }
  
    return s;
}



static FILE *
createFile(char *name, char *suffix)
{
    char *fullname;
    FILE *f;

    fullname = xmalloc(strlen(name) + (suffix ? strlen(suffix) : 0) + 2);
    strcpy(fullname, name);
    if (suffix) {
        strcat(fullname, suffix);
    }
    if (!access(fullname, R_OK)) {
        fprintf(stderr, "smidump: %s already exists\n", fullname);
        xfree(fullname);
        return NULL;
    }
    f = fopen(fullname, "w");
    if (!f) {
        fprintf(stderr, "smidump: cannot open %s for writing: ", fullname);
        perror(NULL);
        xfree(fullname);
        exit(1);
    }
    xfree(fullname);
    return f;
}



static int
isGroup(SmiNode *smiNode)
{
    SmiNode *childNode;

    if (smiNode->nodekind == SMI_NODEKIND_ROW) {
	return 1;
    }
    
    for (childNode = smiGetFirstChildNode(smiNode);
	 childNode;
	 childNode = smiGetNextChildNode(childNode)) {
	if (childNode->nodekind == SMI_NODEKIND_SCALAR) {
	    return 1;
	}
    }

    return 0;
}



static int
isAccessible(SmiNode *groupNode)
{
    SmiNode *smiNode;
    int num = 0;
    
    for (smiNode = smiGetFirstChildNode(groupNode);
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	if ((smiNode->nodekind == SMI_NODEKIND_SCALAR
	     || smiNode->nodekind == SMI_NODEKIND_COLUMN)
	    && (smiNode->access == SMI_ACCESS_READ_ONLY
		|| smiNode->access == SMI_ACCESS_READ_WRITE)) {
	    num++;
	}
    }

    return num;
}



static int
isIndex(SmiNode *groupNode, SmiNode *smiNode)
{
    SmiElement *smiElement;
    
    /*
     * Perhaps this test needs to be more sophisticated if you have
     * really creative cross-table indexing constructions...
     */

    for (smiElement = smiGetFirstElement(groupNode);
	 smiElement; smiElement = smiGetNextElement(smiElement)) {
	if (smiNode == smiGetElementNode(smiElement)) {
	    return 1;
	}
    }

    return 0;
}



static unsigned int
getMinSize(SmiType *smiType)
{
    SmiRange *smiRange;
    SmiType  *parentType;
    unsigned int min = 65535, size;
    
    switch (smiType->basetype) {
    case SMI_BASETYPE_BITS:
	return 0;
    case SMI_BASETYPE_OCTETSTRING:
    case SMI_BASETYPE_OBJECTIDENTIFIER:
	size = 0;
	break;
    default:
	return -1;
    }

    for (smiRange = smiGetFirstRange(smiType);
	 smiRange ; smiRange = smiGetNextRange(smiRange)) {
	if (smiRange->minValue.value.unsigned32 < min) {
	    min = smiRange->minValue.value.unsigned32;
	}
    }
    if (min < 65535 && min > size) {
	size = min;
    }

    parentType = smiGetParentType(smiType);
    if (parentType) {
	unsigned int psize = getMinSize(parentType);
	if (psize > size) {
	    size = psize;
	}
    }

    return size;
}



static unsigned int
getMaxSize(SmiType *smiType)
{
    SmiRange *smiRange;
    SmiType  *parentType;
    SmiNamedNumber *nn;
    unsigned int max = 0, size;
    
    switch (smiType->basetype) {
    case SMI_BASETYPE_BITS:
    case SMI_BASETYPE_OCTETSTRING:
	size = 65535;
	break;
    case SMI_BASETYPE_OBJECTIDENTIFIER:
	size = 128;
	break;
    default:
	return -1;
    }

    if (smiType->basetype == SMI_BASETYPE_BITS) {
	for (nn = smiGetFirstNamedNumber(smiType);
	     nn;
	     nn = smiGetNextNamedNumber(nn)) {
	    if (nn->value.value.unsigned32 > max) {
		max = nn->value.value.unsigned32;
	    }
	}
	size = (max / 8) + 1;
	return size;
    }

    for (smiRange = smiGetFirstRange(smiType);
	 smiRange ; smiRange = smiGetNextRange(smiRange)) {
	if (smiRange->maxValue.value.unsigned32 > max) {
	    max = smiRange->maxValue.value.unsigned32;
	}
    }
    if (max > 0 && max < size) {
	size = max;
    }

    parentType = smiGetParentType(smiType);
    if (parentType) {
	unsigned int psize = getMaxSize(parentType);
	if (psize < size) {
	    size = psize;
	}
    }

    return size;
}



static char*
getSnmpType(SmiType *smiType)
{
    struct {
	char *module;
	char *name;
	char *tag;
    } typemap[] = {
	{ "RFC1155-SMI",	"Counter",	"G_SNMP_COUNTER32" },
	{ "SNMPv2-SMI",		"Counter32",	"G_SNMP_COUNTER32" },
	{ "RFC1155-SMI",	"TimeTicks",	"G_SNMP_TIMETICKS" },
	{ "SNMPv2-SMI",		"TimeTicks",	"G_SNMP_TIMETICKS" },
	{ "RFC1155-SMI",	"Opaque",	"G_SNMP_OPAQUE" },
	{ "SNMPv2-SMI",		"Opaque",	"G_SNMP_OPAQUE" },
	{ "RFC1155-SMI",	"IpAddress",	"G_SNMP_IPADDRESS" },
	{ "SNMPv2-SMI",		"IpAddress",	"G_SNMP_IPADDRESS" },
	{ NULL, NULL, NULL }
    };

    SmiBasetype basetype = smiType->basetype;
    
    do {
	int i;
	for (i = 0; typemap[i].name; i++) {
	    if (smiType->name
		&& (strcmp(smiType->name, typemap[i].name) == 0)) {
		return typemap[i].tag;
	    }
	}
    } while ((smiType = smiGetParentType(smiType)));

    switch (basetype) {
    case SMI_BASETYPE_INTEGER32:
    case SMI_BASETYPE_ENUM:
	return "G_SNMP_INTEGER32";
    case SMI_BASETYPE_UNSIGNED32:
	return "G_SNMP_UNSIGNED32";
    case SMI_BASETYPE_INTEGER64:
	return NULL;
    case SMI_BASETYPE_UNSIGNED64:
	return "G_SNMP_COUNTER64";
    case SMI_BASETYPE_OCTETSTRING:
	return "G_SNMP_OCTET_STRING";
    case SMI_BASETYPE_BITS:
	return "G_SNMP_OCTET_STRING";
    case SMI_BASETYPE_OBJECTIDENTIFIER:
	return "G_SNMP_OBJECT_ID";
    case SMI_BASETYPE_FLOAT32:
    case SMI_BASETYPE_FLOAT64:
    case SMI_BASETYPE_FLOAT128:
	return NULL;
    case SMI_BASETYPE_UNKNOWN:
	return NULL;
    }
    return NULL;
}



static void
printHeaderEnumerations(FILE *f, SmiModule *smiModule)
{
    SmiNode  *smiNode;
    SmiType  *smiType;
    SmiNamedNumber *nn;
    int      cnt = 0;
    char     *cName, *cModuleName;
    char     *dName, *dModuleName;

    cModuleName = translateLower(smiModule->name);
    dModuleName = translateUpper(smiModule->name);

    for (smiNode = smiGetFirstNode(smiModule,
				   SMI_NODEKIND_SCALAR | SMI_NODEKIND_COLUMN);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode,
				  SMI_NODEKIND_SCALAR | SMI_NODEKIND_COLUMN)) {
	smiType = smiGetNodeType(smiNode);
	if (smiType && smiType->basetype == SMI_BASETYPE_ENUM) {
	    if (! cnt) {
		fprintf(f,
			"/*\n"
			" * Tables to map enumerations to strings and vice versa.\n"
			" */\n"
			"\n");
	    }
	    cnt++;
	    cName = translate(smiNode->name);
	    dName = translateUpper(smiNode->name);
	    for (nn = smiGetFirstNamedNumber(smiType); nn;
		 nn = smiGetNextNamedNumber(nn)) {
		char *dEnum = translateUpper(nn->name);
		fprintf(f, "#define %s_%s_%s\t%d\n",
			dModuleName, dName, dEnum,
			(int) nn->value.value.integer32);
		xfree(dEnum);
	    }
	    fprintf(f, "\nextern stls_enum_t const %s_enums_%s[];\n\n",
		    cModuleName, cName);
	    xfree(dName);
	    xfree(cName);
	}
    }
    
    if (cnt) {
	fprintf(f, "\n");
    }

    xfree(dModuleName);
    xfree(cModuleName);
}



static void
printHeaderIdentities(FILE *f, SmiModule *smiModule)
{
    SmiNode  *smiNode, *moduleIdentityNode;
    int      i, cnt = 0;
    char     *dName, *dModuleName;
    char     *cModuleName;

    moduleIdentityNode = smiGetModuleIdentityNode(smiModule);
    
    dModuleName = translateUpper(smiModule->name);

    for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_NODE);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_NODE)) {
	if (smiNode->status == SMI_STATUS_UNKNOWN) {
	    continue;
	}
	if (smiNode == moduleIdentityNode) {
	    continue;
	}
	if (! cnt) {
	    fprintf(f,
		    "/*\n"
		    " * Tables to map identities to strings and vice versa.\n"
		    " */\n"
		    "\n");
	}
	cnt++;
	dName = translateUpper(smiNode->name);
	fprintf(f, "#define %s_%s\t", dModuleName, dName);
	for (i = 0; i < smiNode->oidlen; i++) {
	    fprintf(f, "%s%u", i ? "," : "", smiNode->oid[i]);
	}
	fprintf(f, "\n");
	xfree(dName);
    }
    
    if (cnt) {
	cModuleName = translateLower(smiModule->name);
	fprintf(f,
		"\n"
		"extern stls_identity_t const %s_identities[];\n"
		"\n",
		cModuleName);
	xfree(cModuleName);
    }

    xfree(dModuleName);
}



static void
printHeaderTypedefMember(FILE *f, SmiNode *smiNode,
			 SmiType *smiType, int isIndex)
{
    char *cName;
    unsigned minSize, maxSize;

    cName = translate(smiNode->name);
    switch (smiType->basetype) {
    case SMI_BASETYPE_OBJECTIDENTIFIER:
	maxSize = getMaxSize(smiType);
	minSize = getMinSize(smiType);
	if (isIndex) {
	    fprintf(f,
		    "    guint32  %s[%u];\n", cName,
		    maxSize < 128 ? maxSize : 128);
	} else {
	    fprintf(f,
		    "    guint32  *%s;\n", cName);
	}
	if (maxSize != minSize) {
	    fprintf(f,
		    "    gsize    _%sLength;\n", cName);
	}
	break;
    case SMI_BASETYPE_OCTETSTRING:
    case SMI_BASETYPE_BITS:
	maxSize = getMaxSize(smiType);
	minSize = getMinSize(smiType);
	if (isIndex) {
	    fprintf(f,
		    "    guchar   %s[%u];\n", cName,
		    maxSize < 128 ? maxSize : 128);
	} else {
	    fprintf(f,
		    "    guchar   *%s;\n", cName);
	}
	if (maxSize != minSize) {
	    fprintf(f,
		    "    gsize    _%sLength;\n", cName);
	}
	break;
    case SMI_BASETYPE_ENUM:
    case SMI_BASETYPE_INTEGER32:
	fprintf(f,
		"    gint32   %s%s;\n", isIndex ? "" : "*", cName);
	break;
    case SMI_BASETYPE_UNSIGNED32:
	fprintf(f,
		"    guint32  %s%s;\n", isIndex ? "" : "*", cName);
	break;
    case SMI_BASETYPE_INTEGER64:
	fprintf(f,
		"    gint64   *%s; \n", cName);
	break;
    case SMI_BASETYPE_UNSIGNED64:
	fprintf(f,
		"    guint64  *%s; \n", cName);
	break;
    default:
	fprintf(f,
		"    /* ?? */  _%s; \n", cName);
	break;
    }
    xfree(cName);
}



static void
printHeaderTypedefIndex(FILE *f, SmiNode *smiNode, SmiNode *groupNode)
{
    SmiElement *smiElement;
    SmiNode *iNode;
    SmiType *iType;
    
    for (smiElement = smiGetFirstElement(smiNode);
	 smiElement; smiElement = smiGetNextElement(smiElement)) {
	iNode = smiGetElementNode(smiElement);
	if (iNode) {
	    iType = smiGetNodeType(iNode);
	    if (iType) {
		printHeaderTypedefMember(f, iNode, iType, 1);
	    }
	}
    }
}



static void
printHeaderTypedef(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    SmiNode *smiNode, *indexNode;
    SmiType *smiType;
    char    *cModuleName, *cGroupName;
    int     writable = 0;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    fprintf(f,
	    "/*\n"
	    " * C type definitions for %s::%s.\n"
	    " */\n\n",
	    smiModule->name, groupNode->name);
    
    fprintf(f, "typedef struct {\n");

    /*
     * print index objects that are not part of the group
     */

    switch (groupNode->indexkind) {
    case SMI_INDEX_INDEX:
    case SMI_INDEX_REORDER:
	printHeaderTypedefIndex(f, groupNode, groupNode);
	break;
    case SMI_INDEX_EXPAND:	/* TODO: we have to do more work here! */
	break;
    case SMI_INDEX_AUGMENT:
    case SMI_INDEX_SPARSE:
	indexNode = smiGetRelatedNode(groupNode);
	if (indexNode) {
	    printHeaderTypedefIndex(f, indexNode, groupNode);
	}
	break;
    case SMI_INDEX_UNKNOWN:
	break;
    }

	    
    for (smiNode = smiGetFirstChildNode(groupNode);
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	if (smiNode->nodekind & (SMI_NODEKIND_COLUMN | SMI_NODEKIND_SCALAR)
	    && (smiNode->access >= SMI_ACCESS_READ_ONLY)) {
	    if (isIndex(groupNode, smiNode)) {
		continue;
	    }
	    if (smiNode->access == SMI_ACCESS_READ_WRITE) {
		writable++;
	    }
	    smiType = smiGetNodeType(smiNode);
	    if (! smiType) {
		continue;
	    }
	    printHeaderTypedefMember(f, smiNode, smiType, 0);
	}	    
    }

    fprintf(f, "} %s_%s_t;\n\n", cModuleName, cGroupName);

    if (groupNode->nodekind == SMI_NODEKIND_ROW) {
	char *cTableName;
	SmiNode *tableNode;

	tableNode = smiGetParentNode(groupNode);
	if (tableNode) {
	    cTableName = translate(tableNode->name);
	    fprintf(f, "extern int\n"
		    "%s_get_%s(GSnmpSession *s, %s_%s_t ***%s);\n\n",
		    cModuleName, cTableName,
		    cModuleName, cGroupName, cGroupName);
	    fprintf(f, "extern void\n"
		    "%s_free_%s(%s_%s_t **%s);\n\n",
		    cModuleName, cTableName,
		    cModuleName, cGroupName, cGroupName);
	    xfree(cTableName);
	}
    }
    fprintf(f, "extern %s_%s_t *\n"
	    "%s_new_%s();\n\n",
	    cModuleName, cGroupName, cModuleName, cGroupName);
    fprintf(f, "extern int\n"
	    "%s_get_%s(GSnmpSession *s, %s_%s_t **%s);\n\n",
	    cModuleName, cGroupName,
	    cModuleName, cGroupName, cGroupName);
    if (writable) {
	fprintf(f, "extern int\n"
		"%s_set_%s(GSnmpSession *s, %s_%s_t *%s);\n\n",
		cModuleName, cGroupName,
		cModuleName, cGroupName, cGroupName);
    }
    fprintf(f, "extern void\n"
	    "%s_free_%s(%s_%s_t *%s);\n\n",
	    cModuleName, cGroupName,
	    cModuleName, cGroupName, cGroupName);
	    
    xfree(cGroupName);
    xfree(cModuleName);
}



static void
printHeaderTypedefs(FILE *f, SmiModule *smiModule)
{
    SmiNode   *smiNode;
    int       cnt = 0;
    
    for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_ANY);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_ANY)) {
	if (isGroup(smiNode) && isAccessible(smiNode)) {
	    cnt++;
	    printHeaderTypedef(f, smiModule, smiNode);
	}
    }
    
    if (cnt) {
	fprintf(f, "\n");
    }
}



static void
dumpHeader(SmiModule *smiModule, char *baseName)
{
    char *pModuleName;
    FILE *f;
    
    pModuleName = translateUpper(smiModule->name);

    f = createFile(baseName, ".h");
    if (! f) {
	return;
    }

    printTopComment(f, smiModule);
    
    fprintf(f,
	    "#ifndef _%s_H_\n"
	    "#define _%s_H_\n"
	    "\n"
	    "#include \"stools.h\"\n"
	    "\n",
	    pModuleName, pModuleName);

    printHeaderEnumerations(f, smiModule);
    printHeaderIdentities(f, smiModule);
    printHeaderTypedefs(f, smiModule);

    fprintf(f,
	    "#endif /* _%s_H_ */\n",
	    pModuleName);

    fclose(f);
    xfree(pModuleName);
}



static void
printStubEnumerations(FILE *f, SmiModule *smiModule)
{
    SmiNode   *smiNode;
    SmiType   *smiType;
    SmiNamedNumber *nn;
    char      *cName, *cModuleName;
    char      *dName, *dModuleName;
    int       cnt = 0;
    
    cModuleName = translateLower(smiModule->name);
    dModuleName = translateUpper(smiModule->name);
    
    for (smiNode = smiGetFirstNode(smiModule,
				   SMI_NODEKIND_SCALAR | SMI_NODEKIND_COLUMN);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode,
				  SMI_NODEKIND_SCALAR | SMI_NODEKIND_COLUMN)) {
	smiType = smiGetNodeType(smiNode);
	if (smiType && smiType->basetype == SMI_BASETYPE_ENUM) {
	    cnt++;
	    cName = translate(smiNode->name);
	    dName = translateUpper(smiNode->name);
	    fprintf(f, "stls_enum_t const %s_enums_%s[] = {\n",
		    cModuleName, cName);
	    for (nn = smiGetFirstNamedNumber(smiType); nn;
		 nn = smiGetNextNamedNumber(nn)) {
		char *dEnum = translateUpper(nn->name);
		fprintf(f, "    { %s_%s_%s,\t\"%s\" },\n",
			dModuleName, dName, dEnum, nn->name);
		xfree(dEnum);
	    }
	    fprintf(f,
		    "    { 0, NULL }\n"
		    "};\n"
		    "\n");
	    xfree(dName);
	    xfree(cName);
	}
    }
    
    if (cnt) {
	fprintf(f, "\n");
    }

    xfree(dModuleName);
    xfree(cModuleName);
}



static void
printStubIdentities(FILE *f, SmiModule *smiModule)
{
    SmiNode   *smiNode, *moduleIdentityNode;
    char      *cName, *cModuleName;
    char      *dName, *dModuleName;
    int       cnt = 0;
    
    moduleIdentityNode = smiGetModuleIdentityNode(smiModule);
    
    cModuleName = translateLower(smiModule->name);
    dModuleName = translateUpper(smiModule->name);
    
    for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_NODE);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_NODE)) {
	if (smiNode->status == SMI_STATUS_UNKNOWN) {
	    continue;
	}
	if (smiNode == moduleIdentityNode) {
	    continue;
	}
	cnt++;
	cName = translate(smiNode->name);
	dName = translateUpper(smiNode->name);
	fprintf(f,
		"static guint32 const %s[]\n\t= { %s_%s };\n",
		cName, dModuleName, dName);
	xfree(dName);
	xfree(cName);
    }

    if (cnt) {
	fprintf(f,
		"\n"
		"stls_identity_t const %s_identities[] = {\n",
		cModuleName);
    
	for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_NODE);
	     smiNode;
	     smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_NODE)) {
	    if (smiNode->status == SMI_STATUS_UNKNOWN) {
		continue;
	    }
	    if (smiNode == moduleIdentityNode) {
		continue;
	    }
	    cName = translate(smiNode->name);
	    fprintf(f, "    { %s,\n"
		    "      sizeof(%s)/sizeof(guint32),\n"
		    "      \"%s\" },\n",
		    cName, cName, smiNode->name);
	    xfree(cName);
	}
	
	fprintf(f,
		"    { 0, 0, NULL }\n"
		"};\n"
		"\n"
		"\n");
    }

    xfree(dModuleName);
    xfree(cModuleName);
}



static void
printAttribute(FILE *f, SmiNode *smiNode, SmiNode *groupNode, int flags)
{
    SmiType *smiType;
    char *snmpType;

    smiType = smiGetNodeType(smiNode);
    if (!smiType) {
	return;
    }

    snmpType = getSnmpType(smiType);
    if (!snmpType) {
	return;
    }

    /*
     * Suppress all INDEX objects as if they were not-accessible.
     */

    if (flags) {
	if (isIndex(groupNode, smiNode)) {
	    return;
	}
    }

    fprintf(f, "    { %u, %s, \"%s\" },\n",
	    smiNode->oid[smiNode->oidlen-1], snmpType, smiNode->name);
}



static void
printScalarsAttributes(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    SmiNode *smiNode;
    
    for (smiNode = smiGetFirstChildNode(groupNode);
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	printAttribute(f, smiNode, groupNode, 0);
    }
}



static void
printTableAttributes(FILE *f, SmiModule *smiModule, SmiNode *entryNode)
{
    SmiNode *smiNode;
    int     idx, cnt;
    
    for (smiNode = smiGetFirstChildNode(entryNode), idx = 0, cnt = 0;
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	if (isIndex(entryNode, smiNode)) idx++;
	cnt++;
    }

    for (smiNode = smiGetFirstChildNode(entryNode);
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	printAttribute(f, smiNode, entryNode, cnt > idx);
    }
}



static void
printStubAttributes(FILE *f, SmiModule *smiModule)
{
    SmiNode *smiNode;
    char    *cName;
    int     cnt = 0;
    
    for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_ANY);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_ANY)) {
	if (isGroup(smiNode) && isAccessible(smiNode)) {
	    cnt++;
	    cName = translate(smiNode->name);
	    fprintf(f,
		    "static stls_stub_attr_t _%s[] = {\n",
		    cName);
	    if (smiNode->nodekind == SMI_NODEKIND_ROW) {
		printTableAttributes(f, smiModule, smiNode);
	    } else {
		printScalarsAttributes(f, smiModule, smiNode);
	    }
	    fprintf(f,
		    "    { 0, 0, NULL }\n"
		    "};\n"
		    "\n");
	    xfree(cName);
	}
    }
    
    if (cnt) {
	fprintf(f, "\n");
    }
}



static void
printUnpackMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    SmiElement *smiElement;
    SmiNode *indexNode = NULL;
    SmiNode *iNode;
    SmiType *iType;
    char    *cModuleName, *cGroupName, *cName;
    unsigned maxSize, minSize;
    int last = 0;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    switch (groupNode->indexkind) {
    case SMI_INDEX_INDEX:
    case SMI_INDEX_REORDER:
	indexNode = groupNode;
	break;
    case SMI_INDEX_EXPAND:	/* TODO: we have to do more work here! */
	indexNode = NULL;
	break;
    case SMI_INDEX_AUGMENT:
    case SMI_INDEX_SPARSE:
	indexNode = smiGetRelatedNode(groupNode);
	break;
    case SMI_INDEX_UNKNOWN:
	indexNode = NULL;
	break;
    }

    for (smiElement = smiGetFirstElement(indexNode);
	 smiElement; smiElement = smiGetNextElement(smiElement)) {
	iNode = smiGetElementNode(smiElement);
	if (iNode) {
	    iType = smiGetNodeType(iNode);
	    if (iType && iType->basetype == SMI_BASETYPE_OCTETSTRING) {
		break;
	    }
	}
    }

    fprintf(f,
	    "static int\n"
	    "unpack_%s(GSnmpVarBind *vb, %s_%s_t *%s)\n"
	    "{\n"
	    "    int %sidx = %u;\n"
	    "\n",
	    cGroupName, cModuleName, cGroupName, cGroupName,
	    smiElement ? "i, len, " : "", groupNode->oidlen + 1);

    for (smiElement = smiGetFirstElement(indexNode);
	 smiElement; smiElement = smiGetNextElement(smiElement)) {
	iNode = smiGetElementNode(smiElement);
	last = (smiGetNextElement(smiElement) == NULL);
	if (iNode) {
	    iType = smiGetNodeType(iNode);
	    if (! iType) {
		continue;
	    }
	    cName = translate(iNode->name);
	    switch (iType->basetype) {
	    case SMI_BASETYPE_ENUM:
	    case SMI_BASETYPE_INTEGER32:
		fprintf(f,
			"    if (vb->id_len < idx) return -1;\n"
			"    %s->%s = vb->id[idx++];\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_UNSIGNED32:
		fprintf(f,
			"    if (vb->id_len < idx) return -1;\n"
			"    %s->%s = vb->id[idx++];\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_OCTETSTRING:
		maxSize = getMaxSize(iType);
		minSize = getMinSize(iType);
		if (minSize == maxSize) {
		    fprintf(f,
			    "    len = %u;\n"
			    "    if (vb->id_len < idx + len) return -1;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n",
			    minSize, cGroupName, cName);
		} else if (last && indexNode->implied) {
		    fprintf(f,
			    "    if (vb->id_len < idx) return -1;\n"
			    "    len = vb->id_len - idx;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n"
			    "    %s->_%sLength = len;\n",
			    cGroupName, cName, cGroupName, cName);
		} else {
		    fprintf(f,
			    "    if (vb->id_len < idx) return -1;\n"
			    "    len = vb->id[idx++];\n"
			    "    if (vb->id_len < idx + len) return -1;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n"
			    "    %s->_%sLength = len;\n",
			    cGroupName, cName, cGroupName, cName);
		}
		break;
	    case SMI_BASETYPE_OBJECTIDENTIFIER:
		maxSize = getMaxSize(iType);
		minSize = getMinSize(iType);
		if (minSize == maxSize) {
		    fprintf(f,
			    "    len = %u;\n"
			    "    if (vb->id_len < idx + len) return -1;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n",
			    minSize, cGroupName, cName);
		} else if (last && indexNode->implied) {
		    fprintf(f,
			    "    if (vb->id_len < idx) return -1;\n"
			    "    len = vb->id_len - idx;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n"
			    "    %s->_%sLength = len;\n",
			    cGroupName, cName, cGroupName, cName);
		} else {
		    fprintf(f,
			    "    if (vb->id_len < idx) return -1;\n"
			    "    len = vb->id[idx++];\n"
			    "    if (vb->id_len < idx + len) return -1;\n"
			    "    for (i = 0; i < len; i++) {\n"
			    "        %s->%s[i] = vb->id[idx++];\n"
			    "    }\n"
			    "    %s->_%sLength = len;\n",
			    cGroupName, cName, cGroupName, cName);
		}
		break;
	    default:
		fprintf(f,
			"    /* XXX how to unpack %s->%s ? */\n",
			cGroupName, cName);
		break;
	    }
	    xfree(cName);
	}
    }

    fprintf(f,
	    "    if (vb->id_len > idx) return -1;\n"
	    "    return 0;\n"
	    "}\n\n");

    xfree(cGroupName);
    xfree(cModuleName);
}



static void
printVariableAssignement(FILE *f, SmiNode *groupNode)
{
    SmiNode *smiNode;
    SmiType *smiType;
    char    *cGroupName, *cName, *snmpType;
    unsigned maxSize, minSize;

    cGroupName = translate(groupNode->name);

    for (smiNode = smiGetFirstChildNode(groupNode);
	 smiNode;
	 smiNode = smiGetNextChildNode(smiNode)) {
	if (smiNode->nodekind & (SMI_NODEKIND_COLUMN | SMI_NODEKIND_SCALAR)
	    && (smiNode->access == SMI_ACCESS_READ_ONLY
		|| smiNode->access == SMI_ACCESS_READ_WRITE)) {

	    smiType = smiGetNodeType(smiNode);
	    if (!smiType) {
		continue;
	    }

	    if (isIndex(groupNode, smiNode)) {
		continue;
	    }
	    
	    snmpType = getSnmpType(smiType);
	    if (!snmpType) {
		continue;
	    }

	    cName = translate(smiNode->name);

	    fprintf(f,
		    "        case %d:\n", smiNode->oid[smiNode->oidlen-1]);
	    
	    switch (smiType->basetype) {
	    case SMI_BASETYPE_INTEGER32:
	    case SMI_BASETYPE_ENUM:
		fprintf(f,
			"            %s->%s = &(vb->syntax.i32[0]);\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_UNSIGNED32:
		fprintf(f,
			"            %s->%s = &(vb->syntax.ui32[0]);\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_INTEGER64:
		fprintf(f,
			"            %s->%s = &(vb->syntax.i64[0]);\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_UNSIGNED64:
		fprintf(f,
			"            %s->%s = &(vb->syntax.ui64[0]);\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_OCTETSTRING:
	    case SMI_BASETYPE_BITS:
		maxSize = getMaxSize(smiType);
		minSize = getMinSize(smiType);
		if (minSize != maxSize) {
		    fprintf(f,
			    "            %s->_%sLength = vb->syntax_len;\n",
			    cGroupName, cName);
		}
		fprintf(f,
			"            %s->%s = vb->syntax.uc;\n",
			cGroupName, cName);
		break;
	    case SMI_BASETYPE_OBJECTIDENTIFIER:
		fprintf(f,
			"            %s->_%sLength = vb->syntax_len / sizeof(guint32);\n"
			"            %s->%s = vb->syntax.ui32;\n",
			cGroupName, cName, cGroupName, cName);
		break;
	    default:
		break;
	    }
	    fprintf(f,
		    "            break;\n");
	    
	    xfree(cName);
	}
    }

    xfree(cGroupName);
}



static void
printAssignMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    char *cModuleName, *cGroupName;
    int i;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    if (groupNode->nodekind == SMI_NODEKIND_ROW) {
	printUnpackMethod(f, smiModule, groupNode);
    }
    
    fprintf(f,
	    "static %s_%s_t *\n"
	    "assign_%s(GSList *vbl)\n"
	    "{\n"
	    "    GSList *elem;\n"
	    "    %s_%s_t *%s;\n"
	    "    guint32 idx;\n"
	    "    char *p;\n",
	    cModuleName, cGroupName, cGroupName,
	    cModuleName, cGroupName, cGroupName);

    fprintf(f, "    static guint32 const base[] = {");
    for (i = 0; i < groupNode->oidlen; i++) {
	fprintf(f, "%s%u", i ? ", " : "", groupNode->oid[i]);
    }
    fprintf(f, "};\n\n");

    fprintf(f,
	    "    %s = %s_new_%s();\n"
	    "    if (! %s) {\n"
	    "        return NULL;\n"
	    "    }\n"
	    "\n",
	    cGroupName, cModuleName, cGroupName, cGroupName);

    fprintf(f,
	    "    p = (char *) %s + sizeof(%s_%s_t);\n"
	    "    * (GSList **) p = vbl;\n"
	    "\n", cGroupName, cModuleName, cGroupName);

    if (groupNode->nodekind == SMI_NODEKIND_ROW) {
	fprintf(f,
		"    if (unpack_%s((GSnmpVarBind *) vbl->data, %s) < 0) {\n"
		"        g_warning(\"illegal %s instance identifier\");\n"
		"        g_free(%s);\n"
		"        return NULL;\n"
		"    }\n\n",
		cGroupName, cGroupName, cGroupName, cGroupName);
    }
    
    fprintf(f,
	    "    for (elem = vbl; elem; elem = g_slist_next(elem)) {\n"
	    "        GSnmpVarBind *vb = (GSnmpVarBind *) elem->data;\n"
	    "\n"
	    "        if (stls_vb_lookup(vb, base, sizeof(base)/sizeof(guint32),\n"
	    "                           _%s, &idx) < 0) continue;\n"
	    "\n"
	    "        switch (idx) {\n",
	    cGroupName);
    
    printVariableAssignement(f, groupNode);

    fprintf(f,
	    "        };\n"
	    "    }\n"
	    "\n"
	    "    return %s;\n"
	    "}\n"
	    "\n", cGroupName);

    xfree(cGroupName);
    xfree(cModuleName);
}
 



static void
printGetTableMethod(FILE *f, SmiModule *smiModule, SmiNode *entryNode)
{
    SmiNode *tableNode;
    char    *cModuleName, *cEntryName, *cTableName;
    int     i;

    tableNode = smiGetParentNode(entryNode);
    if (! tableNode) {
	return;
    }

    cModuleName = translateLower(smiModule->name);
    cEntryName = translate(entryNode->name);
    cTableName = translate(tableNode->name);

    fprintf(f,
	    "int\n"
	    "%s_get_%s(GSnmpSession *s, %s_%s_t ***%s)\n"
	    "{\n"
	    "    GSList *in = NULL, *out = NULL;\n",
	    cModuleName, cTableName, cModuleName, cEntryName, cEntryName);

    fprintf(f,
	    "    GSList *row;\n"
	    "    int i;\n");

    fprintf(f, "    static guint32 base[] = {");
    for (i = 0; i < entryNode->oidlen; i++) {
	fprintf(f, "%u, ", entryNode->oid[i]);
    }
    fprintf(f, "0};\n");

    fprintf(f,
	    "\n"
	    "    *%s = NULL;\n"
	    "\n",
	    cEntryName);

    fprintf(f,
	    "    stls_vbl_attributes(s, &in, base, %u, _%s);\n",
	    entryNode->oidlen, cEntryName);

    fprintf(f,
	    "\n"
	    "    out = stls_snmp_gettable(s, in);\n"
	    "    /* stls_vbl_free(in); */\n"
	    "    if (! out) {\n"
	    "        return -2;\n"
	    "    }\n"
	    "\n");
    fprintf(f,
	    "    *%s = (%s_%s_t **) g_malloc0((g_slist_length(out) + 1) * sizeof(%s_%s_t *));\n"
	    "    if (! *%s) {\n"
	    "        return -4;\n"
	    "    }\n"
	    "\n",
	    cEntryName, cModuleName, cEntryName,
	    cModuleName, cEntryName, cEntryName);
    
    fprintf(f,
	    "    for (row = out, i = 0; row; row = g_slist_next(row), i++) {\n"
	    "        (*%s)[i] = assign_%s(row->data);\n"
	    "    }\n"
	    "\n", cEntryName, cEntryName);
    
    fprintf(f,
	    "    return 0;\n"
	    "}\n"
	    "\n");

    xfree(cTableName);
    xfree(cEntryName);
    xfree(cModuleName);
}
 



static void
printGetScalarsMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    char    *cModuleName, *cGroupName;
    int     i;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    fprintf(f,
	    "int\n"
	    "%s_get_%s(GSnmpSession *s, %s_%s_t %s%s)\n"
	    "{\n"
	    "    GSList *in = NULL, *out = NULL;\n",
	    cModuleName, cGroupName, cModuleName, cGroupName,
	    (groupNode->nodekind == SMI_NODEKIND_ROW) ? "***" : "**",
	    cGroupName);

    fprintf(f, "    static guint32 base[] = {");
    for (i = 0; i < groupNode->oidlen; i++) {
	fprintf(f, "%u, ", groupNode->oid[i]);
    }
    fprintf(f, "0};\n");

    fprintf(f,
	    "\n"
	    "    *%s = NULL;\n"
	    "\n",
	    cGroupName);

    fprintf(f,
	    "    stls_vbl_attributes(s, &in, base, %u, _%s);\n",
	    groupNode->oidlen, cGroupName);

    fprintf(f,
	    "\n"
	    "    out = stls_snmp_getnext(s, in);\n"
	    "    stls_vbl_free(in);\n"
	    "    if (! out) {\n"
	    "        return -2;\n"
	    "    }\n"
	    "\n");
    
    fprintf(f,
	    "    *%s = assign_%s(out);\n"
	    "\n", cGroupName, cGroupName);

    fprintf(f,
	    "    return 0;\n"
	    "}\n"
	    "\n");

    xfree(cGroupName);
    xfree(cModuleName);
}



static void
printNewMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    char *cModuleName, *cGroupName;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    fprintf(f,
	    "%s_%s_t *\n"
	    "%s_new_%s()\n"
	    "{\n"
	    "    %s_%s_t *%s;\n"
	    "\n",
	    cModuleName, cGroupName,
	    cModuleName, cGroupName,
	    cModuleName, cGroupName, cGroupName);

    fprintf(f,
	    "    %s = (%s_%s_t *) g_malloc0(sizeof(%s_%s_t) + sizeof(gpointer));\n"
	    "    return %s;\n"
	    "}\n"
	    "\n",
	    cGroupName, cModuleName, cGroupName,
	    cModuleName, cGroupName, cGroupName);

    xfree(cGroupName);
    xfree(cModuleName);
}



static void
printFreeTableMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    SmiNode *tableNode;
    char *cModuleName, *cGroupName, *cTableName;

    tableNode = smiGetParentNode(groupNode);
    if (! tableNode) {
	return;
    }

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);
    cTableName = translate(tableNode->name);

    fprintf(f,
	    "void\n"
	    "%s_free_%s(%s_%s_t **%s)\n"
	    "{\n"
	    "    int i;\n"
	    "\n",
	    cModuleName, cTableName, cModuleName, cGroupName, cGroupName);

    fprintf(f,	    
	    "    if (%s) {\n"
	    "        for (i = 0; %s[i]; i++) {\n"
	    "            %s_free_%s(%s[i]);\n"
	    "        }\n"
	    "        g_free(%s);\n"
	    "    }\n"
	    "}\n"
	    "\n",
	    cGroupName, cGroupName, cModuleName,
	    cGroupName, cGroupName, cGroupName);

    xfree(cTableName);
    xfree(cGroupName);
    xfree(cModuleName);
}
 



static void
printFreeMethod(FILE *f, SmiModule *smiModule, SmiNode *groupNode)
{
    char *cModuleName, *cGroupName;

    cModuleName = translateLower(smiModule->name);
    cGroupName = translate(groupNode->name);

    fprintf(f,
	    "void\n"
	    "%s_free_%s(%s_%s_t *%s)\n"
	    "{\n"
	    "    GSList *vbl;\n"
	    "    char *p;\n"
	    "\n",
	    cModuleName, cGroupName, cModuleName, cGroupName, cGroupName);

    fprintf(f,	    
	    "    if (%s) {\n"
	    "        p = (char *) %s + sizeof(%s_%s_t);\n"
	    "        vbl = * (GSList **) p;\n"
	    "        stls_vbl_free(vbl);\n"
	    "        g_free(%s);\n"
	    "    }\n"
	    "}\n"
	    "\n",
	    cGroupName, cGroupName, cModuleName, cGroupName, cGroupName);

    xfree(cGroupName);
    xfree(cModuleName);
}
 



static void
printStubMethods(FILE *f, SmiModule *smiModule)
{
    SmiNode   *smiNode;
    int       cnt = 0;
    
    for (smiNode = smiGetFirstNode(smiModule, SMI_NODEKIND_ANY);
	 smiNode;
	 smiNode = smiGetNextNode(smiNode, SMI_NODEKIND_ANY)) {
	if (isGroup(smiNode) && isAccessible(smiNode)) {
	    cnt++;
	    printNewMethod(f, smiModule, smiNode);
	    printAssignMethod(f, smiModule, smiNode);
	    if (smiNode->nodekind == SMI_NODEKIND_ROW) {
		printGetTableMethod(f, smiModule, smiNode);
	    } else {
		printGetScalarsMethod(f, smiModule, smiNode);
	    }
	    printFreeMethod(f, smiModule, smiNode);
	    if (smiNode->nodekind == SMI_NODEKIND_ROW) {
		printFreeTableMethod(f, smiModule, smiNode);
	    }
	}
    }
    
    if (cnt) {
	fprintf(f, "\n");
    }
}



static void
dumpStubs(SmiModule *smiModule, char *baseName)
{
    FILE *f;

    f = createFile(baseName, ".c");
    if (! f) {
        return;
    }

    printTopComment(f, smiModule);

    fprintf(f,
	    "#include \"%s.h\"\n"
	    "\n",
	    baseName);

    printStubEnumerations(f, smiModule);
    printStubIdentities(f, smiModule);
    printStubAttributes(f, smiModule);
    printStubMethods(f, smiModule);
    
    fclose(f);
}



static void
dumpStools(int modc, SmiModule **modv, int flags, char *output)
{
    char	*baseName;
    int		i;

    if (flags & SMIDUMP_FLAG_UNITE) {
	/* not implemented yet */
    } else {
	for (i = 0; i < modc; i++) {
	    baseName = output ? output : translateFileName(modv[i]->name);
	    dumpHeader(modv[i], baseName);
	    dumpStubs(modv[i], baseName);
	    if (! output) xfree(baseName);
	}
    }

}



void initStools()
{
    static SmidumpDriver driver = {
	"stools",
	dumpStools,
	0,
	SMIDUMP_DRIVER_CANT_UNITE,
	"ANSI C manager stubs for the stools package",
	NULL,
	NULL
    };

    smidumpRegisterDriver(&driver);
}
