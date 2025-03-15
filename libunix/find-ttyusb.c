// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
    "ttyUSB",	// linux
    "ttyACM",   // linux
    "cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int timesort(const struct dirent **a, const struct dirent **b) {
    struct stat st1, st2;
    char *a_path = strdupf("/dev/%s", (*a)->d_name);
    char *b_path = strdupf("/dev/%s", (*b)->d_name);
    stat(a_path, &st1);
    stat(b_path, &st2);
    free(a_path);
    free(b_path);
    return (st1.st_mtime > st2.st_mtime) ? -1 : 1;
}

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    for (int i = 0; ttyusb_prefixes[i] != 0; i++) {

        // trace("ttyusb_prefixes[%d]: %s\n", i, ttyusb_prefixes[i]);
        // trace("d->d_name: %s\n", d->d_name);
        if (strncmp(d->d_name, ttyusb_prefixes[i], strlen(ttyusb_prefixes[i])) == 0) {
            return 1;
        }
    }
    return 0;
}



// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    struct dirent **dir_list = NULL;
    char *path = "/dev";
    int count = scandir(path, &dir_list, filter, alphasort);
    if (count == 1) {
        // return malloc'd name
        char *name = strdupf("%s/%s", path, dir_list[0]->d_name);
        free(dir_list[0]);
        return name;
    }
    // Panic
    panic("find_ttyusb: found %d devices\n", count);
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    struct dirent **dir_list = NULL;
    char *path = "/dev";
    int count = scandir(path, &dir_list, filter, timesort);
    if (count == 0) {
        panic("find_ttyusb_last: found 0 devices\n");
    }
    char *name = strdupf("%s/%s", path, dir_list[0]->d_name);
    free(dir_list[0]);
    return name;
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    struct dirent **dir_list = NULL;
    char *path = "/dev";
    int count = scandir(path, &dir_list, filter, timesort);
    if (count == 0) {
        panic("find_ttyusb_last: found 0 devices\n");
    }
    char *name = strdupf("%s/%s", path, dir_list[count - 1]->d_name);
    free(dir_list[count - 1]);
    return name;
}
