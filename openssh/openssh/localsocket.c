#include "localsocket.h"


#ifdef WIN32

#include <Windows.h>


typedef struct {
    ipc_callback    cb; 
    HANDLE          hPipe;
} IpcData;

static const int intsize = sizeof(int);

static BOOL ReadPipe2(HANDLE hPipe, void *pbuf, int length)
{
    /*
    �������ֲ��Լ������ϣ���û��Ū���ף�Ϊʲô�ᵼ�³��������
    */
    OVERLAPPED overlap;
    DWORD read, avail, left;
    memset(&overlap, 0, sizeof(overlap));
    if(!ReadFile(hPipe, pbuf, length, &read, &overlap)){
        if (GetLastError() != ERROR_IO_PENDING) {
            return FALSE;
        }
        if(!GetOverlappedResult(hPipe, &overlap, &read, TRUE)) {
            return FALSE;
        }
        return TRUE;
    }
    return TRUE;
}

static BOOL ReadPipe(HANDLE hPipe, char *pbuf, int length)
{
    OVERLAPPED overlap = { 0 };
    DWORD cnt = 0, avail = 0;
    while (avail < length) {
        if (!PeekNamedPipe(hPipe, 0, 0, 0, &avail, 0)) {
            return FALSE;
        }
        if(avail >= length) {
            break;
        }
        Sleep(100);
    }
    return ReadFile(hPipe, pbuf, length, &cnt, &overlap);
}

static DWORD WINAPI read_threadfunc(void *arg)
{
    IpcData *pdata = (IpcData*)arg;
    char *pbuf = 0;
    int buf_max = 0;
    char *argv[100] = {0};
    int argc = 0;

    while(1) {
        int i = 0, length = 0;
        char *ptr = 0;
        if(!ReadPipe(pdata->hPipe, &length, intsize)) {
            pdata->cb(-1, 0, 0);
            return 0;
        }
        if(buf_max < length){
            if(pbuf) {
                free(pbuf);
            }
            pbuf = malloc(length);
            buf_max = length;
        }
        if(!ReadPipe(pdata->hPipe, pbuf, length)) {
            pdata->cb(-2, 0, 0);
            return 0;
        }
        ptr = pbuf;
        memcpy(&argc, ptr, intsize);
        ptr += intsize;
        if(argc >= 100) {
            pdata->cb(-3, 0, 0);
            return 0;
        }
        for(i = 0; i < argc; i++) {
            int length;
            memcpy(&length, ptr, intsize);
            ptr += intsize;
            argv[i] = ptr;
            ptr += length;
        }
        if(pdata->cb(0, argv, argc) < 0) {
            return 0;
        }
    }
    return 0;
}

