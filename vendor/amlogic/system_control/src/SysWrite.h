#ifndef SYS_WRITE_H
#define SYS_WRITE_H

#include <stdint.h>

#define UNIFYKEY_ATTACH     "/sys/class/unifykeys/attach"
#define UNIFYKEY_NAME        "/sys/class/unifykeys/name"
#define UNIFYKEY_WRITE      "/sys/class/unifykeys/write"
#define UNIFYKEY_READ        "/sys/class/unifykeys/read"
#define UNIFYKEY_EXIST       "/sys/class/unifykeys/exist"
#define UNIFYKEY_LOCK       "/sys/class/unifykeys/lock"

class SysWrite
{
public:
    SysWrite();
    ~SysWrite();

//	bool getProperty(const char *key, char *value);
//	bool getPropertyString(const char *key, char *value, const char *def);
//	int32_t getPropertyInt(const char *key, int32_t def);
//	int64_t getPropertyLong(const char *key, int64_t def);
//	bool getPropertyBoolean(const char *key, bool def);
//	void setProperty(const char *key, const char *value);

    bool readSysfs(const char *path, char *value);
    bool readSysfsOriginal(const char *path, char *value);
    bool writeSysfs(const char *path, const char *value);
    bool writeSysfs(const char *path, const char *value, const int size);
    void executeCMD(const char *cmd, char *result);
private:
    void writeSys(const char *path, const char *val);
    int writeSys(const char *path, const char *val, const int size);
    void readSys(const char *path, char *buf, int count, bool needOriginalData);
    int readSys(const char *path, char *buf, int count);
};

#endif
