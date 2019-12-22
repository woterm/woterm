var WoLocalCommand;

(function(){
    var __local__singleton;
    WoLocalCommand = function () {
        if (__local__singleton !== undefined) {
            return __local__singleton;
        }
        __local__singleton = this;

        var glog = new WoLog();
    
        this.scriptDirectory = function() {
            var path = local.scriptDirectory();
            glog.info("scriptDirectory", "\r\n", path);
            return path;
        };

        this.echo = function(msg) {
            local.echo(msg);
            glog.info("echo ", "\r\n", msg);
        }

        this.pwd = function() {
            return local.pwd();
            var path = local.pwd();
            glog.info("pwd", "\r\n", path);
            return path;
        };

        this.cleanPath = function(path) {
            var path= local.cleanPath(path);
            glog.info("cleanpath " + path, "\r\n", path);
            return path;
        };

        this.toNativePath = function(path) {
            var path = local.toNativePath(path);
            glog.info("toNativePath " + path, "\r\n", path);
            return path;
        };

        this.isAbsolutePath = function(path) {
            var ok = local.isAbsolutePath();
            glog.info("isAbsolutePath " + path, "\r\n", ok);
            return ok;
        };

        this.nativeSeperator = function() {
            return local.nativeSeperator();
        };

        this.cd = function(path) {
            var ok = local.cd(path);
            glog.info("cd " + path, "\r\n", ok);
            return ok;
        };
        
        this.mkpath=function(path) {
            var ok = local.mkpath(path);
            glog.info("mkpath " + path, "\r\n", ok);
            return ok;
        };

        this.exist=function(path) {
            var ok = local.exist(path);
            glog.info("exist " + path, "\r\n", ok);
            return ok;
        };

        this.rmpath=function(path) {
            var ok = local.rmpath(path);
            glog.info("rmpath " + path, "\r\n", ok);
            return ok;
        };
        this.ls = function(path) {
            var fis = local.ls(path);
            glog.info("ls " + path, "\r\n", fis.join(" "));
            return fis;
        };
        this.isFile = function(path) {
            var ok = local.isFile(path);
            glog.info("isFile " + path, "\r\n", ok);
            return ok;
        };
        
        this.fileContent=function(path) {
            var result = local.fileContent(path);
            if(result.err)  {
                glog.error(result.cmd, "\r\n", result.output);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return result.output;
        };
        
        this.runScriptFile=function(file) {
            var result = local.runScriptFile(file);
            if(result.err)  {
                glog.error(result.cmd, "\r\n", result.output);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };
        return this;
    }
})();