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
#include "ipc_property_internal.h"

#include <ipc_property_priv.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <sys/time.h>

#define IPC_PROP_VALUE_MAX 256

static const char *help_str =
   " ===============  Help  ===============\n"
   " Daemon name:  " DAEMON_NAME          "\n"
   " Daemon  ver:  " DAEMON_VERSION_STR   "\n"
   " Build  date:  " __DATE__ "\n"
   " Build  time:  " __TIME__ "\n\n"
   "Options:                      description:\n\n"
   "       --no_chdir             Don't change the directory to '/'\n"
   "       --no_fork              Don't do fork\n"
   "       --no_close             Don't close standart IO files\n"
   "       --pid_file     [value] Set pid file name\n"
   "       --log_file     [value] Set log file name\n\n"
   "       --cfg_file     [value] Set config file name\n\n"
   "  -v,  --version              Display daemon version\n"
   "  -h,  --help                 Display this help\n\n";


// indexes for long_opt function
namespace LongOpts
{
  enum
  {
    version = 'v',
    help    = 'h',

    no_chdir = 1,
    no_fork,
    no_close,
    pid_file,
    log_file,

    cfg_file,

  };
}

static const char *short_opts = "hv";

static const struct option long_opts[] =
   {
      { "version",      no_argument,       nullptr, LongOpts::version       },
      { "help",         no_argument,       nullptr, LongOpts::help          },

      //daemon options
      { "no_chdir",     no_argument,       nullptr, LongOpts::no_chdir      },
      { "no_fork",      no_argument,       nullptr, LongOpts::no_fork       },
      { "no_close",     no_argument,       nullptr, LongOpts::no_close      },
      { "pid_file",     required_argument, nullptr, LongOpts::pid_file      },
      { "log_file",     required_argument, nullptr, LongOpts::log_file      },

      { "cfg_file",     required_argument, nullptr, LongOpts::cfg_file      },


      { nullptr,           no_argument,       nullptr,  0                      }
   };

static int property_cmd_fd = -1;
static int property_event_fd = -1;

static std::vector<int> event_client_fds;

static pthread_cond_t property_event_cond;
static pthread_mutex_t property_event_mutex;
static struct ipc_prop_msg *property_event;

static bool property_changed = false;

