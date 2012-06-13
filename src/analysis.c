/*
 * Copyright (C) 2003-2012 bocelli.hu <bocelli.hu@gmail.com>
 * 
 * This file is part of JSAnalysis, the Javascript Static Code Analysis Engine.
 * 
 * JSAnalysis is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * JSAnalysis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with JSAnalysis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdlib.h"
#include "stdio.h"
#include <jsapi.h>
#include <jsparse.h>
#include <jsscan.h>
#include <jsatom.h>
#include "jscntxt.h"
#include <jsstr.h>
#include <jsfun.h>
#include <jsscope.h>
#include <jsinterp.h>
#include <jsstddef.h>
#include <jsprf.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#ifdef XP_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     /* for isatty() */
#endif

#include "analysis.h"
const char * TOKENS[] = {
  "EOF", "EOL", "SEMI", "COMMA", "ASSIGN", "HOOK", "COLON", "OR", "AND",
  "BITOR", "BITXOR", "BITAND", "EQOP", "RELOP", "SHOP", "PLUS", "MINUS", "STAR",
  "DIVOP", "UNARYOP", "INC", "DEC", "DOT", "LB", "RB", "LC", "RC", "LP", "RP",
  "NAME", "NUMBER", "STRING", "OBJECT", "PRIMARY", "FUNCTION", "EXPORT",
  "IMPORT", "IF", "ELSE", "SWITCH", "CASE", "DEFAULT", "WHILE", "DO", "FOR",
  "BREAK", "CONTINUE", "IN", "VAR", "WITH", "RETURN", "NEW", "DELETE",
  "DEFSHARP", "USESHARP", "TRY", "CATCH", "FINALLY", "THROW", "INSTANCEOF",
  "DEBUGGER", "XMLSTAGO", "XMLETAGO", "XMLPTAGC", "XMLTAGC", "XMLNAME",
  "XMLATTR", "XMLSPACE", "XMLTEXT", "XMLCOMMENT", "XMLCDATA", "XMLPI", "AT",
  "DBLCOLON", "ANYNAME", "DBLDOT", "FILTER", "XMLELEM", "XMLLIST",
  "YIELD","ARRAYCOMP","ARRAYPUSH","LEXICALSCOPE","TOK_LET","TOK_BODY",
  "RESERVED",  "LIMIT",
};

const char * TYPES[7] = {
    "FUNC",     //= -3,
    "LIST",     //= -2,
    "NAME",     //= -1,
    "NULLARY",  //=  0
    "UNARY",    //=  1,
    "BINARY",   //=  2,
    "TERNARY",  //=  3,
};

const char * OP_NAMES[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    name,
#include "jsopcode.tbl"
#undef OPDEF
};

const int NUM_TOKENS = sizeof(TOKENS) / sizeof(TOKENS[0]);


static void traceTree(JSContext *cx, JSParseNode * root, int indent);
static void traceTokenStream(JSContext *cx, JSTokenStream *ts);
static JSBool parseNodeSetObj(JSContext *cx, JSParseNode * root, JSObject *obj);
static JSBool parseTokenStreamSetObj(JSContext *cx, JSTokenStream *ts, JSObject *arr);
static JSBool jsfunc_parse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool jsfunc_scanToken(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSFunctionSpec _functions[] = {
    {"parse", jsfunc_parse, 1, 0, 0},
    {"scan_token", jsfunc_scanToken, 1, 0, 0},
};

JSBool js_InitAnalysisFunctions(JSContext *cx, JSObject* obj) {
    return JS_DefineFunctions(cx, obj, _functions);
}

static JSErrorFormatString _ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, number, count, exception, format) \
    { format, count, JSEXN_ERR } ,
#include "analysis.msg"
#undef MSG_DEF
};
const JSErrorFormatString *getErrorMessage(void *userRef, const char *locale, const uintN errorNumber) {
    if ((errorNumber > 0) && (errorNumber < JSAnalysisErr_Limit))
        return &_ErrorFormatString[errorNumber];
    return NULL;
}

