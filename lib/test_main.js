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

/*
 *  
 * @author bocelli.hu
 *
 */
(function() {
    function process_file(file, result) {

        console.log(file, "..");
        result.files++;
        try {
            var node = parse(file);
            for(var i=0, n=rules.length;i<n;i++) {
                var rule = rules[i];
                var success = 0;
                var fail = 0;
                for(var testcase in rule) {
                    if(!testcase.match("^test_.+$")) continue;
                    var ret = false;
                    try {
                        ret = rule[testcase](node);
                    } catch(e) {
                        console.print_stack_trace(e.message);
                    }
                }
                if(ret) {
                    console.log("    ok", rule.name);
                    result.success++;
                } else {
                    console.error("    ng", rule.name);
                    result.failed++;
                }
            }
        } catch(e) {
            e.fileName=file+"";
            throw e;
        }
    }
    function load_rules(rules, path) {
        
        var rulesFloder = new File(path);

        if(!rulesFloder.exists || !rulesFloder.isDirectory ) return false;
        var files = rulesFloder.list();
        
        for(var i=0,n=files.length;i<n;i++) {

            var file = files[i];
            if(file.isFile) {
                if(!file.name.match(".+\.js$")) continue;
                var obj = {};
                obj.__exec_file = exec_file;
                obj.__exec_file(file);
                if(!obj.name) {
                    obj.name=file.name.substring(0, file.name.length-3);
                }
                rules.push(obj);
            } else {
                load_rules(rules, file);
            }
        }
        return true;
    }
    function process_directory(files, result) {

        for(var i=0,n=files.length;i<n;i++) {
            var file = files[i];
            if(file.isFile) {
                if(!files[i].name.match(".+\.js$")) continue;
                process_file(files[i], result);
            } else if(file.isDirectory) {
                process_directory(file.list(), result);
            }
        }
    }

    if(!this.test_file) {
        console.error("no test file");
        return false;
    }

    var file = new File(test_file);
    if(!file.exists) {
        console.log(file, "not exists.");
        return false;
    }

    var rules = new Array;
    var rules_path = this.rules_path || __base_dir + "/../rules";
    load_rules(rules, rules_path);
    var result = {success: 0, failed: 0, files: 0};
    if(file.isFile) {
        process_file(file, result);
    } else {
        process_directory(file.list(), result); 
    }
    console.info("files:", result.files, ", tests:", result.success + result.failed,
                ", success:", result.success, ", failed:", result.failed);
})();

