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

/*
 * this is common utils can help you print and access treenode or scan token easy.
 * @author bocelli.hu
 *
 */
(function() {

    function _try_loop_tree(node, func, arg) {

        switch(node.arity) {
        case NodeArity.PN_FUNC: 
            func(node.body, arg);
            break;
        case NodeArity.PN_LIST: 
            for(var i=0,n=node.list.length; i<n;i++) {
                func(node.list[i], arg);
            }
            break;
        case NodeArity.PN_TERNARY: 
            func(node.kid1, arg);
            func(node.kid2, arg);
            func(node.kid3, arg);
            break;
        case NodeArity.PN_BINARY: 
            func(node.left, arg);
            func(node.right, arg);
            break;
        case NodeArity.PN_UNARY: 
            func(node.kid, arg);
            break;
        case NodeArity.PN_NAME: 
            if(node.expr) func(node.expr, arg);
            break;
        case NodeArity.PN_NULLARY: 
            break;
        default: 
        }
    }
    function print_tree(node, indent) {
        if(!node) return;
        if(!indent) indent = 0;
        for(var i=0;i<indent;i++) print(" ");
        print(TYPE_NAMES[node.type], "from L"+node.begin_lineno, ",C"+node.begin_index);
        print(", to L"+node.end_lineno, ",C"+node.end_index);
        print(", arity:", ARITY_NAMES[node.arity]);
        print(", op:", OPS[node.op].name);
        print(", name:", (node.name?node.name:""));
        if(node.arity == NodeArity.PN_FUNC) {
            var args = node.args;
            print(", args:", node.args.join())
        } else { 
            print(", value:", (node.value==undefined?"":node.value));
        }
        println();
        _try_loop_tree(node, print_tree, indent+2);
    }
    function visit_tree(node, callback) {
        if(!node || !callback) return false;
        
        var nt = NodeType;
        switch(node.type) {
        case nt.FUNCTION:
            callback.got_function && callback.got_function(node);
            break;
        case nt.LP:
            if(node.op == NodeOp.call) {
                callback.got_call && callback.got_call(node);
            }
            break;
        case nt.VAR:
            callback.got_var && callback.got_var(node);
            break;
        case nt.IF:
            callback.got_if && callback.got_if(node);
            break;
        case nt.WHILE:
            callback.got_while && callback.got_while(node);
            break;
        case nt.DO:
            callback.got_do && callback.got_do(node);
            break;
        case nt.FOR:
            callback.got_for && callback.got_for(node);
            break;
        case nt.SWITCH:
            callback.got_switch && callback.got_switch(node);
            break;
        case nt.TRY:
            callback.got_try && callback.got_try(node);
            break;
        case nt.HOOK:
            callback.got_hook && callback.got_hook(node);
            break;
        }
        
        _try_loop_tree(node, visit_tree, callback);
    }
    
    function iterate_tree(node, callback) {
        if(!node || !callback) return false;

        callback(node);
        _try_loop_tree(node, iterate_tree, callback);
    }

    // for token stream
    function print_token(token) {
        if(!token || !token.type) return false;
        print(TYPE_NAMES[token.type], "from L"+token.begin_lineno, ",C"+token.begin_index);
        print(", to L"+token.end_lineno, ",C"+token.end_index);
        print(", op:", OPS[token.op].name);
        print(", value:", (token.value==undefined?"":token.value));
        println();
    }
    function print_tokens(token_arr) {
        if(!token_arr) return false;
        for(var i=0,n=token_arr.length;i<n;i++) {
            print_token(token_arr[i]);
        }
    }
    function iterate_token(token_arr, callback) {
        if(!token_arr) return false;
        for(var i=0,n=token_arr.length;i<n;i++) {
            callback(token_arr[i]);
        }
    }
    
    make_public("print_tree", print_tree);
    make_public("print_tokens", print_tokens);
    make_public("print_token", print_token);
    make_public("visit_tree", visit_tree);
    make_public("iterate_tree", iterate_tree);
    make_public("iterate_token", iterate_token);
   
    // other utils
    make_public("__filename", function() {
        return (__files.length>0 ? __files[__files.length-1] : undefined);
    });
})();

