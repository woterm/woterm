#ifndef LOCALSOCKET_H
#define LOCALSOCKET_H

typedef int (*ipc_callback)(int err, char *argv[], int argc);

void* ipc_connect(const char* name, ipc_callback cb);
int ipc_call(void* hdl, char *argv[], int argc);
void ipc_close(void* hdl);

#endif