static void
on_property_changed(const char* key, const char* value) {
  if(key == nullptr || value == nullptr) return;
  struct ipc_prop_msg *msg;
  msg = (struct ipc_prop_msg *) malloc (sizeof(struct ipc_prop_msg));
  msg->cmd = IPC_PROP_EVENT_CHANGED;
  msg->keysz = strlen(key) + 1;
  msg->valsz = strlen(value) + 1;
  msg = (struct ipc_prop_msg *) realloc (msg,
                                         sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
  strcpy(&msg->buf[0], key);
  strcpy(&msg->buf[msg->keysz], value);
  pthread_mutex_lock(&property_event_mutex);
  property_event = msg;
  property_changed = true;
  pthread_cond_broadcast(&property_event_cond);
  pthread_mutex_unlock(&property_event_mutex);
}

static void
handle_property_cmd_fd() {
  struct ipc_prop_msg *msg = nullptr;
  char *prop_key = nullptr;
  char *prop_value = nullptr;
  int s;
  int r;
  int res;
  struct ucred cr;
  struct sockaddr_un addr;
  socklen_t addr_size = sizeof(addr);
  socklen_t cr_size = sizeof(cr);
  if ((s = accept(property_cmd_fd, (struct sockaddr *) &addr, &addr_size)) < 0) {
    return;
  }
  /* Check socket options here */
  if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
    close(s);
    fprintf(stderr, "Unable to receive socket options\n");
    return;
  }

  msg = (struct ipc_prop_msg *)malloc(sizeof(struct ipc_prop_msg));
  r = TEMP_FAILURE_RETRY(recv(s, msg, sizeof(struct ipc_prop_msg), 0));
  if(r != sizeof(ipc_prop_msg)) {
    fprintf(stderr, "ipc_prop: mis-match msg size received: %d expected: %ld errno: %d\n",
           r, sizeof(ipc_prop_msg), errno);
    close(s);
    return;
  }

  if (msg->keysz + msg->valsz > 0) {
    msg = (struct ipc_prop_msg *) realloc(msg,
                                          sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
    r = TEMP_FAILURE_RETRY(recv(s, &msg->buf[0], msg->keysz + msg->valsz, 0));
    if (r != msg->keysz + msg->valsz) {
      fprintf(stderr, "ipc_prop: ms-match msg buf size, received: %d expected: %ld errono: %d\n",
             r, msg->keysz + msg->valsz, errno);
      close(s);
      return;
    }
  }

  switch(msg->cmd) {
    case IPC_PROP_MSG_GETPROP:
    case IPC_PROP_MSG_LISTPROP:
    case IPC_PROP_MSG_GETPROP_ARRAY_SIZE:
      if(msg->cmd == IPC_PROP_MSG_GETPROP) {
        prop_key = &msg->buf[0];
        prop_value = (char *) calloc(sizeof(char), IPC_PROP_VALUE_MAX);
        msg->valsz = __property_get(prop_key, prop_value, IPC_PROP_VALUE_MAX) + 1;
      } else if(msg->cmd == IPC_PROP_MSG_LISTPROP) {
        if (msg->keysz > 0) {
          prop_key = &msg->buf[0];
        } else {
          prop_key = nullptr;
        }
        prop_value = (char *) calloc(sizeof(char), IPC_PROP_VALUE_MAX << 8);
        msg->valsz = __property_list(prop_key, prop_value, IPC_PROP_VALUE_MAX << 8) + 1;
        msg->keysz = 0;
      } else if(msg->cmd == IPC_PROP_MSG_GETPROP_ARRAY_SIZE) {
        prop_key = &msg->buf[0];
        msg->valsz = sizeof(int);
        int array_sz = __property_get_array_size(prop_key);
        prop_value = (char *) calloc(sizeof(int), 1);
        memcpy (prop_value, &array_sz, sizeof(int));
      }
      msg = (struct ipc_prop_msg *)realloc(msg,
                                           sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz);
      memcpy(&msg->buf[msg->keysz], prop_value, msg->valsz);
      free(prop_value);
      r = TEMP_FAILURE_RETRY(send(s, msg,
                                  sizeof(struct ipc_prop_msg), 0));
      if (r != sizeof(struct ipc_prop_msg)) {
        fprintf(stderr, "ipc_prop: mis-match msg size sent: %d expected: %ld errno: %d\n",
               r, sizeof(ipc_prop_msg), errno);
      } else {
        // split send
        size_t bufsize = msg->valsz + msg->keysz;
        char *buf = &msg->buf[0];

        do {
          r = TEMP_FAILURE_RETRY(send(s, buf, bufsize > 1024 ? 1024 : bufsize, 0));
          if (r <= 0) {
            break;
          }
          buf += r;
          bufsize -= r;
        } while (bufsize > 0);
      }
      close(s);
      break;
    case IPC_PROP_MSG_SETPROP:
      prop_key = &msg->buf[0];
      prop_value = &msg->buf[msg->keysz];
      if(__property_set(prop_key, prop_value)) {
        on_property_changed(prop_key, prop_value);
      }
      close(s);
      break;
    case IPC_PROP_MSG_REMOVEPROP:
      prop_key = &msg->buf[0];
      if(__property_remove(prop_key)) {
        on_property_changed(prop_key, "");
      }
      close(s);
      break;
    default:
      close(s);
      break;
  }
  if(msg) free(msg);
}

static void
handle_property_event_fd() {
  int s;
  struct ucred cr;
  struct sockaddr_un addr;
  socklen_t addr_size = sizeof(addr);
  socklen_t cr_size = sizeof(cr);
  if ((s = accept(property_event_fd, (struct sockaddr *) &addr, &addr_size)) < 0) {
    return;
  }
  /* Check socket options here */
  if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
    close(s);
    fprintf(stderr, "Unable to receive socket options\n");
    return;
  }
  pthread_mutex_lock(&property_event_mutex);
  event_client_fds.push_back(s);
  pthread_mutex_unlock(&property_event_mutex);
}

static bool
broadcast_event(struct ipc_prop_msg *msg) {
  if (msg->cmd == IPC_PROP_EVENT_PROC_EXIT) {
    return true;
  } else if (msg->cmd == IPC_PROP_EVENT_CHANGED) {
    // event changed, broadcast to all client
    for (std::vector<int>::iterator it = event_client_fds.begin();
         it < event_client_fds.end(); it++) {
      int fd = *it;
      if (fd < 0) {
        fprintf(stderr, "invalid fd\n");
        continue;
      }
      size_t sent_bytes = send(
          fd, msg, sizeof(struct ipc_prop_msg) + msg->keysz + msg->valsz, 0);
      if (sent_bytes < 0) {
        fprintf(stderr, "broad send event failed\n");
        close(fd);
        *it = -1;
      }
    }
  }

  // clear invalid client
  event_client_fds.erase(std::remove_if(event_client_fds.begin(),
                                        event_client_fds.end(),
                                        [](int i) { return i < 0; }),
                         event_client_fds.end());
  return false;
}

static void *
property_event_broadcast(void* arg) {
  bool bexit = false;

  for (;;) {
    pthread_mutex_lock(&property_event_mutex);
    while (property_event == nullptr) {
      pthread_cond_wait(&property_event_cond, &property_event_mutex);
    }

    if (property_event) {
      bexit = broadcast_event(property_event);
      free(property_event);
      property_event = nullptr;
    }

    pthread_mutex_unlock(&property_event_mutex);

    if (bexit)
      break;
  }
  return nullptr;
}