static JSBool _parse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, JSBool scanOnly) {
    
    JSString *path;
    JSTokenStream *token_stream;
    JSParseNode *node;
    FILE *file;
    JSBool ret;
    
    JS_MaybeGC(cx);
    file = NULL;
    if(argc < 1) {
        file = stdin;
    } else {

        path = JS_ValueToString(cx, argv[0]);
        if (!path) {
            JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                                 JSAMSG_FIRST_ARGUMENT_NOT_STRING_ERROR,
                                 "func");
            goto err;
        }
    
        file = fopen(JS_GetStringBytes(path), "r+");
        if(file == NULL) {
            JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                                 JSAMSG_CANNOT_OPEN_ERROR,
                                 JS_GetStringBytes(path));
            goto err;
        }
    }

    if (!isatty(fileno(file))) {
        SKIP_UTF8_BOM(file);

        token_stream = js_NewFileTokenStream(cx, NULL, file);
        if (token_stream == NULL) {
            goto err;
        }

        if(scanOnly) {
            JSObject *list = JS_NewArrayObject(cx, 0, NULL);
            if(list == NULL) {
                goto err;
            }
            *rval = OBJECT_TO_JSVAL(list);
            if(!parseTokenStreamSetObj(cx, token_stream, list)) {
                goto err;
            }
            //traceTokenStream(cx, token_stream);
        } else {
            node = js_ParseTokenStream(cx, obj, token_stream);
            if (node == NULL) {
                goto err;
            }

            //traceTree(cx, node, 0);
            JSObject *tree = JS_NewObject(cx, NULL, NULL, obj);
            if(tree == NULL) {
                goto err;
            }
            *rval = OBJECT_TO_JSVAL(tree);
            if(!parseNodeSetObj(cx, node, tree)) {
                goto err;
            }
        }
    } 
    ret = JS_TRUE;
    goto out;
err: 
    ret = JS_FALSE;
out:
    if(file != NULL && file != stdin) fclose(file);
    return ret;
}

static JSBool jsfunc_parse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return _parse(cx, obj, argc, argv, rval, JS_FALSE);
}

static JSBool jsfunc_scanToken(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return _parse(cx, obj, argc, argv, rval, JS_TRUE);
}

