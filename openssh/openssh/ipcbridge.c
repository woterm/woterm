/* $ IpcBridge: for message with master process. $ */

#include "ipcbridge.h"
#include "localsocket.h"

#include <stdio.h>
#include <stdlib.h>

static void *ipc_hdl = 0;

int win_width = 80;
int win_height = 24;

#ifdef WIN32

#include <Windows.h>

extern void queue_terminal_exit_event();
extern void queue_terminal_window_change_event();

#else

#include <dlfcn.h>
#include <signal.h>

void queue_terminal_exit_event() {
    raise(SIGINT);
}

void queue_terminal_window_change_event() {
    raise(SIGWINCH);
}

#endif //


static int MyIpcCallBack(int err, char *funArgv[], int argc) {
    if (err < 0) {
        queue_terminal_exit_event();
        return 0;
    }
    if (strcmp(funArgv[0], "setwinsize") == 0) {
        int width = atoi(funArgv[1]);
        int height = atoi(funArgv[2]);
        if (width != win_width || win_height != height) {
            win_width = width;
            win_height = height;
            queue_terminal_window_change_event();
        }
    }
    return 0;
}

void safeExit(void) {
    ipc_close(ipc_hdl);
}

/*因为程序启动时，会从3开始，一直清理当前进程下的所有句柄，所以该函数调用，需要摆放到closefrom函数调用之后*/
int IpcBridgeStart() {
    char *name = getenv("TERM_MSG_IPC_NAME");
    if (name == 0) {
        return 1;
    }
    if (strcmp(name, "bad") == 0) {
        return 2;
    }

    ipc_hdl = ipc_connect(name, MyIpcCallBack);

    putenv("TERM_MSG_IPC_NAME=bad");
    return 1;
}

void IpcBridgeRequestPassword(char *prompt, int echo) {
    if (ipc_hdl == 0 || prompt == 0 || strlen(prompt) == 0) {
        return;
    }
    char *pbool = echo != 0 ? "true" : "false";
    char *buf[] = {"requestpassword", prompt, pbool};
    ipc_call(ipc_hdl, buf, 3);
}

void IpcBridgeUpdatePassword(char *password) {
    if (ipc_hdl == 0 || password == 0 || strlen(password) == 0) {
        return;
    }
    char *buf[] = {"updatepassword", password};
    ipc_call(ipc_hdl, buf, 2);
}

int IpcBridgeGetTerminalSize(int *width, int *height) {
    if (ipc_hdl == 0) {
        return 0;
    }
    *width = win_width;
    *height = win_height;
    char *buf[] = {"getwinsize"};
    ipc_call(ipc_hdl, buf, 1);
    return 1;
}

void IpcBridgeSuccessToLogin()
{
    if (ipc_hdl == 0) {
        return;
    }
    char *buf[] = {"successToLogin"};
    ipc_call(ipc_hdl, buf, 1);
}

