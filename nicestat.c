#define _XOPEN_SOURCE 500
#include "nicestat.h"

#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

static staterr nicestat_translate (bool error, struct stat st, stat_t* result) {
    /*Translate the error, if any*/
    if (error) {
        switch (errno) {
        case ENOMEM: return staterr_nomemory;

        case ENOENT: return staterr_notexist;
        case EACCES: return staterr_access;
        case EOVERFLOW: return staterr_overflow;

        case ENOTDIR: return staterr_notdir;
        case ENAMETOOLONG: return staterr_nametoolong;
        case ELOOP: return staterr_loop;

        case EFAULT: return staterr_nullresultptr;
        case EBADF: return staterr_baddescriptor;
        case EINVAL: return staterr_badflags;

        default: return staterr_other;
        }
    }

    /*Translate the mode*/

    fmode mode;

    switch (st.st_mode & S_IFMT) {
    case S_IFREG: mode = file_regular; break;
    case S_IFDIR: mode = file_dir; break;
    case S_IFLNK: mode = file_symlink; break;
    case S_IFBLK: mode = file_blockdevice; break;
    case S_IFCHR: mode = file_chardevice; break;
    case S_IFSOCK: mode = file_socket; break;
    case S_IFIFO: mode = file_fifo; break;
    default: mode = file_other;
    }

    /*Map the struct*/

    *result = (stat_t) {
        .mode = mode,
        .user = st.st_uid,
        .group = st.st_gid,
        .size = st.st_size
    };

    return stat_success;
}

staterr nicestat (const char* filename, stat_t* result) {
    struct stat st;
    bool error = stat(filename, &st);
    return nicestat_translate(error, st, result);
}

staterr nicelstat (const char* filename, stat_t* result) {
    struct stat st;
    bool error = lstat(filename, &st);
    return nicestat_translate(error, st, result);
}

staterr nicefstat (int filedescriptor, stat_t* result) {
    struct stat st;
    bool error = fstat(filedescriptor, &st);
    return nicestat_translate(error, st, result);
}

const char* fmode_getstr (fmode mode) {
    switch (mode) {
    case file_regular: return "regular file";
    case file_dir: return "directory";
    case file_symlink: return "symbolic link";
    case file_blockdevice: return "block device";
    case file_chardevice: return "character device";
    case file_socket: return "socket";
    case file_fifo: return "FIFO/pipe";
    case file_other: return "unknown file type";
    }
    
    return "<unhandled file type>";
}
