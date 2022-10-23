#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define SYSTEM_CONTROL_SOCKET   "/run/systemcontrol/socket"

static int write_all(int fd, char *buf, size_t count) {
    ssize_t ret;
    int c = 0;

    while (count > 0) {
        ret = write(fd, buf, count);
        if (ret < 0) {
            return -1;
        }
        count -= ret;
        buf += ret;
        c += ret;
    }

    return c;
}

int main(int argc, char **argv) {
    int fd;
    int i;
    int ret;
    char cmd[128] = {0,};
    int s;
    struct sockaddr_un server_addr;

    fd = open("/dev/console", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd != -1) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    for (i = 1; i < argc; i++) {
        fprintf(stderr, "argv[%d]: (%s)\n", i, argv[i]);
        strncat(cmd, argv[i], sizeof(cmd) - 1 - strlen(cmd));
    }

    fprintf(stderr, "cmd:(%s)\n", cmd);

    if ((s = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "create socket failed!\n");
        return -1;
    }

    server_addr.sun_family = PF_UNIX;
    strncpy(server_addr.sun_path, SYSTEM_CONTROL_SOCKET, sizeof(server_addr.sun_path));
    server_addr.sun_path[sizeof(server_addr.sun_path) - 1] = '\0';

    if (connect(s, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) < 0) {
        fprintf(stderr, "connect socket failed, error:%s!\n", strerror(errno));
        close(s);
        return -1;
    }

    ret = write_all(s, cmd, sizeof(cmd));
    fprintf(stderr, "write socket ret:(%d) sizeof(cmd): (%d)\n", ret, sizeof(cmd));

    close(s);

    return 0;
}
