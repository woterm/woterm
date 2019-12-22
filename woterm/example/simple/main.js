function main(project, version, tarfile) {
    var local = new MyLocalCommand();
    var shell = new MyShellCommand();
    var log = new WoLog();
    var path_script = local.scriptDirectory();

    var fis = local.ls("./");

    if(!local.mkpath("d:/abc123/dd22/22")) {
        return 0;
    }
    if(!local.rmpath("d:/abc123/dd22/22")) {
        return 0;
    }
    var content = local.fileContent("./hello.conf");
    var result = local.runScriptFile("./script.bat");
    local.echo("dddddddddddddddddddddddddddddddddd");
    return 11;
}