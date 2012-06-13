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
