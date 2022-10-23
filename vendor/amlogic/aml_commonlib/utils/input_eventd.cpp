#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "events_process.h"

int main(int argc, char **argv) {
    EventsProcess* ep = new EventsProcess();
    ep->Init();
    while (1) ep->WaitKey();
    return 0;
}

