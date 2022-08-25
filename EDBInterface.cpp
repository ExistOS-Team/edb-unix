#include "EDBInterface.h"

#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <malloc.h>
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

void EDBInterface::reset(bool mode) {
    //DWORD wrcnt;
    //bool ret;
    if (USBHighSpeed) {
        // Sleep(100);
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // com.SetRTS(mode);
        // Sleep(20);
        // com.SetDTR(false);
    }
}

bool EDBInterface::waitStr(char *str) {
    int len = strlen(str);
    int retry = 2;
    //DWORD cnt;
    //char buf[128];
    //memset(buf, 0, 128);
    int ret;
    if (USBHighSpeed) {
        while (retry > 0) {
            cout << "Waiting Sync...\r";
            memset(wrBuf, 0, wrBufSize);
            lseek(hCMDf, 0, SEEK_SET);
            ret = read(hCMDf, wrBuf, wrBufSize);  // ReadFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
            if (ret) {
                if (strcmp(str, wrBuf) == 0) {
                    return true;
                } else {
                    cout << "Sync Failed... Retry:" << retry << endl;
                    this->reset(EDB_MODE_TEXT);
                    retry--;
                }
            } else {
                cout << "Sync Failed... Retry:" << retry << endl;
                this->reset(EDB_MODE_TEXT);
                retry--;
            }
        }
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // while (retry > 0) {
        //     cout << "Waiting Sync...\r";
        //     com.Read(buf, len, &cnt);
        //     if (cnt > 0) {
        //         if (strcmp(str, buf) == 0) {
        //             return true;
        //         } else {
        //             cout << "Sync Failed... Retry:" << retry << endl;
        //             this->reset(EDB_MODE_TEXT);
        //             retry--;
        //         }
        //     } else {
        //         cout << "Sync Failed... Retry:" << retry << endl;
        //         this->reset(EDB_MODE_TEXT);
        //         retry--;
        //     }
        // }
    }
    return false;
}

void EDBInterface::wrStr(const char *str) {
    if (!str) {
        return;
    }
    //DWORD cnt;
    int len = strlen(str);
    if (USBHighSpeed) {
        memset(wrBuf, 0, wrBufSize);
        strcpy(wrBuf, str);
        lseek(hCMDf, 0, SEEK_SET);
        write(hCMDf, wrBuf, wrBufSize);  // WriteFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // com.WriteStr(str);
    }
}

bool EDBInterface::wrDat(char *dat, size_t len) {
    int ret = 0;
    //DWORD cnt;
    if (USBHighSpeed) {
        lseek(hDATf, 0, SEEK_SET);

        ret = write(hDATf, dat, len);  // WriteFile(hDATf, dat, len, &cnt, NULL);

        if (ret) {
            return true;
        }
        return false;
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // return com.Write(dat, len);
    }
}

bool EDBInterface::rdDat(char *dat, size_t len, size_t *rbcnt) {
    //DWORD cnt;
    if (USBHighSpeed) {
        memset(wrBuf, 0, wrBufSize);
        lseek(hCMDf, 0, SEEK_SET);
        int ret = read(hCMDf, wrBuf, wrBufSize);  // ReadFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
        memcpy(dat, wrBuf, len);
        *rbcnt = len;
        if (ret) {
            return true;
        }
        return false;
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // return com.Read(dat, len, (DWORD *)rbcnt);
    }
}

bool EDBInterface::eraseBlock(unsigned int block) {
    char cmdbuf[64];
    memset(cmdbuf, 0, sizeof(cmdbuf));
    sprintf(cmdbuf, "ERASEB:%d\n", block);
    this->wrStr(cmdbuf);
    if (this->waitStr((char *)"EROK\n") == false) {
        printf("Erase Block Time Out:%d\n", block);
        return false;
    }
    return true;
}

