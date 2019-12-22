var WoLocalCommand;

(function(){
    var __shell__singleton;
    WoShellCommand = function () {
        if (__shell__singleton !== undefined) {
            return __shell__singleton;
        }
        __shell__singleton = this;

        var glog = new WoLog();

        this.cd = function(path){
            var result = shell.execCommand("cd "+ path);
            if(result.err)  {
                glog.error("failed to cd path", path);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.login = function(hosts) {
            var result = shell.login(hosts)
            if(result.err)  {
                glog.error("failed to login all server");
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.su = function(user, pass) {
            var result = shell.su(user, pass);
            if(result.err) {
                glog.error("failed to switch to root user");
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.sudo = function(pass) {
            var result = shell.sudo(pass);
            if(result.err) {
                glog.error("failed to switch to root user");
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.mkdir = function(path) {
            var result = shell.execCommand("mkdir -p " + path)
            if(result.err) {
                glog.error("failed to create directory.", path);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.rm = function(path) {
            var result = shell.execCommand("rm -rf " + path)
            if(result.err) {
                glog.error("failed to rm file", path);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.upload = function(path) {
            var result = shell.upload(path);
            if(result.err) {
                glog.error("failed to upload file:", path);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.tar = function(file) {
            var result = shell.execCommand("tar -xf " + file);
            if(result.err) {
                glog.error("failed to extract file:", file);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.ln = function(src, dst, force) {
            var cmd = "ln -sf " + src + " " + dst;
            if(force) {
                cmd = "rm -rf " + dst + " && " + cmd;
            }
            var result = shell.execCommand(cmd);
            if(result.err) {
                glog.error("failed to ln file from:", src, dst);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.sleep = function(ms) {
            shell.sleep(ms);
        };

        this.ls = function(path, hidden) {
            var cmd = hidden ? "ls -a" + path : "ls "+ path;
            var result = shell.execCommand(cmd);
            if(result.err) {
                glog.error("failed to ls directory.", path);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        this.exec = function(cmd) {        
            var result = shell.execCommand(cmd);
            if(result.err) {
                glog.error("failed to ls directory.", cmd);
                return false;
            }
            glog.info(result.cmd, "\r\n", result.output);
            return true;
        };

        return this;
    }
})();