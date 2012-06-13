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

use("make_public");
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
