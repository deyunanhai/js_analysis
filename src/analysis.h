/*
    This file is part of JSAnalysis, the Javascript Static Code Analysis Engine.
    Copyright (c) 2011 Hu Hua (bocelli.hu@gmail.com)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

*/

#ifndef analysis_h___
#define analysis_h___

typedef enum JSAnalysisErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "analysis.msg"
#undef MSG_DEF
    JSAnalysisErr_Limit
#undef MSGDEF
} JSAnalysisErrNum;

#define SKIP_UTF8_BOM(_FILE_)                   \
    do {                                        \
        int ch = fgetc(_FILE_);                 \
        if(ch == 0xEF) {                        \
            int ch2 = fgetc(_FILE_);            \
            if(ch2 != 0xBB) {                   \
                ungetc(ch2,_FILE_);             \
                ungetc(ch,_FILE_);              \
            } else {                            \
                int ch3 = fgetc(_FILE_);        \
                if(ch3 != 0xBF) {               \
                    ungetc(ch3,_FILE_);         \
                    ungetc(ch2,_FILE_);         \
                    ungetc(ch,_FILE_);          \
                }                               \
            }                                   \
        } else ungetc(ch, _FILE_);              \
    } while(0)

#define SKIP_SHELL_COMMENT(_FILE_)              \
    do {                                        \
        int ch = fgetc(_FILE_);                 \
        if (ch == '#') {                        \
            while((ch = fgetc(_FILE_)) != EOF) {\
                if (ch == '\n' || ch == '\r')   \
                    break;                      \
            }                                   \
        }                                       \
        ungetc(ch, file);                       \
    } while(0)

extern JSBool js_InitAnalysisFunctions(JSContext *cx, JSObject* obj);
extern const JSErrorFormatString *getErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

#endif
