#pragma once
#include <errno.h>
#include <libudev.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#define INFO(fmt, ...) printf(fmt, __VA_ARGS__)

#define EOS_VID 0xCAFE
#define EOS_PID 0x4003
// Device endpoint(s)
#define EP_IN 0x83
#define EP_OUT 0x03

#define MY_CONFIG 1
#define MY_INTF 2

#define EDB_MODE_BIN true
#define EDB_MODE_TEXT false

#define BIN_BLOB_SIZE 32768

// Availble on Windows but not on Unix {
#define DWORD unsigned int
#define WCHAR char16_t
#define LPWSTR wchar_t *

#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2

#define INVALID_HANDLE_VALUE ((HANDLE) ~(ULONG_PTR)0)
#define INVALID_FILE_SIZE (~0u)
#define INVALID_SET_FILE_POINTER (~0u)
#define INVALID_FILE_ATTRIBUTES (~0u)

// }

typedef struct flashImg {
    FILE *f;
    char *filename;
    uint32_t toPage;
    bool bootImg;
} flashImg;

class EDBInterface {
private:
    libusb_device_handle *usbdev = NULL; /* the device handle */
    bool USBHighSpeed = false;
    // CComHelper com;
    char *sendBuf = nullptr;
    int hCMDf;
    int hDATf;
    const int wrBufSize = 512;
    char *wrBuf = nullptr;

public:
    void reset(bool mode);
    bool waitStr(char *str);
    void wrStr(const char *str);
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
    int open(bool mode);
};
