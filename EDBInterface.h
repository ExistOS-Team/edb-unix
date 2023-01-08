#pragma once
#include <cstdio>
#include <cstdint>
#include <string>

#define BIN_BLOB_SIZE 32768

typedef struct flashImg {
    FILE *f;
    char *filename;
    uint32_t toPage;
    bool bootImg;
} flashImg;

class EDBInterface {
private:
    int hCMDf;
    int hDATf;
    const int wrBufSize = 512;
    char *wrBuf = nullptr;
    char *sendBuf = nullptr;
    char* createCmdPath();
    char* createDatPath();
    char *devicePath = nullptr;

public:
    char* c_mnt_path = (char*) "/tmp/edbMount";
    bool waitStr(char *str);
    bool wrStr(const char *str);
    bool wrDat(char *dat, size_t len);
    bool rdDat(char *dat, size_t len, size_t *rbcnt);
    bool eraseBlock(unsigned int block);
    int flash(flashImg item);
    void reboot();
    void vm_suspend();
    void vm_resume();
    void vm_reset();
    void mscmode();
    bool ping();
    void close();
    int open();
};
