#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "SystemControl.h"

#define LOG_TAG "systemcontrol"
#define THREAD_SLEEP_MS 100
#define SYSTEM_CONTROL_SOCKET   "/run/systemcontrol/socket"
#define SOCKET_BACKLOG  5

static bool quit = false;
static SystemControl *pSystemControl = NULL;

static void signal_handler(int sig) {
    quit = true;
}


static void strsplit(char cmd[128], char* value1, char* value2, char* value3)
{
    int start=0;
    int len = 0;

    for (int i = 0; i < 126; i++) {
        len++;
        char* temp = (char *)calloc(128, sizeof(char));
        char* temp1 = (char *)calloc(128, sizeof(char));
        char* temp2 = (char *)calloc(128, sizeof(char));

        memcpy(temp, &cmd[i], 2);
        memcpy(temp1, &cmd[i], 4);
        memcpy(temp2, &cmd[i], 4);
        char target[] = "hz";
        char autoTarget[] = "auto";
        char cvbsTarget[] = "cvbs";
        if (strcmp(temp,target) == 0) {
            len++;
            break;
        } else if (strcmp(temp1, autoTarget) == 0) {
            len += 3;
            break;
        } else if (strcmp(temp2, cvbsTarget) == 0) {
            len += 3;
	    break;
        }
        free(temp);
        free(temp1);
        free(temp2);
    }

    memcpy(value1, &cmd[start], len);
    start = len;
    len = 0;
    for (int i = start; i < 126; i++) {
        len++;
        char* temp = (char *)calloc(128, sizeof(char));
        char* temp1 = (char *)calloc(128, sizeof(char));
        memcpy(temp, &cmd[i], 3);
        memcpy(temp1, &cmd[i], 4);
        char target[] = "bit";
        char autoTarget[] = "auto";
        if (strcmp(temp,target) == 0) {
            len += 2;
            break;
        } else if (strcmp(temp1, autoTarget) == 0) {
            len += 3;
            break;
        }
        free(temp);
        free(temp1);
    }

    memcpy(value2, &cmd[start], len);
    memcpy(value3, &cmd[start+len], 128 - start - len);
}

static int server_socket(){
    long flags;
    struct sockaddr_un addr;
    int sk;

    sk = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sk < 0)
        return sk;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SYSTEM_CONTROL_SOCKET, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    if (bind(sk, (struct sockaddr *)&addr,sizeof(addr)) == -1) {
        syslog(LOG_CRIT, "system control bind failed!");
        close(sk);
        return -1;
    }

    flags = fcntl(sk, F_GETFL);
    if (flags == -1 || fcntl(sk, F_SETFL, flags | O_NONBLOCK) == -1) {
        syslog(LOG_CRIT, "system control set NONBLOCK failed!");
        close(sk);
        return -1;
    }

    if (listen(sk, SOCKET_BACKLOG) == -1) {
        syslog(LOG_CRIT, "system control listen failed!");
        close(sk);
        return -1;
    }

    return sk;
}

int main(int argc, char **argv) {
    const char *path = NULL;
    int ret = 0;
    int sock;
    struct pollfd pfd;
    int npfd;
    int cnt;
    struct sockaddr_un from_addr;
    socklen_t fromlen;
    int ns;
    char cmd[128] = {0,};
    int len;

    if (argc >= 2)
        path = argv[1];
    openlog(LOG_TAG, LOG_CONS | LOG_PID, LOG_DAEMON);
    if (daemon(0, 0) < 0) {
        syslog(LOG_CRIT, "system control daemon failed!");
        ret = -1;
        goto ret;
    }

    syslog(LOG_INFO, "system control started");
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGABRT, signal_handler);

    sock = server_socket();
    if (sock == -1) {
        syslog(LOG_CRIT, "system control socket failed!");
        ret = -1;
        goto ret;
    }

    pSystemControl = new SystemControl(path);
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = sock;
    pfd.events = POLLIN;

    while (!quit) {
        while ((cnt = poll(&pfd, 1, 500)) > 0) {
            if (pfd.revents) {
                fromlen = sizeof(from_addr);
                ns = accept(sock, (struct sockaddr *)&from_addr, &fromlen);
                if (ns < 0) {
                    continue;
                }
                len = read(ns, &cmd, sizeof(cmd));
                syslog(LOG_CRIT, "system control service read cmd len(%d) (%s)", len, cmd);
                if (!strcmp(cmd, "zoomout")) {
                    pSystemControl->setZoomOut();
                } else if (!strcmp(cmd, "zoomin")) {
                    pSystemControl->setZoomIn();
                } else {
                    char* displaymode = (char *)calloc(128, sizeof(char));
                    char* colormode = (char *)calloc(128, sizeof(char));
                    char* hdcpmode = (char *)calloc(128, sizeof(char));
                    strsplit(cmd, displaymode, colormode, hdcpmode);
                    syslog(LOG_CRIT, "system control service displaymode(%s) colormode(%s) hdcpmode(%s)", displaymode, colormode, hdcpmode);
                    pSystemControl->setBootEnv(UBOOTENV_HDCPMODE, hdcpmode);
                    pSystemControl->setTvColorFormat(colormode);
                    pSystemControl->setTvOutputMode(displaymode);
                    pSystemControl->setTvHdcpMode(hdcpmode);

                    free(colormode);
                    free(displaymode);
                    free(hdcpmode);
                }
            }
        }
    }

    closelog();

ret:
    if (pSystemControl)
        delete pSystemControl;
    close(sock);

    return ret;
}
