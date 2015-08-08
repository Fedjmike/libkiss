#pragma once

#include <sys/types.h>

typedef enum staterr {
    stat_success,
    staterr_other, staterr_nomemory,
    staterr_notexist, staterr_access, staterr_overflow,
    staterr_notdir, staterr_nametoolong, staterr_loop,
    staterr_nullresultptr, staterr_baddescriptor, staterr_badflags,
} staterr;

typedef enum fmode {
    file_regular, file_dir, file_symlink,
    file_blockdevice, file_chardevice, file_socket,
    file_fifo, file_other
} fmode;

typedef struct stat_t {
    fmode mode;
    uid_t user;
    gid_t group;
    off_t size;
} stat_t;

staterr nicestat (const char* filename, stat_t* result);
staterr nicelstat (const char* filename, stat_t* result);
staterr nicefstat (int filedescriptor, stat_t* result);

/*Translate a file mode into a statically allocated, uncapitalised string*/
const char* fmode_getstr (fmode mode);
