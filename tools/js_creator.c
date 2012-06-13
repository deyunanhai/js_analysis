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