void* ipc_connect(const char* name, ipc_callback cb)
{
    /* connect to named pipe */
    IpcData *pdata = 0;
    HANDLE hdl = INVALID_HANDLE_VALUE;
    char pipe_name[256] = { 0 };
    strcpy(pipe_name, name);
    hdl = CreateFileA(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(hdl == INVALID_HANDLE_VALUE) {
        return 0;
    }
    pdata = (IpcData*)malloc(sizeof(IpcData));
    pdata->cb = cb;
    pdata->hPipe = hdl;
    DWORD tid;
    HANDLE hThread = CreateThread(NULL, 0, read_threadfunc, (void*)pdata, 0, &tid);
    CloseHandle(hThread);
    Sleep(0);
    return pdata;
}

#define MAX_SIZE  (1)
int ipc_call(void* hdl, char *argv[], int argc)
{
    OVERLAPPED overlap;
    char buf[MAX_SIZE] = { 0 };
    char *pbuf = 0, *ptr = 0;
    IpcData *pdata = (IpcData*)hdl;
    int i = 0, cnt = 0;
    int length = intsize + 1;

    for(i = 0; i < argc; i++) {
        length += intsize;
        length += strlen(argv[i]);
        length += 1;
    }
    if ((length + intsize) >= MAX_SIZE) {
        pbuf = (char*)malloc(length + intsize);
    }else{
        pbuf = buf;
    }
    ptr = pbuf;
    memcpy(ptr, &length, intsize);
    ptr += intsize;
    memcpy(ptr, &argc, intsize);
    ptr += intsize;
    for(i = 0; i < argc; i++) {
        int sz = strlen(argv[i]) + 1;
        memcpy(ptr, &sz, intsize);
        ptr += intsize;
        memcpy(ptr, argv[i], sz-1);
        ptr += sz-1;
        ptr[0] = '\0';
        ptr += 1;
    }
    ptr[0] = '\0';

    memset(&overlap, 0, sizeof(overlap));

    if(!WriteFile(pdata->hPipe, pbuf, length + intsize, &cnt, &overlap)){
        if (GetLastError() == ERROR_IO_PENDING) {
            GetOverlappedResult(pdata->hPipe, &overlap, &cnt, TRUE);
        }
    }

    if((length + intsize) > MAX_SIZE){
        free(pbuf);
    }
    return 0;
}

void ipc_close(void* hdl)
{
    IpcData *pdata = (IpcData*)hdl;
    CloseHandle(pdata->hPipe);
    free(pdata);
}

#else

#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#if defined(__APPLE__)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

typedef struct {
    int             fd;
    ipc_callback    cb;
    pthread_t       pthread;
} IpcData;

static const int intsize = sizeof(int);

static void* read_cb(void *arg)
{
    IpcData *pdata = (IpcData*)arg;
    char *pbuf = 0;
    int buf_max = 0;
    char *argv[100] = {0};
    int argc = 0;

    while(1) {
        int i = 0, length = 0;
        char *ptr = 0;
        if(recv(pdata->fd, &length, intsize, 0) <= 0) {
            pdata->cb(-1, 0, 0);
            return 0;
        }

        if(buf_max < length){
            if(pbuf) {
                free(pbuf);
            }
            pbuf = malloc(length);
            buf_max = length;
        }

        if(recv(pdata->fd, pbuf, length, MSG_WAITALL) <= 0) {
            pdata->cb(-2, 0, 0);
            return 0;
        }
        ptr = pbuf;
        memcpy(&argc, ptr, intsize);
        ptr += intsize;
        if(argc >= 100) {
            pdata->cb(-2, 0, 0);
            return 0;
        }
        for(i = 0; i < argc; i++) {
            int length;
            memcpy(&length, ptr, intsize);
            ptr += intsize;
            argv[i] = ptr;
            ptr += length;
        }
        if(pdata->cb(0, argv, argc) < 0) {
            return 0;
        }
    }
    return 0;
}

void* ipc_connect(const char* name, ipc_callback cb)
{
    char unixname[256] = { 0 };
    struct sockaddr_un un;
    IpcData * data = 0;

    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    strcpy(unixname, name);

    un.sun_family = PF_UNIX;
    strcpy(un.sun_path, unixname);
    if(connect(fd, (struct sockaddr *)&un, sizeof(un)) < 0){
        printf("\nconnect socket failed");
        return 0;
    }
    data = (IpcData*)malloc(sizeof(IpcData));
    data->fd = fd;
    data->cb = cb;
    if(pthread_create(&data->pthread, 0, read_cb, data) < 0) {
        printf("\nfailed to create read thread");
        free(data);
        return 0;
    }
    sleep(0);
    return data;
}

int ipc_call(void* hdl, char *argv[], int argc)
{
    char buf[512] = {0};
    char *pbuf = 0, *ptr = 0;
    IpcData *pdata = (IpcData*)hdl;
    int i = 0;
    int length = intsize + 1;

    for(i = 0; i < argc; i++) {
        length += intsize;
        length += strlen(argv[i]);
        length += 1;
    }
    if((length + intsize) >= 512) {
        pbuf = (char*)malloc(length + intsize);
    }else{
        pbuf = buf;
    }
    ptr = pbuf;
    memcpy(ptr, &length, intsize);
    ptr += intsize;
    memcpy(ptr, &argc, intsize);
    ptr += intsize;
    for(i = 0; i < argc; i++) {
        int sz = strlen(argv[i]) + 1;
        memcpy(ptr, &sz, intsize);
        ptr += intsize;
        memcpy(ptr, argv[i], sz-1);
        ptr += sz-1;
        ptr[0] = '\0';
        ptr += 1;
    }
    ptr[0] = '\0';
    send(pdata->fd, pbuf, length + intsize, 0);
    if(length >= 512) {
        free(pbuf);
    }
    return 0;
}

void ipc_close(void* hdl)
{
    IpcData *pdata = (IpcData*)hdl;
    close(pdata->fd);
    free(pdata);
}

#endif