#define RETURN_IF_NULL(N) do {if(N == NULL) return JS_FALSE;} while(0)
#define RETURN_IF_FALSE(N, MSG) do {if(!(N)) { fprintf(stderr, MSG); return JS_FALSE;}} while(0)
#define SET_VAL_PROPERTY(OBJ, NM, VAL) do { if(!JS_SetProperty(cx, OBJ, NM, VAL)) { fprintf(stderr, "can't set property %s\n", NM); return JS_FALSE;}} while(0) 
#define SET_PROPERTY(OBJ, NM, VAL) do { jsval val = VAL; SET_VAL_PROPERTY(OBJ, NM, &val);} while(0) 
#define SET_DOUBLE_PROPERTY(OBJ, NM, DB) do { jsval val; if(!JS_NewNumberValue(cx, DB, &val)) return JS_FALSE; SET_VAL_PROPERTY(OBJ, NM, &val);} while(0) 
#define SET_ELEMENT(OBJ, INDEX, VAL) do { jsval val = VAL;if(!JS_SetElement(cx, OBJ, INDEX, &val)) { fprintf(stderr, "can't set element\n"); return JS_FALSE;}} while(0)
#define CONVERT_ATOM_TO_STRING(ATOM) JS_ValueToString(cx, ATOM_KEY(ATOM))
#define SET_ATOM_STR_PROPERTY(OBJ, NM, ATOM) SET_PROPERTY(OBJ, NM, STRING_TO_JSVAL(CONVERT_ATOM_TO_STRING(ATOM)))
#define SET_ATOM_STR_ELEMENT(OBJ, INDEX, ATOM) SET_ELEMENT(OBJ, INDEX, STRING_TO_JSVAL(CONVERT_ATOM_TO_STRING(ATOM)))
static JSBool parseNodeSetObj(JSContext *cx, JSParseNode * root, JSObject *obj) {
    if (root == NULL) {
        return JS_FALSE;
    }
  
    SET_PROPERTY(obj, "type", INT_TO_JSVAL(root->pn_type));
    SET_PROPERTY(obj, "op", INT_TO_JSVAL(root->pn_op));
    SET_PROPERTY(obj, "arity", INT_TO_JSVAL(root->pn_arity+3));
    if(root->pn_type == TOK_NUMBER) {
        SET_DOUBLE_PROPERTY(obj, "dval", root->pn_dval);
    }

    SET_PROPERTY(obj, "offset", INT_TO_JSVAL(root->pn_offset));
        
    SET_PROPERTY(obj, "begin_index", INT_TO_JSVAL(root->pn_pos.begin.index));
    SET_PROPERTY(obj, "begin_lineno", INT_TO_JSVAL(root->pn_pos.begin.lineno));
    SET_PROPERTY(obj, "end_index", INT_TO_JSVAL(root->pn_pos.end.index));
    SET_PROPERTY(obj, "end_lineno", INT_TO_JSVAL(root->pn_pos.end.lineno));

    if(root->pn_source) SET_ATOM_STR_PROPERTY(obj, "source", root->pn_source);
    // @see jsparse.h L53 to L261
    switch(root->pn_arity) {
    case PN_FUNC:
        {
        JSObject *funcobj = ATOM_TO_OBJECT(root->pn_funAtom);
        JSFunction * function = (JSFunction *) JS_GetPrivate(cx, funcobj);
        if (function->atom) {
            SET_ATOM_STR_PROPERTY(obj, "name", function->atom);
        }
        JSScopeProperty * scope_property;
        JSScope * scope = OBJ_SCOPE(funcobj);
        int argc = 0;
#define loop_scope \
        for (scope_property = SCOPE_LAST_PROP(scope);  \
            scope_property != NULL;                    \
            scope_property = scope_property->parent)  
#define check_scope_property \
            if (scope_property->getter != js_GetArgument) { \
                continue;                                   \
            }                                               

        loop_scope {
            check_scope_property
            argc++;  
        }
        
        JSObject *args = JS_NewArrayObject(cx, argc, NULL);
        RETURN_IF_NULL(args);
        SET_PROPERTY(obj, "args", OBJECT_TO_JSVAL(args));
        loop_scope {
            check_scope_property
            SET_ATOM_STR_ELEMENT(args, scope_property->shortid, JSID_TO_ATOM(scope_property->id));
        }
        
#undef loop_scope
#undef check_scope_property

        JSObject *body = JS_NewObject(cx, NULL, NULL, obj);
        RETURN_IF_NULL(body);
        SET_PROPERTY(obj, "body", OBJECT_TO_JSVAL(body));
        if(!parseNodeSetObj(cx, root->pn_body, body)) return JS_FALSE;
        }
        break; 
    case PN_LIST:
        {
        JSParseNode *p;
        JSObject *list = JS_NewArrayObject(cx, root->pn_count, NULL);
        RETURN_IF_NULL(list);
        SET_PROPERTY(obj, "list", OBJECT_TO_JSVAL(list));
        if(root->pn_count == 0) return JS_TRUE;
        jsint index = 0;
        for (p = root->pn_head; p != NULL; p = p->pn_next) {
            JSObject *element = JS_NewObject(cx, NULL, NULL, list);
            RETURN_IF_NULL(element);
            SET_ELEMENT(list, index, OBJECT_TO_JSVAL(element));
            if(!parseNodeSetObj(cx, p, element)) return JS_FALSE;
            index ++;
        }
        }
        break;
    case PN_TERNARY:
        if(root->pn_kid1) {
            JSObject *kid1 = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(kid1);
            SET_PROPERTY(obj, "kid1", OBJECT_TO_JSVAL(kid1));
            if(!parseNodeSetObj(cx, root->pn_kid1, kid1)) return JS_FALSE;
        }
        if(root->pn_kid2) {
            JSObject *kid2 = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(kid2);
            SET_PROPERTY(obj, "kid2", OBJECT_TO_JSVAL(kid2));
            if(!parseNodeSetObj(cx, root->pn_kid2, kid2)) return JS_FALSE;
        }
        if(root->pn_kid3) {
            JSObject *kid3 = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(kid3);
            SET_PROPERTY(obj, "kid3", OBJECT_TO_JSVAL(kid3));
            if(!parseNodeSetObj(cx, root->pn_kid3, kid3)) return JS_FALSE;
        }
        break;
    case PN_BINARY:
        if(root->pn_left) {
            JSObject *l = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(l);
            SET_PROPERTY(obj, "left", OBJECT_TO_JSVAL(l));
            if(!parseNodeSetObj(cx, root->pn_left, l)) return JS_FALSE;
        }
        if(root->pn_right) {
            JSObject *r = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(r);
            SET_PROPERTY(obj, "right", OBJECT_TO_JSVAL(r));
            if(!parseNodeSetObj(cx, root->pn_right, r)) return JS_FALSE;
        }
        break;
    case PN_UNARY:
        if(root->pn_kid) {
            JSObject *kid = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(kid);
            SET_PROPERTY(obj, "kid", OBJECT_TO_JSVAL(kid));
            if(!parseNodeSetObj(cx, root->pn_kid, kid)) return JS_FALSE;
        }
        break;
    case PN_NAME:
        if(root->pn_expr != 0) {
            JSObject *expr = JS_NewObject(cx, NULL, NULL, obj); RETURN_IF_NULL(expr);
            SET_PROPERTY(obj, "expr", OBJECT_TO_JSVAL(expr));
            if(!parseNodeSetObj(cx, root->pn_expr, expr)) return JS_FALSE;
        }
        if(root->pn_atom != 0 && root->pn_type != TOK_LEXICALSCOPE) {
            SET_ATOM_STR_PROPERTY(obj, "name", root->pn_atom);
        }
        break;
    case PN_NULLARY:
        if(root->pn_type == TOK_STRING || root->pn_type == TOK_NAME || root->pn_type == TOK_OBJECT || root->pn_type == TOK_BREAK || root->pn_type == TOK_CONTINUE) {
            if(root->pn_atom != 0) {
                SET_ATOM_STR_PROPERTY(obj, "value", root->pn_atom);
            }
        } else if(root->pn_type == TOK_PRIMARY) {
            SET_PROPERTY(obj, "value", INT_TO_JSVAL(root->pn_op));
        } else if(root->pn_type == TOK_NUMBER) {
            SET_DOUBLE_PROPERTY(obj, "value", root->pn_dval);
        } else {
            fprintf(stderr, "Unknown parse node type(%d) in NULLARY\n", root->pn_type);
        }
        break;
    default:
        fprintf(stderr, "Unknown node type\n");
        break;
    }
    return JS_TRUE;
}
static JSBool parseTokenStreamSetObj(JSContext *cx, JSTokenStream *ts, JSObject *arr) {

    JSTokenType type;
    JSTokenPos  pos;
    JSToken token;
    JSObject *obj;
    int index;
    index = 0;
    while(1) {
        type = js_GetToken(cx, ts);
        if(type <= 0) break;

        token = CURRENT_TOKEN(ts);
        pos = token.pos;
        
        obj = JS_NewObject(cx, NULL, NULL, arr); RETURN_IF_NULL(obj);
        SET_ELEMENT(arr, index, OBJECT_TO_JSVAL(obj));

        SET_PROPERTY(obj, "type", INT_TO_JSVAL(type));
        SET_PROPERTY(obj, "op", INT_TO_JSVAL(token.t_op));

        if(type == TOK_NUMBER) {
            SET_DOUBLE_PROPERTY(obj, "value", token.t_dval);
        } else { 
            if(token.t_atom) {
                SET_ATOM_STR_PROPERTY(obj, "value", token.t_atom);
            }
        }
        

        SET_PROPERTY(obj, "begin_index", INT_TO_JSVAL(pos.begin.index));
        SET_PROPERTY(obj, "begin_lineno", INT_TO_JSVAL(pos.begin.lineno));
        SET_PROPERTY(obj, "end_index", INT_TO_JSVAL(pos.end.index));
        SET_PROPERTY(obj, "end_lineno", INT_TO_JSVAL(pos.end.lineno));

        index++;
    }
    return JS_TRUE; 
}

