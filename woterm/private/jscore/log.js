var WoLog;

(function(){
    var __log__singleton;
    WoLog = function() {
        if (__log__singleton !== undefined) {
            return __log__singleton;
        }
        __log__singleton = this;

        this.error = function() {
            var len=arguments.length;
            var out = [];
            for(var i=0;i<len;i++){
                out.push(""+arguments[i]);
            }
            log.error(out.join(", "));
        }

        this.info = function() {
            var len=arguments.length;
            var out = [];
            for(var i=0;i<len;i++){
                out.push(""+arguments[i]);
            }
            log.info(out.join(", "));
        }
        
        this.warn = function() {
            var len=arguments.length;
            var out = [];
            for(var i=0;i<len;i++){
                out.push(""+arguments[i]);
            }
            log.warn(out.join(", "));
        }
        return this;
    };
})();