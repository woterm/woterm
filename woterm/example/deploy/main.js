function main(project, version, tarfile) {
    var local = new MyLocalCommand();
    var shell = new MyShellCommand();
    var log = new WoLog();
    var path_script = local.scriptDirectory();
    var keep = 30;
    var path_project="/data/code/" + project;
    var path_work=path_project + "/work";
    var path_version=path_project+"/" + version;
    var path_tarfile = local.cleanPath(path_script + "/./" + tarfile); 
    
    log.warn("warning...............");
    var hosts = ["mini","devel"];
    if(!shell.login(hosts)) {
        return 0;
    }
    if(!shell.su("root", "123")) {
        return 0;
    }
    if(!shell.mkdir(path_project)) {
        return 0;
    }
    if(!shell.mkdir(path_version)) {
        return 0;
    }
    if(!shell.cd(path_version)) {
        return 0;
    }
    if(!shell.rm(tarfile)) {
        return 0;
    }
    if(!shell.upload(path_tarfile)){
        return 0;
    }
    if(!shell.exec("tar -xf "+tarfile+" && ls ./ && chmod a+x ./run-app.sh")) {
        return 0;
    }
    if(!shell.ln(path_version, path_work, true)) {
        return 0;
    }
    if(!shell.cd(path_work)) {
        return 0;
    }
    if(!shell.exec("pwd && ls ./ && ./run-app.sh install dev")) {
        return 0;
    }
    if(!shell.exec("./run-app.sh reload")) {
        return 0;
    }
    if(!shell.cd(path_project)) {
        return 0;
    }
    
    if(!shell.ls(path_project, hosts, function(items){
        for(var i = 0; i < items.length - keep; i++) {            
            shell.rm(items[i]);
        }
    })){
        return 0;
    }
    
    return 11;
}