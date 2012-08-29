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
#include <sys/stat.h> /* struct stat, fchmod (), stat (), S_ISREG, S_ISDIR */
#endif

#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     /* for isatty() */
#endif

#include "analysis.h" 

typedef enum JSShellExitCode {
    EXITCODE_RUNTIME_ERROR      = 3,
    EXITCODE_VALID_PARAMS       = 4,
    EXITCODE_OUT_OF_MEMORY      = 5
} JSShellExitCode;
typedef struct LibraryCollection {
    char path[PATH_MAX];
    struct LibraryCollection *next;
} LibraryCollection;

static JSBool processBytes(JSContext *cx, JSObject *obj, const char *bytes);
static JSBool processFile(JSContext *cx, JSObject *obj, const char *filename, jsval *result);

static JSBool jsfunc_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool jsfunc_println(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool jsfunc_use(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool jsfunc_execFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSFunctionSpec std_functions[] = {
    {"print", jsfunc_print, 0, 0, 0},
    {"println", jsfunc_println, 0, 0, 0},
    {"exec_file", jsfunc_execFile, 0, 0, 0},
    {"use", jsfunc_use, 1, 0, 0},
    {0}
};

int gExitCode;
FILE *gErrFile;
static JSBool reportWarnings;
JSString *baseDir;
LibraryCollection *firstLibrary, *lastLibrary;
JSObject *files;
int filesIndex;

static void errorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
    int i, j, k, n;
    char *prefix, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    prefix = NULL;
    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix)
            fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }

    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    /* report->linebuf usually ends with a newline. */
    n = strlen(report->linebuf);
    fprintf(gErrFile, ":\n%s%s%s%s",
            prefix,
            report->linebuf,
            (n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n",
            prefix);
    n = PTRDIFF(report->tokenptr, report->linebuf, char);
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags)) {
        if (report->errorNumber == JSMSG_OUT_OF_MEMORY) {
            gExitCode = EXITCODE_OUT_OF_MEMORY;
        } else {
            gExitCode = EXITCODE_RUNTIME_ERROR;
        }
    }
    JS_free(cx, prefix);
}

static int usage(void) {
    fprintf(gErrFile, "%s\n", JS_GetImplementationVersion());
    fprintf(gErrFile, "%s\n", "usage: js_analysis [-w] [-s] [-l library floder] [-e script] scriptfile");
    fprintf(gErrFile, "%s\n", "-w: print warning messages.");
    fprintf(gErrFile, "%s\n", "-s: execute script from standard input.");
    fprintf(gErrFile, "%s\n", "-l: add directory dir to the list of directories to be searched for use");
    fprintf(gErrFile, "%s\n", "-e: execute script from string.");
#if 0
    fprintf(gErrFile, "%s\n", "versions:");
    fprintf(gErrFile, "  %d:%s\n",100,"JavaScript 1.0    ");
    fprintf(gErrFile, "  %d:%s\n",110,"JavaScript 1.1    ");
    fprintf(gErrFile, "  %d:%s\n",120,"JavaScript 1.2    ");
    fprintf(gErrFile, "  %d:%s\n",130,"JavaScript 1.3    ");
    fprintf(gErrFile, "  %d:%s\n",140,"JavaScript 1.4    ");
    fprintf(gErrFile, "  %d:%s\n",148,"ECMA 262 Edition 3");
    fprintf(gErrFile, "  %d:%s\n",150,"JavaScript 1.5    ");
    fprintf(gErrFile, "  %d:%s\n",160,"JavaScript 1.6    ");
    fprintf(gErrFile, "  %d:%s\n",170,"JavaScript 1.7    ");
#endif
    return EXITCODE_VALID_PARAMS;
}

static JSBool initGlobalObject(JSContext *cx, JSObject *global) {
    
    jsval propertyval;
    
    propertyval = STRING_TO_JSVAL(baseDir);
    if(!JS_SetProperty(cx, global, "__base_dir", &propertyval)) {
        fprintf(gErrFile, "%s\n", "can't set property to gobal object.");
        return JS_FALSE;
    }
   
    propertyval = OBJECT_TO_JSVAL(global);
    if(!JS_SetProperty(cx, global, "__global", &propertyval)) {
        fprintf(gErrFile, "%s\n", "can't set property to gobal object.");
        return JS_FALSE;
    }

    propertyval = OBJECT_TO_JSVAL(files);
    if(!JS_SetProperty(cx, global, "__files", &propertyval)) {
        fprintf(gErrFile, "%s\n", "can't set property to gobal object.");
        return JS_FALSE;
    }
    
    return JS_TRUE;
}

