#include "EDBInterface.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

using namespace std;

long long getTime() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

unsigned char blockChksum(char *block, unsigned int blockSize) {
    unsigned char sum = 0x5A;
    for (unsigned int i = 0; i < blockSize; i++) {
        sum += block[i];
    }
    return sum;
}

char *EDBInterface::createCmdPath() {
    size_t len = strlen(mntPath);
    char *s = (char *)malloc(len + 10);
    strcpy(s, mntPath);
    if (len != 0 || mntPath[len - 1] != '/') {
        strcat(s, "/");
    }
    strcat(s, "cmd_port");
    return s;
}

char *EDBInterface::createDatPath() {
    size_t len = strlen(mntPath);
    char *s = (char *)malloc(len + 10);
    strcpy(s, mntPath);
    if (len != 0 || mntPath[len - 1] != '/') {
        strcat(s, "/");
    }
    strcat(s, "dat_port");
    return s;
}

bool EDBInterface::waitStr(char *str) {
    int retry = 2;
    int ret;
    while (retry > 0) {
        cout << "Waiting Sync...\r";
        memset(wrBuf, 0, wrBufSize);
        lseek(hCMDf, 0, SEEK_SET);
        ret = read(hCMDf, wrBuf, wrBufSize); // ReadFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
        if (ret) {
            if (strcmp(str, wrBuf) == 0) {
                return true;
            } else {
                cout << "Sync Failed... Retry: " << retry << endl;
                retry--;
            }
        } else {
            cout << "Sync Failed... Retry: " << retry << endl;
            retry--;
        }
    }

    return false;
}

bool EDBInterface::wrStr(const char *str) {
    if (!str) {
        return false;
    }
    memset(wrBuf, 0, wrBufSize);
    strcpy(wrBuf, str);
    lseek(hCMDf, 0, SEEK_SET);
    return (bool)write(hCMDf, wrBuf, wrBufSize);
}

bool EDBInterface::wrDat(char *dat, size_t len) {
    int ret = 0;
    lseek(hDATf, 0, SEEK_SET);
    return (bool)write(hDATf, dat, len);
}

bool EDBInterface::rdDat(char *dat, size_t len, size_t *rbcnt) {
    memset(wrBuf, 0, wrBufSize);
    lseek(hCMDf, 0, SEEK_SET);
    int ret = read(hCMDf, wrBuf, wrBufSize);
    memcpy(dat, wrBuf, len);
    *rbcnt = len;

    return (bool)ret;
}

bool EDBInterface::eraseBlock(unsigned int block) {
    char cmdbuf[64];
    memset(cmdbuf, 0, sizeof(cmdbuf));
    sprintf(cmdbuf, "ERASEB:%d\n", block);
    this->wrStr(cmdbuf);
    if (this->waitStr((char *)"EROK\n") == false) {
        printf("Erase block timed out: %d\n", block);
        return false;
    }
    return true;
}

int EDBInterface::flash(flashImg item) {
    char cmdbuf[64];
    size_t cnt;
    size_t rbcnt;
    if (this->ping() == false) {
        cout << "Device not responding." << endl;
        return false;
    }
    this->wrStr("RESETDBUF\n");
    if (!this->waitStr((char *)"READY\n")) {
        cout << "Device not responding." << endl;
        return false;
    }
    cout << "Writing: " << item.filename << "..." << endl;
    fseek(item.f, 0, SEEK_END);
    size_t fsize = ftell(item.f);
    uint8_t chksum;
    uint32_t rcshkdum;
    long long st;
    rewind(item.f);

    uint32_t page_cnt = item.toPage;
    uint32_t block_cnt = page_cnt / 64;
    uint32_t last_block = 0;
    do {
        block_cnt = page_cnt / 64;

        st = getTime();

        memset(sendBuf, 0xFF, BIN_BLOB_SIZE);
        memset(cmdbuf, 0, 64);

        cnt = fread(sendBuf, 1, BIN_BLOB_SIZE, item.f);
        chksum = blockChksum(sendBuf, BIN_BLOB_SIZE);

        this->wrDat(sendBuf, BIN_BLOB_SIZE);

        this->wrStr("BUFCHK\n");

        this->rdDat(cmdbuf, 10, &rbcnt);

        sscanf(cmdbuf, "CHKSUM:%02x\n", &rcshkdum);

        if (rcshkdum != chksum) {
            printf("chksum error: expecting %02x, got %02x instead\n", chksum, rcshkdum);
            return false;
        }

        if (block_cnt != last_block) {
            if (eraseBlock(block_cnt) == false) {
                printf("Erase Block Time Out: %d\n", block_cnt);
                return false;
            }
            // Block Erase;
        }

        if (item.bootImg) {
            sprintf(cmdbuf, "PROGP:%d,1\n", page_cnt);
        } else {
            sprintf(cmdbuf, "PROGP:%d,0\n", page_cnt);
        }
        this->wrStr(cmdbuf);
        if (this->waitStr((char *)"PGOK\n") == false) {
            printf("Program page timed out: %d\n", page_cnt);
            return false;
        }

        if (page_cnt % 200 == 0) {
            long long speed = BIN_BLOB_SIZE / (getTime() - st);
            cout << "Upload: " << ftell(item.f) << "/" << fsize;
            cout << " Page: " << page_cnt << " Block: " << block_cnt;
            printf("  chksum: %02x==%02x, %lld KB/s", chksum, rcshkdum, speed);
            cout << "  " << (fsize - ftell(item.f)) / speed / 1000 << "s remaining        \r";
            fflush(stdout);
        }

        page_cnt += BIN_BLOB_SIZE / 2048;
        last_block = block_cnt;
    } while (cnt > 0);

    if (item.bootImg) {
        cout << "\nSetting NCB..." << endl;
        memset(cmdbuf, 0, sizeof(cmdbuf));
        sprintf(cmdbuf, "MKNCB: %d, %ld\n", item.toPage / 64, fsize / 2048);
        this->wrStr(cmdbuf);
        if (this->waitStr((char *)"MKOK\n") == false) {
            printf("Setting NCB page timed out: %d\n", item.toPage / 64);
            return false;
        }
    }

    return true;
}

