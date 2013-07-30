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

#include "stdlib.h"
#include "stdio.h"
#include <jsscan.h>
#include <jsatom.h>

typedef struct ops_t {
    const char* name;
    const char* token;
} ops_t;

const ops_t OPS[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) \
    {name, token},
#include "jsopcode.tbl"
#undef OPDEF
};

const int NUM_OPS = sizeof(OPS) / sizeof(OPS[0]);

int main(int argc, char **argv) {
    
    int i;
    ops_t op;
   
#define op_name op.name
#define op_token (op.token ? op.token : "")
#define print_fmt_default "    {name:'%s', token:'%s'}"
#define print_fmt ((i+1) != NUM_OPS ? print_fmt_default ",\n" : print_fmt_default "\n")
    //OPS
    printf("var OPS = [\n");
    for(i=0;i<NUM_OPS;i++) {
        op = OPS[i];
        printf(print_fmt, op_name, op_token);
    }
    printf("];\n");
#undef print_fmt_default
#define print_fmt_default "    \"%s\" : %d"
    //NodeOp
    printf("var NodeOp = {\n");
    for(i=0;i<NUM_OPS;i++) {
        op = OPS[i];
        printf(print_fmt, op_name, i);
    }
    printf("};\n");
#undef op_token
#undef op_name

    return 0; 
}