int main(int argc, char **argv) {
    JSContext *context;
    JSRuntime *runtime;
    JSObject *global;
    jsval result;
    int i;
    char path[PATH_MAX], *p;
    
    files = NULL;
    gErrFile = stderr;
    reportWarnings = JS_FALSE;
    gExitCode = 0;
    firstLibrary = lastLibrary = 0;
    filesIndex=0;

    strcpy(path, argv[0]);
    for(p=path;*p;p++);
    for(;p!=path;p--) {
        if(*p == '/') {
            *p = 0;
            break;
        }
    }
    
    runtime = JS_NewRuntime(1024L * 1024L * 1024L);
    if (runtime == NULL) {
        fprintf(stderr, "cannot create runtime");
        exit(EXIT_FAILURE);
    }

    context = JS_NewContext(runtime, 8192);
    if (context == NULL) {
        fprintf(stderr, "cannot create context");
        exit(EXIT_FAILURE);
    }
    baseDir = JS_NewStringCopyZ(context, path);
    if(!baseDir) {
        fprintf(stderr, "%s\n", "out of memory.");
        exit(EXIT_FAILURE);
    }
    JS_SetErrorReporter(context, errorReporter);
    global = JS_NewObject(context, NULL, NULL, NULL);
    if (global == NULL) {
        fprintf(stderr, "cannot create global object");
        exit(EXIT_FAILURE);
    }
    if (!JS_InitStandardClasses(context, global)) {
        fprintf(stderr, "cannot initialize standard classes");
        exit(EXIT_FAILURE);
    }
#if JS_HAS_FILE_OBJECT
    if(!js_InitFileClass(context, global)) {
        fprintf(stderr, "cannot initialize file class");
        exit(EXIT_FAILURE);
    }
#endif /* JS_HAS_FILE_OBJECT */
    files = JS_NewArrayObject(context, 0, NULL);
    if (files == NULL) {
        fprintf(stderr, "cannot create files object");
        exit(EXIT_FAILURE);
    }
    if (!initGlobalObject(context, global)) {
        fprintf(stderr, "cannot initialize global object");
        exit(EXIT_FAILURE);
    }
    if (!JS_DefineFunctions(context, global, std_functions)) {
        fprintf(stderr, "cannot initialize standard functions");
        exit(EXIT_FAILURE);
    }
    if (!js_InitAnalysisFunctions(context, global)) {
        fprintf(stderr, "cannot initialize analysis functions");
        exit(EXIT_FAILURE);
    }
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            processFile(context, global, argv[i], &result);
            continue;
        }
        switch (argv[i][1]) {
        case 'v':
            if (++i == argc)
                return usage();

            int ver = atoi(argv[i]);
            if(ver < 100 || ver > 170) 
                return usage();
            JS_SetVersion(context, (JSVersion) ver);
            break;
        case 'w':
        case 'W':
            reportWarnings = JS_TRUE;
            break;
        case 's':
            processFile(context, global, NULL, &result);
            break;
        case 'e':
            if (++i == argc)
                return usage();

            processBytes(context, global, argv[i]);
            break;
        case 'l':
            if (++i == argc)
                return usage();

            {
            LibraryCollection *l = (LibraryCollection*) malloc(sizeof(LibraryCollection));
            l->next = 0;
            strcpy(l->path, argv[i]);
            if(!lastLibrary) {
                firstLibrary = lastLibrary = l;
            } else {
                lastLibrary->next = l;
                lastLibrary=l;
            }
            }
            break;
        default:
            return usage();
        }
    }

    JS_DestroyContext(context);
    JS_DestroyRuntime(runtime);
    if(firstLibrary) {
        LibraryCollection *l,*n;
        l = firstLibrary;
        while(l) {
            n = l->next;
            free(l);
            l = n;
        }
        firstLibrary = lastLibrary = 0;
    }

    return gExitCode;
}

