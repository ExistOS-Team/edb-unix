#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "EDBInterface.h"

using namespace std;

vector<flashImg> imglist;

EDBInterface edb;

void showUsage() {
    cout << "Note: Serial mode is deprecated by the King and will NOT be implemented." << endl;
    cout << "Usage:" << endl;
    cout << "\t-f <bin file> <page> [b] (Specify 'b' to flash as boot image.)" << endl;
    cout << "\t-r Reboot if all operations done." << endl;
    cout << "\t-m Enter MSC mode. (Is this working at all?)" << endl;
}

int main(int argc, char *argv[]) {
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
                    printf("Set boot img.\n");
                    item.bootImg = true;
                }
            }
            printf("Flash to page: %d\n", item.toPage);
            imglist.push_back(item);
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