void EDBInterface::reboot() {
    this->wrStr("REBOOT\n");
}

void EDBInterface::vm_suspend() {
    this->wrStr("VMSUSPEND\n");
}

void EDBInterface::vm_resume() {
    this->wrStr("VMRESUME\n");
}

void EDBInterface::vm_reset() {
    this->wrStr("VMRESET\n");
}

void EDBInterface::mscmode() {
    this->wrStr("MSCDATA\n");
}

bool EDBInterface::ping() {
    int retry = 5;
    int ret;
    while (retry) {
        memset(wrBuf, 0, wrBufSize);
        strcpy(wrBuf, "PING\n");
        lseek(hCMDf, 0, SEEK_SET);
        ret = write(hCMDf, wrBuf, wrBufSize);
        if (ret > 0) {
            lseek(hCMDf, 0, SEEK_SET);
            ret = read(hCMDf, wrBuf, wrBufSize);
            if (ret > 0) {
                if (strcmp(wrBuf, "PONG\n") == 0) {
                    return true;
                }
                sleep(2);
                retry--;
            }
        } else {
            sleep(2);
            retry--;
        }
    }

    return false;
}

void EDBInterface::close() {
    ::close(hCMDf);
    ::close(hDATf);

    int returnVal = umount(mntPath);
    if (returnVal) {
        printf("umount() failed with code %d\n", returnVal);
    }

    returnVal = rmdir(mntPath);
    if (returnVal) {
        printf("rmdir() failed with code %d\n", returnVal);
    }

    free(wrBuf);
    free(sendBuf);
    wrBuf = nullptr;
    sendBuf = nullptr;
}

int EDBInterface::open() {
    free(wrBuf);
    free(sendBuf);
    wrBuf = (char *)memalign(512, wrBufSize);
    sendBuf = (char *)memalign(512, BIN_BLOB_SIZE);

    FILE *lsblkFile;

    char line[128] = "\0";
    char *devicePath = nullptr;

    cout << "Waiting for USB CDC connection: " << flush;

    // ----------------GET 39GII BLOCK FILE PATH----------------

    for (int retry = 0; retry < 5; retry++) {

        lsblkFile = popen("lsblk -d -P -p -o HOTPLUG,VENDOR,LABEL,KNAME", "r");
        if (lsblkFile) {

            while (fgets(line, sizeof(line), lsblkFile)) { // iterate through every line
                // check if this line is what we are looking for
                if (strstr(line, "HOTPLUG=\"1\"") != nullptr &&     // Is a USB device?
                    strstr(line, "ExistOS") != nullptr /*&&*/       // Vendor should contain "ExistOS"
                    /*strstr(line, "EOSRECDISK")    != nullptr*/) { // Label should contain "EOSRECDISK"

                    char *position = strstr(line, "KNAME=\""); // locate where 'KNAME="' starts
                    if (position) {
                        position += 7;                             // jump over 'KNAME="'
                        char *endPosition = strchr(position, '"'); // locate the end of KNAME key-value pair

                        if (endPosition != nullptr && position != endPosition) { // Deal with 'KNAME="' (this line is too long) or 'KNAME=""' (empty value)
                            *endPosition = '\0';                                 // Success!
                            devicePath = position;
                            break; // retreat!
                        }
                    }
                }
                line[0] = '\0'; // in case fgets goes nuts (Is this really necessary?)
            }

            fclose(lsblkFile);
        }

        if (devicePath)
            break; // devicePath not being null indicates a success
        if (retry == 4) {
            cout << "timed out." << endl;
            return -1;
        }
        cout << ". " << flush; // each dot indicates a failed attempt
        sleep(2);
    }
    cout << "connected." << endl;

    // ----------------MOUNT 39GII DISK----------------

    struct stat st = {0};
    int returnVal;

    if (stat(mntPath, &st) == -1) {
        mkdir(mntPath, 0700);
    } else {
        if ((st.st_mode & S_IFMT) != S_IFDIR) {
            printf("A file already exists: %s\n", mntPath);
            return -1;
        }

        returnVal = umount2(mntPath, MNT_FORCE);
        if (returnVal) {
            printf("umount2() failed with code %d\n", returnVal);
            // maybe it isn't mounted
        }
    }

    returnVal = mount(devicePath, mntPath, "vfat", MS_SYNCHRONOUS, NULL); // should we use synchronous IO?
    if (returnVal) {
        printf("mount() failed with code %d\n", returnVal);
        return -1;
    }

    char *c_cmd_path = createCmdPath();
    char *c_dat_path = createDatPath();

    hCMDf = ::open(c_cmd_path, O_RDWR | O_CREAT | O_DIRECT | O_SYNC, 0666);
    hDATf = ::open(c_dat_path, O_RDWR | O_CREAT | O_DIRECT | O_SYNC, 0666);

    free(c_cmd_path);
    free(c_dat_path);

    // printf("%s %s %s", getMntPath(), getCmdPath(), getDatPath());
    return 0;
}
