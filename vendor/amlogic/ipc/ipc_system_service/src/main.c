/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "daemon.h"

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

#define CMD_SOCKET "/tmp/ipc.system.cmd.socket"
#define DEFAULT_SCRIPT_DIR "/etc/ipc/scripts"
#define THREAD_NOT_EXISTS -1

static const char *help_str =
    " ===============  Help  ===============\n"
    " Daemon name    :  " DAEMON_NAME "\n"
    " Daemon version :  " DAEMON_VERSION_STR "\n"
    " Build  date    :  " __DATE__ "\n"
    " Build  time    :  " __TIME__ "\n\n"
    "Options:                      description:\n\n"
    "       --no_chdir             Don't change the directory to '/'\n"
    "       --no_fork              Don't do fork\n"
    "       --no_close             Don't close standart IO files\n"
    "       --pid_file     [value] Set pid file name\n"
    "       --log_file     [value] Set log file name\n\n"
    "       --scripts      [value] Set scripts location\n\n"
    "  -v,  --version              Display daemon version\n"
    "  -h,  --help                 Display this help\n\n";

// indexes for long_opt function
enum {
  opt_version = 'v',
  opt_help = 'h',

  opt_no_chdir = 1,
  opt_no_fork,
  opt_no_close,
  opt_pid_file,
  opt_log_file,

  opt_scripts,

};

static const char *short_opts = "hv";

static const struct option long_opts[] = {
    {"version", no_argument, NULL, opt_version},
    {"help", no_argument, NULL, opt_help},

    // daemon options
    {"no_chdir", no_argument, NULL, opt_no_chdir},
    {"no_fork", no_argument, NULL, opt_no_fork},
    {"no_close", no_argument, NULL, opt_no_close},
    {"pid_file", required_argument, NULL, opt_pid_file},
    {"log_file", required_argument, NULL, opt_log_file},

    {"scripts", required_argument, NULL, opt_scripts},

    {NULL, no_argument, NULL, 0}};

static int gs_cmd_fd = -1;

#define LOGE(fmt, arg...)                                                      \
  fprintf(stderr, "[ipc-system-service] %s:%d " fmt "\n", __FUNCTION__,        \
          __LINE__, ##arg)

enum STATUS {
  STATUS_IDLE = 0,
  STATUS_BUSY = 1,
};

struct CmdInfo {
  TAILQ_ENTRY(CmdInfo) node;
  pthread_t tid;
  char *cmd;
  enum STATUS status;
  int retcode;
  char *output;
  int szoutput;
};

struct CmdInfoList {
  TAILQ_HEAD(CmdInfoListHead, CmdInfo) head;
};

static void *async_cmd_exec(void *args) {
  struct CmdInfo *cmdinfo = (struct CmdInfo *)args;
  if (cmdinfo->cmd == NULL || cmdinfo->cmd[0] == '\0') {
    cmdinfo->status = STATUS_IDLE;
    return NULL;
  }
  int fds[2];
  if (pipe(fds) == -1) {
    LOGE("fatal: create pipe failed!");
    cmdinfo->status = STATUS_IDLE;
    return NULL;
  }
  pid_t pid = fork();
  if (pid == -1) {
    LOGE("fatal: fork process failed!");
    cmdinfo->status = STATUS_IDLE;
    return NULL;
  }
  int status = 0;
  if (pid == 0) {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    close(fds[1]);
    execlp("/bin/sh", "sh", "-c", cmdinfo->cmd, NULL);
  } else {
    close(fds[1]);
    int nbytes = 0, tbytes = 0;
    char *out = NULL;
    char buf[1024];
    while (0 != (nbytes = read(fds[0], buf, sizeof(buf)))) {
      out = realloc(out, tbytes + nbytes + 1);
      memcpy(out + tbytes, buf, nbytes);
      tbytes += nbytes;
    }
    out[tbytes] = '\0';
    cmdinfo->output = out;
    cmdinfo->szoutput = tbytes;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      status = WEXITSTATUS(status);
    }
    cmdinfo->status = STATUS_IDLE;
    cmdinfo->retcode = status;
  }
  return NULL;
}