static JSBool jsfunc_println(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

    if(jsfunc_print(cx, obj, argc, argv, rval)) {
        fputc('\n', stdout);
        return JS_TRUE;
    } else {
        return JS_FALSE;
    }
}
static JSBool jsfunc_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    uintN i;
    JSString *str;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        fprintf(stdout, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    return JS_TRUE;
}
static JSBool jsfunc_use(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSString *filenamestr;
    JSBool ret;
    jsval *result;

    if(argc < 1) {
        JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                             JSAMSG_MORE_ARGS_NEEDED,
                             "use", "1");
        goto err;
    }
    filenamestr = JS_ValueToString(cx, argv[0]);
    if (!filenamestr) {
        JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                             JSAMSG_FIRST_ARGUMENT_NOT_STRING_ERROR,
                             "use");
        goto err;
    }
    
    {
        char path[PATH_MAX];
        struct stat sts;
        char *filename;
#define file_exists(p) (stat(p, &sts) != -1 && S_ISREG(sts.st_mode))
        
        filename = JS_GetStringBytes(filenamestr);
        strcpy(path, JS_GetStringBytes(baseDir));
        strcat(path, "/lib/");
        strcat(path, filename);
        strcat(path, ".js");
        
        if (!file_exists(path)) {
            LibraryCollection *l;
            l = firstLibrary;
            while(l) {
                strcpy(path, l->path);
                strcat(path, "/");
                strcat(path, filename);
                strcat(path, ".js");
                if(file_exists(path)) break;
                l = l->next;
            } 
        }
        if(!processFile(cx, obj, path, &result))
            goto err;
#undef file_not_exists
         
    }

    *rval = JSVAL_TRUE;
    ret = JS_TRUE;
    goto out;
err: 
    *rval = JSVAL_FALSE;
    ret = JS_FALSE;
out:
    return ret;
}
static JSBool jsfunc_execFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    char *filename;
    JSBool ret;

    if(argc < 1) {
        filename = NULL;
    } else {
        JSString *filenameStr;
        filenameStr = JS_ValueToString(cx, argv[0]);
        if (!filenameStr) {
            JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                                 JSAMSG_FIRST_ARGUMENT_NOT_STRING_ERROR,
                                 "execFile");
            goto err;
        }
        filename = JS_GetStringBytes(filenameStr);
    }
    
    if(!processFile(cx, obj, filename, rval))
        goto err;

    ret = JS_TRUE;
    goto out;
err: 
    ret = JS_FALSE;
out:
    return ret;
}
#define DEFAULT_SCRIPT_FILE_NAME "inline script"
static JSBool pushFileName(JSContext *cx, const char* filename) {
    JSString *filenameStr;
    jsval property;
    if(filename) {
        filenameStr = JS_NewStringCopyZ(cx, filename);
        property = STRING_TO_JSVAL(filenameStr);
        if(!JS_SetElement(cx, files, filesIndex, &property)) {
            fprintf(gErrFile, "%s\n", "can't add file to files object.");
            return JS_FALSE;
        }
    }
    filesIndex++;
    return JS_TRUE;
}
static JSBool popFileName(JSContext *cx) {
     
    filesIndex--;
    if(!JS_SetArrayLength(cx, files, filesIndex)) {
        filesIndex++;
        fprintf(gErrFile, "%s\n", "can't remove file to files object.");
        return JS_FALSE;
    }
    return JS_TRUE;
}
static JSBool processBytes(JSContext *cx, JSObject *obj, const char *bytes) {
    JSScript *script;
    jsval result;

    script = JS_CompileScript(cx, obj, bytes, strlen(bytes), DEFAULT_SCRIPT_FILE_NAME, 0);
    if (script) {
        pushFileName(cx, NULL);
        if(JS_ExecuteScript(cx, obj, script, &result) == JS_FALSE) {
            fprintf(stderr, "can't execute inline string\n");
            popFileName(cx);
            JS_DestroyScript(cx, script);
            return JS_FALSE;
        }
        popFileName(cx);
        JS_DestroyScript(cx, script);
    } else return JS_FALSE;

    return JS_TRUE;
}
static JSBool processFile(JSContext *cx, JSObject *obj, const char *filename, jsval *result) {
    JSScript *script;
    FILE *file;

    if (!filename) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            JS_ReportErrorNumber(cx, getErrorMessage, NULL,
                             JSAMSG_CANNOT_OPEN_ERROR,
                             filename);
            return JS_FALSE;
        }
    }

    if (!isatty(fileno(file))) {
        SKIP_UTF8_BOM(file);
        SKIP_SHELL_COMMENT(file);

        script = JS_CompileFileHandle(cx, obj, filename, file);
        if (script) {
            pushFileName(cx, filename);
            if(JS_ExecuteScript(cx, obj, script, result) == JS_FALSE) {
                fprintf(stderr, "can't execute file %s\n", (filename?filename:"standard input"));
                popFileName(cx);
                JS_DestroyScript(cx, script);
                return JS_FALSE;
            }
            popFileName(cx);
            JS_DestroyScript(cx, script);
        } else return JS_FALSE;

        return JS_TRUE;
    }

    return JS_TRUE;
}


