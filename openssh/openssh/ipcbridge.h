#ifndef IPCBRIDGE_H
#define IPCBRIDGE_H


int IpcBridgeStart();
void IpcBridgeRequestPassword(char *prompt, int echo);
void IpcBridgeUpdatePassword(char *password);
void IpcBridgeSuccessToLogin();
int IpcBridgeGetTerminalSize(int *width, int *height);

#endif