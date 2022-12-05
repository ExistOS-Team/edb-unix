#include "EDBInterface.h"
#include <cstring>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <vector>

using namespace std;

vector<flashImg> imglist;

EDBInterface edb;

void showUsage() {
    cout << "Note: Serial mode is deprecated by the King and will NOT be implemented." << endl;
    cout << "Usage:" << endl;
    cout << "\t-f <bin file> <page> [b] (Specify 'b' to flash as boot image.)" << endl;
    cout << "\t-t <path> Specify temporary mount location." << endl;
    cout << "\t-r Reboot if all operations are done." << endl;
    cout << "\t-m Enter Mass Storage mode." << endl;
}

void handleInterrupt(int id) {
    printf("\nInterrupted\n");
    edb.close();
    exit(-1);
}

int main(int argc, char *argv[]) {
    struct sigaction sigHandler;
    sigHandler.sa_handler = handleInterrupt;
    sigaction(SIGINT, &sigHandler, NULL);

    bool reboot = false;
    bool mscmode = false;

    if (argc < 2) {
        showUsage();
        return -1;
    }

    if (geteuid() != 0) {
        cout << "Please run with root privileges!" << endl;
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 2 > argc) {
                showUsage();
                return -1;
            }
            flashImg item;
            memset(&item, 0, sizeof(item));
            // printf("Open: %s\n", argv[i + 1]);
            item.f = fopen(argv[i + 1], "rb");
            if (!item.f) {
                printf("Open: %s Failed.\n", argv[i + 1]);
                return -1;
            }
            item.filename = argv[i + 1];
            item.toPage = atoi(argv[i + 2]);
            if (i + 3 < argc) {
                if (strcmp(argv[i + 3], "b") == 0) {
                    printf("Set as boot img.\n");
                    item.bootImg = true;
                    i++;
                }
            }
            printf("Flash to page: %d\n", item.toPage);
            imglist.push_back(item);
            i += 2;
        }

        if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 > argc) {
                showUsage();
                return -1;
            }
            edb.c_mnt_path = argv[i + 1];
            i += 1;
        }

        if (strcmp(argv[i], "-r") == 0) {
            reboot = true;
        }

        if (strcmp(argv[i], "-m") == 0) {
            mscmode = true;
        }
    }

    if (edb.open()) {
        edb.close();
        return -1;
    }

    if (edb.ping() == false) {
        cout << "Device not responding." << endl;
        edb.close();
        return 10;
    }

    if (mscmode) {
        edb.vm_suspend();
        edb.mscmode();
    }

    edb.vm_suspend();
    for (flashImg &item : imglist) {
        edb.flash(item);
        int ret = fclose(item.f);
        if (ret) {
            printf("fclose(item.f) failed with code %d\n", ret);
        }
    }
    // edb.vm_reset();
    // edb.vm_resume();

    if (reboot) {
        edb.reboot();
    }

    edb.close();

    return 0;
}
