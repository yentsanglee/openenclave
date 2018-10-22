#ifndef _oe_filesys_h
#define _oe_filesys_h

#include <fcntl.h>
#include <limits.h>

typedef struct _oe_file oe_file_t;
typedef struct _oe_filesys oe_filesys_t;

struct _oe_filesys
{
    oe_file_t* (*open)(
        oe_filesys_t* filesys, 
        const char* path, 
        int flags, 
        mode_t mode);
};

struct _oe_file
{
    ssize_t (*read)(oe_file_t* file, void *buf, size_t count);

    int (*close)(oe_file_t* file);
};

/* Mount the file system at the given path. */
int oe_filesys_mount(oe_filesys_t* filesys, const char* path);

// Lookup the file system that contains the given path. Path_out will contain
// the absolute path of the file within the file system.
oe_filesys_t* oe_filesys_lookup(const char* path, char path_out[PATH_MAX]);

#endif /* _oe_filesys_h */
