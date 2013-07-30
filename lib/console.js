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
 * this is a set of utils can help you print color strings on screen.
 * @author bocelli.hu
 *
 */
(function() {
    var _colors = {
        red     : "\033[0;31m",
        cyan    : "\033[1;36m",
        green   : "\033[4;32m",
        blue    : "\033[9;34m",
        black   : "\033[0;30m",
        yellow  : "\033[0;33m",
        magenta : "\033[0;35m",
        gray    : "\033[0;37m",
        none    : "\033[0m"   
    };

    function _print(color, args_array) {
        var args;
        if(!color || color == _colors.none) args = args_array;
        else {
            args = [];
            for(var i=0,n=args_array.length;i<n;i++) {
                args.push(color+args_array[i]+_colors.none);
            }
        }
        
        println.apply(this, args);
    }
  
    make_public("console", {
        log: function() {
            _print(undefined, arguments);
        },
        info: function() {
            _print(_colors.blue, arguments);
        },
        error: function() {
            _print(_colors.red, arguments);
        },
        warn: function() {
            _print(_colors.yellow, arguments);
        },
        debug: function() {
            _print(undefined, arguments);
        },
        print_stack_trace: function(err) {
            
            console.error(err.fileName + ":" + err.lineNumber + ":", err.message);
            if(err.stack) {
                var stack = err.stack.replace(/(?:\n@:0)?\s+$/m, '').replace(/^\(/gm, '{anonymous}(').split('\n');
                for(var i=0, n=stack.length;i<n;i++) {
                    console.error("   ", stack[i]);
                }
            }
        }
    });
})();