int EDBInterface::flash(flashImg item) {
    char cmdbuf[64];
    size_t cnt;
    size_t rbcnt;
    this->reset(EDB_MODE_TEXT);
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

        this->reset(EDB_MODE_BIN);
        this->wrDat(sendBuf, BIN_BLOB_SIZE);
        this->reset(EDB_MODE_TEXT);

        this->wrStr("BUFCHK\n");

        this->rdDat(cmdbuf, 10, &rbcnt);

        sscanf(cmdbuf, "CHKSUM:%02x\n", &rcshkdum);

        if (rcshkdum != chksum) {
            printf("chksum error, w:%02x r:%02x\n", chksum, rcshkdum);
            return false;
        }

        if (block_cnt != last_block) {
            if (eraseBlock(block_cnt) == false) {
                printf("Erase Block Time Out:%d\n", block_cnt);
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
            printf("Program Page Time Out:%d\n", page_cnt);
            return false;
        }

        if (page_cnt % 200 == 0) {
            long long speed = BIN_BLOB_SIZE / (getTime() - st);
            cout << "Upload: " << ftell(item.f) << "/" << fsize;
            cout << " Page:" << page_cnt << " Block:" << block_cnt << " ";
            printf(" chksum:%02x==%02x, %lld KB/s", chksum, rcshkdum, speed);
            cout << "  remaining:" << (fsize - ftell(item.f)) / speed / 1000 << "s        \r";
            fflush(stdout);
        }

        page_cnt += BIN_BLOB_SIZE / 2048;
        last_block = block_cnt;
    } while (cnt > 0);

    if (item.bootImg) {
        cout << "\nSetting NCB..." << endl;
        memset(cmdbuf, 0, sizeof(cmdbuf));
        sprintf(cmdbuf, "MKNCB:%d,%d\n", item.toPage / 64, fsize / 2048);
        this->wrStr(cmdbuf);
        if (this->waitStr((char *)"MKOK\n") == false) {
            printf("Setting NCB Page Time Out:%d\n", item.toPage / 64);
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
    char buf[16];
    //DWORD cnt;
    int retry = 5;
    int ret;

    if (USBHighSpeed) {
        while (retry) {
            memset(wrBuf, 0, wrBufSize);
            strcpy(wrBuf, "PING\n");
            lseek(hCMDf, 0, SEEK_SET);  // SetFilePointer(hCMDf, 0, NULL, FILE_BEGIN);
            ret = write(hCMDf, wrBuf, wrBufSize);  // ret = WriteFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
            if (ret > 0) {
                lseek(hCMDf, 0, SEEK_SET);  // SetFilePointer(hCMDf, 0, NULL, FILE_BEGIN);
                ret = read(hCMDf, wrBuf, wrBufSize);  // ret = ReadFile(hCMDf, wrBuf, sizeof(wrBuf), &cnt, NULL);
                if (ret > 0) {
                    if (strcmp(wrBuf, "PONG\n") == 0) {
                        return true;
                    }
                    sleep(2);
                    retry--;
                }
            } else {
                this->reset(EDB_MODE_TEXT);
                sleep(2);
                retry--;
            }
        }
    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // while (retry > 0) {
        //     com.WriteStr("PING\n");
        //     memset(buf, 0, 16);
        //     if (com.Read(buf, 5, &cnt)) {
        //         if (strcmp(buf, "PONG\n") == 0) {
        //             return true;
        //         }
        //         sleep(2);
        //         retry--;
        //     } else {
        //         sleep(2);
        //         this->reset(EDB_MODE_TEXT);
        //         retry--;
        //     }
        // }
    }

    return false;
}

void EDBInterface::close() {
    if (USBHighSpeed) {
        ::close(hCMDf);  // CloseHandle(hCMDf);
        ::close(hDATf);  // CloseHandle(hDATf);

        char *c_mnt_path = "/mnt/edbMount";

        int returnVal = umount(c_mnt_path);
        if (returnVal) {
            printf("umount() failed with code %d\n", returnVal);
        }

        returnVal = rmdir(c_mnt_path);
        if (returnVal) {
            printf("rmdir() failed with code %d\n", returnVal);
        }

    } else {
    }

    free(wrBuf);
    free(sendBuf);
    wrBuf = nullptr;
    sendBuf = nullptr;
}

int EDBInterface::open(bool mode) {
    free(wrBuf);
    free(sendBuf);
    wrBuf = (char*) memalign(512, wrBufSize);
    sendBuf = (char*) memalign(512, BIN_BLOB_SIZE);

    USBHighSpeed = mode;
    if (USBHighSpeed) {
        FILE *lsblkFile;
        char *c_mnt_path = "/mnt/edbMount";
        char *c_cmd_path = "/mnt/edbMount/cmd_port";
        char *c_dat_path = "/mnt/edbMount/dat_port";

        char line[128] = "\0";
        char *devicePath = nullptr;

        cout << "Waiting for USB CDC connection: " << flush;

        // ----------------GET 39GII BLOCK FILE PATH----------------

        for (int retry = 0; retry < 5; retry++) {

            lsblkFile = popen("lsblk -d -P -p -o HOTPLUG,VENDOR,LABEL,KNAME", "r");
            if (lsblkFile) {

                while (fgets(line, sizeof(line), lsblkFile)) {  // iterate through every line
                    // check if this line is what we are looking for
                    if (strstr(line, "HOTPLUG=\"1\"") != nullptr &&  // Is a USB device?
                        strstr(line, "ExistOS")       != nullptr /*&&*/  // Vendor should contain "ExistOS"
                        /*strstr(line, "EOSRECDISK")    != nullptr*/) {  // Label should contain "EOSRECDISK"

                        char *position = strstr(line, "KNAME=\"");  // locate where 'KNAME="' starts
                        if (position) {
                            position += 7; // jump over 'KNAME="'
                            char *endPosition = strchr(position, '"');  // locate the end of KNAME key-value pair

                            if (endPosition != nullptr && position != endPosition) {  // Deal with 'KNAME="' (this line is too long) or 'KNAME=""' (empty value)
                                *endPosition = '\0';  // Success!
                                devicePath = position;
                                break;  // retreat!
                            }
                        }
                    }
                    line[0] = '\0';  // in case fgets goes nuts (Is this really necessary?)
                }

                fclose(lsblkFile);
            }

            if (devicePath) break;  // devicePath not being null indicates a success
            if (retry == 4) {
                cout << " timed out." << endl;
                return -1;
            }
            cout << "." << flush;  // every dot indicates a failed attempt
            sleep(2);
        }
        cout << " connected." << endl;

        // ----------------MOUNT 39GII DISK----------------

        struct stat st = {0};
        
        if (stat(c_mnt_path, &st) == -1) {
            mkdir(c_mnt_path, 0700);
        } else if ((st.st_mode & S_IFMT) != S_IFDIR) {
            printf("File already exists: %s\n", c_mnt_path);
            return -1;
        }

        int returnVal = umount2(c_mnt_path, MNT_FORCE);
        if (returnVal) {
            printf("umount() failed. (%d)\n", returnVal);
        }

        returnVal = mount(devicePath, c_mnt_path, "vfat", MS_SYNCHRONOUS, NULL);  // should we use synchronous IO?
        if (returnVal) {
            printf("mount() failed. (%d)\n", returnVal);
            return -1;
        }

        hCMDf = ::open(c_cmd_path, O_RDWR | O_CREAT | O_DIRECT | O_SYNC, 0666);
        hDATf = ::open(c_dat_path, O_RDWR | O_CREAT | O_DIRECT | O_SYNC, 0666);

    } else {
        cerr << "TODO" << endl;
        close();
        exit(-1);
        // string COM = findUsbSerialCom();
        // while (COM == "NONE") {
        //     cout << "Waiting USB CDC Connect..." << endl;
        //     sleep(2);
        //     COM = findUsbSerialCom();
        // }
        // cout << "Select:" << COM << endl;
        // if (com.Open(COM) == false) {
        //     cout << "Open Serial Port:" << COM << " Failed.\n"
        //          << endl;
        //     sleep(2);
        //     return -1;
        // }
        // com.Set();
    }

    return 0;
}
