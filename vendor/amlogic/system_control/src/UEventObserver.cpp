#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <syslog.h>

#include "UEventObserver.h"

#define LOG_TAG "systemcontrol"

UEventObserver::UEventObserver() : mFd(-1) {
    syslog(LOG_INFO, "UEventObserver::UEventObserver");
    mFd = ueventInit();
    mMatchStr.num = 0;
    mMatchStr.strList.buf = NULL;
    mMatchStr.strList.next = NULL;
    pthread_mutex_init(&mMutex, 0);
}

UEventObserver::~UEventObserver() {
    syslog(LOG_INFO, "UEventObserver::~UEventObserver");
    pthread_mutex_destroy(&mMutex);
    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
}

int UEventObserver::ueventInit() {
    struct sockaddr_nl addr;
    int sz = 64 * 1024;
    int s;

    syslog(LOG_INFO, "UEventObserver::ueventInit");
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (s < 0)
        return 0;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

    return s;
}

int UEventObserver::ueventGetFd() {
    return mFd;
}

int UEventObserver::ueventNextEvent(char *buffer, int length) {
    syslog(LOG_INFO, "UEventObserver::ueventNextEvent");
    while (1) {
        struct pollfd fds;
        int nr;

        fds.fd = mFd;
        fds.events = POLLIN;
        fds.revents = 0;
        nr = poll(&fds, 1, -1);

        if (nr > 0 && (fds.revents & POLLIN)) {
            int count = recv(mFd, buffer, length, 0);
            if (count > 0)
                return count;
        }
    }

    return 0;
}

bool UEventObserver::isMatch(const char *buffer, size_t length,
        UEventData *ueventData, const char *matchStr) {
    bool matched = false;
    const char *field = buffer;
    const char *end = buffer + length + 1;

    do {
        if (!strcmp(field, matchStr)) {
            syslog(LOG_INFO, "UEventObserver isMatch: matched uevent message with pattern: %s\n", matchStr);
            strcpy(ueventData->matchName, matchStr);
            matched = true;
        } else if (strstr(field, "STATE=HDMI=")) {
            strcpy(ueventData->switchState, field + strlen("STATE=HDMI="));
        } else if (strstr(field, "NAME=")) {
            strcpy(ueventData->switchName, field + strlen("NAME="));
        } else if (strstr(field, "FRAME_RATE_HINT=")) {
            strcpy(ueventData->switchName, field + strlen("FRAME_RATE_HINT="));
        } else if (strstr(field, "FRAME_RATE_END_HINT=")) {
            strcpy(ueventData->switchName, "end_hint");
        }
        field += strlen(field) + 1;
    } while (field != end);

    if (matched) {
        ueventData->len = length;
        memcpy(ueventData->buf, buffer, length);
    }

    return matched;
}

bool UEventObserver::isMatch(const char *buffer, size_t length, UEventData *ueventData) {
    bool matched = false;

    syslog(LOG_INFO, "UEventObserver isMatch: size=%d\n", mMatches.size());
    pthread_mutex_lock(&mMutex);
    for (size_t i = 0; i < mMatches.size(); i++) {
        const char *matchStr = mMatches.at(i).c_str();
        matched = isMatch(buffer, length, ueventData, matchStr);
        if (matched)
            break;
    }
    pthread_mutex_unlock(&mMutex);

    return matched;
}

void UEventObserver::waitForNextEvent(UEventData *ueventData) {
    char buffer[1024] = {0};

    syslog(LOG_INFO, "UEventObserver::waitForNextEvent");
    for (;;) {
        int length = ueventNextEvent(buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            syslog(LOG_INFO, "UEventObserver received uevent message length: %d\n", length);
            return;
        }

        buffer[length] = '\0';
        ueventPrint(buffer, length);
        if (isMatch(buffer, length, ueventData))
            return;
    }
}

void UEventObserver::addMatch(const char *matchStr) {
    syslog(LOG_INFO, "UEventObserver::addMatch");
    pthread_mutex_lock(&mMutex);
    mMatches.push_back(matchStr);
    pthread_mutex_unlock(&mMutex);
}

void UEventObserver::removeMatch(const char *matchStr) {
    syslog(LOG_INFO, "UEventObserver::removeMatch");
    pthread_mutex_lock(&mMutex);
    std::vector<std::string>::iterator iter;
    for (iter = mMatches.begin(); iter != mMatches.end(); iter++) {
        if (!strcmp((*iter).c_str(), matchStr)) {
            iter = mMatches.erase(iter);
            break;
        }
    }
    pthread_mutex_unlock(&mMutex);
}

void UEventObserver::ueventPrint(char *buffer, int length) {
    char buf[1024] = {0};

    syslog(LOG_INFO, "UEventObserver::ueventPrint");
    memcpy(buf, buffer, length);
    for (int i = 0; i < length; i++) {
        if (buf[i] == 0x0)
            buf[i] = ' ' ;
    }
    syslog(LOG_INFO, "UEventObserver received uevent message: %s\n", buf);
}