static bool handle_client_cmd(struct CmdInfoList *cmdinfo_list, int client_fd) {
  int r;
  struct ucred cr;
  socklen_t cr_size = sizeof(cr);
  /* Check socket options here */
  if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
    LOGE("Unable to receive socket options");
    return false;
  }

  unsigned char cmdlen;
  r = TEMP_FAILURE_RETRY(recv(client_fd, &cmdlen, sizeof(cmdlen), 0));
  if (r == 0) {
    LOGE("client closed");
    return true;
  }
  if (r != sizeof(cmdlen)) {
    LOGE("invalid msg size received: %d expected: %ld "
         "errno: %d",
         r, sizeof(cmdlen), errno);
    return false;
  }

  if (cmdlen == 0) {
    LOGE("empty cmd received");
    return false;
  }

  char *cmd = (char *)calloc(1, cmdlen + 1);
  r = TEMP_FAILURE_RETRY(recv(client_fd, cmd, cmdlen, 0));
  if (r == 0) {
    LOGE("client closed");
    return true;
  }
  if (r != cmdlen) {
    LOGE("mis-match cmd buffer size, received: %d "
         "expected: %d "
         "errno: %d",
         r, cmdlen, errno);
    return true;
  }

  if (strstr(cmd, "query-")) {
    long tid;
    char id[32];
    // search with tid
    strncpy(id, cmd + 13, strlen(cmd) - 1);
    sscanf(id, "%ld", &tid);
    struct CmdInfo *cmdinfo = NULL;
    TAILQ_FOREACH(cmdinfo, &cmdinfo_list->head, node) {
      if ((long)cmdinfo->tid == tid) {
        break;
      }
    }

    // send STATUS_NOT_EXISTS
    if (!cmdinfo) {
      int res = THREAD_NOT_EXISTS;
      char st[10];
      sprintf(st, "%d", res);
      int r = TEMP_FAILURE_RETRY(send(client_fd, &st, strlen(st), 0));
      if (r < 0) {
        LOGE("failed to send error flag\n");
      }
      return false;
    }

    // send cmdinfo->retcode
    if (strstr(cmd, "query-recode")) {
      char st[10];
      sprintf(st, "%d", cmdinfo->retcode);
      int r = TEMP_FAILURE_RETRY(send(client_fd, &st, strlen(st), 0));
      if (r < 0) {
        LOGE("failed to send command retcode\n");
      }
    }

    // send cmdinfo->status
    if (strstr(cmd, "query-status")) {
      char st[10];
      sprintf(st, "%d", cmdinfo->status);
      int r = TEMP_FAILURE_RETRY(send(client_fd, &st, strlen(st), 0));
      if (r < 0) {
        LOGE("failed to send thread status\n");
      }
    }

    // send cmdinfo->output
    if (strstr(cmd, "query-result")) {
      if (cmdinfo->output) {
        char *pout = cmdinfo->output;
        int len = strlen(cmdinfo->output);
        do {
          r = TEMP_FAILURE_RETRY(
              send(client_fd, pout, len > 1024 ? 1024 : len, 0));
          if (r <= 0) {
            break;
          }
          pout += r;
          len -= r;
        } while (len > 0);
      }
      TAILQ_REMOVE(&cmdinfo_list->head, cmdinfo, node);
      if (cmdinfo->output)
        free(cmdinfo->output);
      free(cmdinfo);
      cmdinfo = NULL;
    }
  } else {
    const char *script_dir = daemon_info.script_dir == NULL
                                 ? DEFAULT_SCRIPT_DIR
                                 : daemon_info.script_dir;

    int sz = snprintf(NULL, 0, "%s/%s", script_dir, cmd) + 1;
    char *script = (char *)malloc(sz);
    snprintf(script, sz, "%s/%s", script_dir, cmd);
    free(cmd);
    cmd = NULL;

    struct CmdInfo *cmdinfo = (struct CmdInfo *)malloc(sizeof(struct CmdInfo));
    cmdinfo->cmd = script;
    cmdinfo->output = NULL;
    cmdinfo->szoutput = 0;
    cmdinfo->status = STATUS_BUSY;
    pthread_create(&cmdinfo->tid, NULL, async_cmd_exec, cmdinfo);
    TAILQ_INSERT_HEAD(&cmdinfo_list->head, cmdinfo, node);

    char id[32];
    sprintf(id, "%ld", cmdinfo->tid);
    int r = TEMP_FAILURE_RETRY(send(client_fd, id, strlen(id), 0));
    if (r < 0) {
      LOGE("failed to send thread id\n");
    }
  }
  return false;
}