static void traceTokenStream(JSContext *cx, JSTokenStream *ts) {

    JSTokenType type;
    JSTokenPos  pos;
    JSToken token;
    while(1) {
        type = js_GetToken(cx, ts);
        if(type <= 0) break;

        token = CURRENT_TOKEN(ts);
        pos = token.pos;
        printf("%s: starts at L%d C%d, ends at L%d C%d op %s dval=%f", TOKENS[type],
               pos.begin.lineno, pos.begin.index, pos.end.lineno, pos.end.index,
               OP_NAMES[token.t_op], token.t_dval);
        if(token.t_atom) printf(" atom=%s", JS_GetStringBytes(CONVERT_ATOM_TO_STRING(token.t_atom)));
        printf("\n");
    }
}
static void traceTree(JSContext *cx, JSParseNode * root, int indent) {
  if (root == NULL) {
    return;
  }
  printf("%*s", indent, "");
  if (root->pn_type >= NUM_TOKENS) {
    printf("UNKNOWN(%d)", root->pn_type);
  }
  else {
    printf("%s: starts at line %d, column %d, ends at line %d, column %d, type %s, op %s",
           TOKENS[root->pn_type],
           root->pn_pos.begin.lineno, root->pn_pos.begin.index,
           root->pn_pos.end.lineno, root->pn_pos.end.index, TYPES[root->pn_arity+3], OP_NAMES[root->pn_op]);
    if(root->pn_arity == PN_NAME) {
        if(root->pn_type == TOK_STRING) printf(" name:%s", JS_GetStringBytes(ATOM_TO_STRING(root->pn_atom)));
        else if(root->pn_type == TOK_LEXICALSCOPE) {
            printf("\n");
            traceTree(cx, root->pn_expr, indent + 2);
        } else if(root->pn_type == TOK_NAME) {
            printf(" name:%s", JS_GetStringBytes(ATOM_TO_STRING(root->pn_atom)));
            printf("\n");
            traceTree(cx, root->pn_expr, indent + 2);
        }
    } else if (root->pn_arity == PN_FUNC) {
        JSObject *object = ATOM_TO_OBJECT(root->pn_funAtom);
        JSFunction * function = (JSFunction *) JS_GetPrivate(cx, object);
        if (function->atom) {
            printf(" name:%s", JS_GetStringBytes(ATOM_TO_STRING(function->atom)));
        }
        JSScopeProperty * scope_property;
        JSScope * scope = OBJ_SCOPE(object);
        for (scope_property = SCOPE_LAST_PROP(scope);
             scope_property != NULL;
             scope_property = scope_property->parent) {
          if (scope_property->getter != js_GetArgument) {
            continue;
          }
          printf(" param%d=%s", scope_property->shortid, JS_GetStringBytes(ATOM_TO_STRING(JSID_TO_ATOM(scope_property->id))));
        }
    } else if (root->pn_arity == PN_NULLARY) {
        //printf(" valued:%f", root->pn_dval); 
        //printf(" valued:%x", root->pn_val); 
        
        if(root->pn_type == TOK_STRING) printf(" name:%s", JS_GetStringBytes(ATOM_TO_STRING(root->pn_atom)));
        else if(root->pn_type == TOK_PRIMARY) printf(" name:%s", OP_NAMES[root->pn_op]);
        else if(root->pn_type == TOK_NUMBER) printf(" name:%f", root->pn_dval);
    }
  }
  printf("\n");
  switch (root->pn_arity) {
  case PN_UNARY:
    traceTree(cx, root->pn_kid, indent + 2);
    break;
  case PN_BINARY:
    traceTree(cx, root->pn_left, indent + 2);
    traceTree(cx, root->pn_right, indent + 2);
    break;
  case PN_TERNARY:
    traceTree(cx, root->pn_kid1, indent + 2);
    traceTree(cx, root->pn_kid2, indent + 2);
    traceTree(cx, root->pn_kid3, indent + 2);
    break;
  case PN_LIST:
    {
      JSParseNode *p;
      for (p = root->pn_head; p != NULL; p = p->pn_next) {
        traceTree(cx, p, indent + 2);
      }
    }
    break;
  case PN_FUNC:
    traceTree(cx, root->pn_body, indent + 2);
    break; 
  case PN_NAME:
  case PN_NULLARY:
    break;
  default:
    fprintf(stderr, "Unknown node type\n");
    exit(EXIT_FAILURE);
    break;
  }
}
