#ifndef SYSTEM_CONTROL_H
#define SYSTEM_CONTROL_H

#include "DisplayMode.h"

class SystemControl {
public:
    SystemControl(const char *path);
    virtual ~SystemControl();
    int getBootEnv(const char *key, char *value);
    int setBootEnv(const char *key, const char *value);
    void setTvOutputMode(const char* mode);
    void setTvColorFormat(const char* mode);
    void setTvHdcpMode(const char* mode);
    void setZoomOut();
    void setZoomIn();

private:
    DisplayMode *pDisplayMode;
};

#endif