static void run_server(int sockfd) {
  int maxfd = sockfd;
  fd_set master_set, working_set;
  int rc;
  char recvbuf[1024];
  int fd;
  char *retstr;
  struct CmdInfoList cmdinfo_list;
  TAILQ_INIT(&cmdinfo_list.head);

  FD_ZERO(&master_set);
  FD_SET(sockfd, &master_set);
  while (1) {
    memcpy(&working_set, &master_set, sizeof(master_set));
    if ((rc = select(maxfd + 1, &working_set, NULL, NULL, NULL)) <= 0) {
      continue;
    }
    for (fd = 0; fd <= maxfd && rc > 0; ++fd) {
      if (!FD_ISSET(fd, &working_set)) {
        continue;
      }
      --rc;
      if (fd == sockfd) {
        struct sockaddr_un addr;
        socklen_t addr_size = sizeof(addr);
        int newfd = accept(sockfd, (struct sockaddr *)&addr, &addr_size);
        if (newfd < 0) {
          if (errno == EWOULDBLOCK) {
            LOGE("sock accept blocked\n");
          } else {
            LOGE("sock accept error\n");
          }
          continue;
        }
        FD_SET(newfd, &master_set);
        if (newfd > maxfd) {
          maxfd = newfd;
        }
      } else {
        if (handle_client_cmd(&cmdinfo_list, fd)) {
          close(fd);
          FD_CLR(fd, &master_set);
          if (fd >= maxfd) {
            maxfd = fd - 1;
          }
        }
      }
    }
    usleep(100);
  }
  return;
}

static int create_socket(const char *name, int type, mode_t perm, uid_t uid,
                         gid_t gid) {
  struct sockaddr_un addr;
  int fd, ret;

  fd = socket(PF_UNIX, type, 0);
  if (fd < 0) {
    fprintf(stderr, "Failed to open socket '%s': %s\n", name, strerror(errno));
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", name);

  ret = unlink(addr.sun_path);
  if (ret != 0 && errno != ENOENT) {
    fprintf(stderr, "Failed to unlink old socket '%s': %s\n", name,
            strerror(errno));
    goto out_close;
  }

  ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret) {
    fprintf(stderr, "Failed to bind socket '%s': %s\n", name, strerror(errno));
    goto out_unlink;
  }

  chown(addr.sun_path, uid, gid);
  chmod(addr.sun_path, perm);

  fprintf(stdout, "Created socket '%s' with mode '%o', user '%d', group '%d'\n",
          addr.sun_path, perm, uid, gid);

  return fd;

out_unlink:
  unlink(addr.sun_path);
out_close:
  close(fd);
  return -1;
}

static void daemon_exit_handler(int sig) {
  unlink(daemon_info.pid_file);
  exit(EXIT_SUCCESS);
}

static void init_signals() {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = daemon_exit_handler;
  if (sigaction(SIGTERM, &sa, NULL) != 0)
    daemon_error_exit("Can't set daemon_exit_handler: %m\n");

  // signal(SIGCHLD, SIG_IGN); // ignore child
  signal(SIGTSTP, SIG_IGN); // ignore tty signals
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
}

static void parse_opt(int argc, char *argv[]) {
  int opt;

  while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
    switch (opt) {
    case opt_help:
      puts(help_str);
      exit_if_not_daemonized(EXIT_SUCCESS);
      break;

    case opt_version:
      puts(DAEMON_NAME "  version  " DAEMON_VERSION_STR "\n");
      exit_if_not_daemonized(EXIT_SUCCESS);
      break;

      // daemon options
    case opt_no_chdir:
      daemon_info.no_chdir = 1;
      break;

    case opt_no_fork:
      daemon_info.no_fork = 1;
      break;

    case opt_no_close:
      daemon_info.no_close_stdio = 1;
      break;

    case opt_pid_file:
      daemon_info.pid_file = optarg;
      break;

    case opt_log_file:
      daemon_info.log_file = optarg;
      break;

    case opt_scripts:
      daemon_info.script_dir = optarg;
      break;

    default:
      puts("for more detail see help\n\n");
      exit_if_not_daemonized(EXIT_FAILURE);
      break;
    }
  }
}

static int start_service() {
  int fd;
  pthread_t tid;

  fd = create_socket(CMD_SOCKET, SOCK_STREAM, 0666, 0, 0);
  if (fd < 0) {
    daemon_error_exit("failed to create cmd service\n");
    return -1;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  listen(fd, 16);
  return fd;
}

static void init(void *data) {
  init_signals();
  gs_cmd_fd = start_service();
}

int main(int argc, char *argv[]) {
  struct pollfd ufds;
  int nr, i, timeout = -1;

  parse_opt(argc, argv);
  daemonize2(init, NULL);

  run_server(gs_cmd_fd);
}
