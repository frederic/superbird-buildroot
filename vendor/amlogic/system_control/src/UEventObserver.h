#ifndef UEVENT_OBSERVER_H
#define UEVENT_OBSERVER_H

#include <pthread.h>

#include <vector>
#include <string>

struct UEventData {
    int len;
    char buf[1024];
    char matchName[256];
    char switchName[64];
    char switchState[64];
};

struct MatchNode {
    char *buf;
    MatchNode *next;
};

struct MatchItem {
    int num;
    MatchNode strList;
};

class UEventObserver {
public:
    UEventObserver();
    ~UEventObserver();

    void addMatch(const char *matchStr);
    void removeMatch(const char *matchStr);
    void waitForNextEvent(UEventData *ueventData);
    int ueventGetFd();

private:
    int ueventInit();
    bool isMatch(const char *buffer, size_t length, UEventData *ueventData, const char *matchStr);
    bool isMatch(const char *buffer, size_t length, UEventData *ueventData);
    int ueventNextEvent(char *buffer, int length);
    void ueventPrint(char *buffer, int length);

    int mFd;
    MatchItem mMatchStr;
    pthread_mutex_t mMutex;
    std::vector<std::string> mMatches;
};

#endif
