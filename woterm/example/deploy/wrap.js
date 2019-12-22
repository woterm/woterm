var MyShellCommand;
var MyLocalCommand;
var MyUtils;

(function(){
    var __shell_singleton;
    MyShellCommand = function () {
        if (__shell_singleton !== undefined) {
            return __shell_singleton;
        }        
        __shell_singleton = this;
        var glog = new WoLog();
        var utils = new MyUtils();
        WoShellCommand.call(this);
        this.ls = function(path, hosts, fn) {
            var cmd = "ls " + path;
            var result = shell.execCommand(cmd);
            if(result.err) {
                glog.error("failed to ls directory.", path);
                return false;
            }

            glog.info(result.cmd, "\r\n", result.output);

            var items = [];
            for(var i = 0; i < hosts.length; i++){
                var out = result.hostOutput(hosts[i]);
                var lines = out.split("\r\n");
                for(var j = 0; j < lines.length; j++) {
                    var files = lines[j].split(" ");
                    items = items.concat(files);
                }
            }   
            items = utils.arrayDistinct(items);
            items = items.sort();
            fn(items);            
            return true;
        };
        
        return this;
    };
})();

(function(){
    var __local_singleton;
    MyLocalCommand =  function() {
        if (__local_singleton !== undefined) {
            return __local_singleton;
        }
        __local_singleton = this;
        var glog = new WoLog();
        WoLocalCommand.call(this);        
        return this;
    };
})();

(function(){
    var __utils_singleton;
    MyUtils = function() {
        if (__utils_singleton !== undefined) {
            return __utils_singleton;
        }
        __utils_singleton = this;
        var glog = new WoLog();
        this.fetchFileName = function(file) {
            var path = glocal.cleanPath(file);
            path = glocal.toNativePath(path);
            var idx = path.lastIndexOf(glocal.nativeSeperator());
            if(idx > 0) {
                path = path.substring(idx);
            }
            return path;
        };

        this.arrayDistinct = function (arr) {
            var result = [];
            for(var i = 0; i < arr.length; i++) {
                var bFind = false;
                if(result.indexOf(arr[i]) >= 0) {
                    continue;
                }
                result.push(arr[i]);
            }
            return result;
        };
        return this;
    };
})();