static void *
property_timed_save(void* arg) {
  for (;;) {
    pthread_mutex_lock(&property_event_mutex);
    while (property_changed == false) {
      pthread_cond_wait(&property_event_cond, &property_event_mutex);
    }

    if (property_changed) {
      __save_properties(daemon_info.cfg_file);
      property_changed = false;
    }

    pthread_mutex_unlock(&property_event_mutex);

    usleep(200*1000);
  }
}

static int
create_socket(const char* name, int type, mode_t perm, uid_t uid, gid_t gid) {
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
    fprintf(stderr, "Failed to unlink old socket '%s': %s\n", name, strerror(errno));
    goto out_close;
  }

  ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret) {
    fprintf(stderr, "Failed to bind socket '%s': %s\n", name, strerror(errno));
    goto out_unlink;
  }

  chown(addr.sun_path, uid, gid);
  chmod(addr.sun_path, perm);

  fprintf(stdout, "Created socket '%s' with mode '%o', user '%d', group '%d'\n", addr.sun_path, perm, uid, gid);

  return fd;

  out_unlink:
  unlink(addr.sun_path);
  out_close:
  close(fd);
  return -1;
}

static void
daemon_exit_handler(int sig) {
  unlink(daemon_info.pid_file);
  exit(EXIT_SUCCESS);
}

static void
init_signals() {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = daemon_exit_handler;
  if ( sigaction(SIGTERM, &sa, nullptr) != 0 )
    daemon_error_exit("Can't set daemon_exit_handler: %m\n");

  signal(SIGCHLD, SIG_IGN); // ignore child
  signal(SIGTSTP, SIG_IGN); // ignore tty signals
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGHUP,  SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
}

static void
processing_cmd(int argc, char *argv[]) {
  int opt;

  while( (opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1 ) {
    switch( opt )
    {
      case LongOpts::help:
        puts(help_str);
        exit_if_not_daemonized(EXIT_SUCCESS);
        break;

      case LongOpts::version:
        puts(DAEMON_NAME "  version  " DAEMON_VERSION_STR "\n");
        exit_if_not_daemonized(EXIT_SUCCESS);
        break;

        //daemon options
      case LongOpts::no_chdir:
        daemon_info.no_chdir = 1;
        break;

      case LongOpts::no_fork:
        daemon_info.no_fork = 1;
        break;

      case LongOpts::no_close:
        daemon_info.no_close_stdio = 1;
        break;

      case LongOpts::pid_file:
        daemon_info.pid_file = optarg;
        break;

      case LongOpts::log_file:
        daemon_info.log_file = optarg;
        break;

      case LongOpts::cfg_file:
        daemon_info.cfg_file = optarg;
        break;

      default:
        puts("for more detail see help\n\n");
        exit_if_not_daemonized(EXIT_FAILURE);
        break;
    }
  }
}

static void
start_service(const char* fn) {
  int fd;
  pthread_t tid;

  if(fn == nullptr) {
    daemon_error_exit("config file not set\n");
    return;
  }

  if(!__load_properties(fn)) {
    daemon_error_exit("failed to open config file: %s\n", fn);
    return;
  }

  pthread_mutex_init(&property_event_mutex, nullptr);
  pthread_cond_init(&property_event_cond, nullptr);
  pthread_create(&tid, nullptr, property_event_broadcast, nullptr);
  pthread_create(&tid, nullptr, property_timed_save, nullptr);

  fd = create_socket(IPC_PROPERTY_SERVICE_CMD_SOCKET, SOCK_STREAM, 0666, 0, 0);
  if(fd < 0) {
    daemon_error_exit("failed to create cmd service\n");
    return;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  listen(fd, 16);
  property_cmd_fd = fd;

  fd = create_socket(IPC_PROPERTY_SERVICE_EVENT_SOCKET, SOCK_STREAM, 0666, 0, 0);
  if(fd < 0) {
    if (property_cmd_fd) close(property_cmd_fd);
    daemon_error_exit("failed to create event service\n");
    return;
  }
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  listen(fd, 64);
  property_event_fd = fd;
}

static void
init(void *data) {
  init_signals();
  start_service(daemon_info.cfg_file);
}


int main(int argc, char* argv[]) {
  int fd_count = 0;
  struct pollfd ufds[4];

  int nr, i, timeout = -1;

  processing_cmd(argc, argv);
  daemonize2(init, nullptr);

  if (property_cmd_fd > 0) {
    ufds[fd_count].fd = property_cmd_fd;
    ufds[fd_count].events = POLLIN;
    ufds[fd_count].revents = 0;

    fd_count ++;
  }

  if (property_event_fd > 0) {
    ufds[fd_count].fd = property_event_fd;
    ufds[fd_count].events = POLLIN;
    ufds[fd_count].revents = 0;

    fd_count ++;
  }

  for(;;) {
    nr = poll(ufds, fd_count, timeout);
    if (nr <= 0)
      continue;
    for (i = 0; i < fd_count; i++) {
      if (ufds[i].revents == POLLIN) {
        if (ufds[i].fd == property_cmd_fd)
          handle_property_cmd_fd();
        if (ufds[i].fd == property_event_fd)
          handle_property_event_fd();
      }
    }
  }

}

