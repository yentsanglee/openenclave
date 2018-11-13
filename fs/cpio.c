// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "cpio.h"
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fs.h"
#include "strarr.h"
#include "strings.h"
#include "utils.h"

#define CPIO_BLOCK_SIZE 512

typedef struct _cpio_header
{
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
} cpio_header_t;

struct _fs_cpio
{
    FILE* stream;
    cpio_header_t header;
    size_t entry_size;
    long eof_offset;
    long offset;
    bool write;
};

typedef struct _entry
{
    cpio_header_t header;
    char name[FS_PATH_MAX];
    size_t size;
} entry_t;

entry_t _dot = {
    .header.magic = "070701",
    .header.ino = "00B66448",
    .header.mode = "000041ED",
    .header.uid = "00000000",
    .header.gid = "00000000",
    .header.nlink = "00000002",
    .header.mtime = "5BE31EB3",
    .header.filesize = "00000000",
    .header.devmajor = "00000008",
    .header.devminor = "00000002",
    .header.rdevmajor = "00000000",
    .header.rdevminor = "00000000",
    .header.namesize = "00000002",
    .header.check = "00000000",
    .name = ".",
    .size = sizeof(cpio_header_t) + 2,
};

entry_t _trailer = {.header.magic = "070701",
                    .header.ino = "00000000",
                    .header.mode = "00000000",
                    .header.uid = "00000000",
                    .header.gid = "00000000",
                    .header.nlink = "00000002",
                    .header.mtime = "00000000",
                    .header.filesize = "00000000",
                    .header.devmajor = "00000000",
                    .header.devminor = "00000000",
                    .header.rdevmajor = "00000000",
                    .header.rdevminor = "00000000",
                    .header.namesize = "0000000B",
                    .header.check = "00000000",
                    .name = "TRAILER!!!",
                    .size = sizeof(cpio_header_t) + 11};

#if 0
static void _dump(const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        uint8_t c = data[i];

        if (c >= ' ' && c <= '~')
            printf("%c", c);
        else
            printf("<%02x>", c);
    }

    printf("\n");
}
#endif

static bool _valid_header(const cpio_header_t* header)
{
    return memcmp(header->magic, "070701", 6) == 0;
}

static ssize_t _hex_to_ssize(const char* str, size_t len)
{
    const char* p;
    ssize_t r = 1;
    ssize_t x = 0;

    for (p = str + len; p != str; p--)
    {
        ssize_t xdigit = p[-1];
        ssize_t d;

        if (xdigit >= '0' && xdigit <= '9')
        {
            d = xdigit - '0';
        }
        else if (xdigit >= 'A' && xdigit <= 'F')
        {
            d = (xdigit - 'A') + 10;
        }
        else
            return -1;

        x += r * d;
        r *= 16;
    }

    return x;
}

static char _hex_digit(unsigned int x)
{
    switch (x)
    {
        case 0x0:
            return '0';
        case 0x1:
            return '1';
        case 0x2:
            return '2';
        case 0x3:
            return '3';
        case 0x4:
            return '4';
        case 0x5:
            return '5';
        case 0x6:
            return '6';
        case 0x7:
            return '7';
        case 0x8:
            return '8';
        case 0x9:
            return '9';
        case 0xA:
            return 'A';
        case 0xB:
            return 'B';
        case 0xC:
            return 'C';
        case 0xD:
            return 'D';
        case 0xE:
            return 'E';
        case 0xF:
            return 'F';
    }

    return '\0';
}

static void _uint_to_hex(char buf[8], unsigned int x)
{
    buf[0] = _hex_digit((x & 0xF0000000) >> 28);
    buf[1] = _hex_digit((x & 0x0F000000) >> 24);
    buf[2] = _hex_digit((x & 0x00F00000) >> 20);
    buf[3] = _hex_digit((x & 0x000F0000) >> 16);
    buf[4] = _hex_digit((x & 0x0000F000) >> 12);
    buf[5] = _hex_digit((x & 0x00000F00) >> 8);
    buf[6] = _hex_digit((x & 0x000000F0) >> 4);
    buf[7] = _hex_digit((x & 0x0000000F) >> 0);
}

static ssize_t _get_mode(const cpio_header_t* header)
{
    return _hex_to_ssize(header->mode, 8);
}

static ssize_t _get_filesize(const cpio_header_t* header)
{
    return _hex_to_ssize(header->filesize, 8);
}

static ssize_t _get_namesize(const cpio_header_t* header)
{
    return _hex_to_ssize(header->namesize, 8);
}

static int _skip_padding(FILE* stream)
{
    int ret = -1;
    long pos;
    long new_pos;

    if ((pos = ftell(stream)) < 0)
        goto done;

    new_pos = fs_round_to_multiple(pos, 4);

    if (new_pos != pos && fseek(stream, new_pos, SEEK_SET) != 0)
        goto done;

    ret = 0;
    goto done;

done:
    return ret;
}

static int _write_padding(FILE* stream, size_t n)
{
    int ret = -1;
    long pos;
    long new_pos;

    if ((pos = ftell(stream)) < 0)
        goto done;

    new_pos = fs_round_to_multiple(pos, n);

    for (size_t i = pos; i < new_pos; i++)
    {
        if (fputc('\0', stream) == EOF)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

fs_cpio_t* fs_cpio_open(const char* path, uint32_t flags)
{
    fs_cpio_t* ret = NULL;
    fs_cpio_t* cpio = NULL;
    FILE* stream = NULL;

    if (!path)
        goto done;

    if (!(cpio = calloc(1, sizeof(fs_cpio_t))))
        goto done;

    if ((flags & FS_CPIO_FLAG_CREATE))
    {
        if (!(stream = fopen(path, "wb")))
            goto done;

        if (fwrite(&_dot, 1, _dot.size, stream) != _dot.size)
            goto done;

        cpio->stream = stream;
        cpio->write = true;
        stream = NULL;
    }
    else
    {
        if (!(stream = fopen(path, "rb")))
            goto done;

        cpio->stream = stream;
        stream = NULL;
    }

    ret = cpio;
    cpio = NULL;

done:

    if (stream)
        fclose(stream);

    if (cpio)
        free(cpio);

    return ret;
}

int fs_cpio_close(fs_cpio_t* cpio)
{
    int ret = -1;

    if (!cpio || !cpio->stream)
        goto done;

    /* If file was open for write, then pad and write out the header. */
    if (cpio->write)
    {
        /* Write the trailer. */
        if (fwrite(&_trailer, 1, _trailer.size, cpio->stream) != _trailer.size)
            goto done;

        /* Pad the trailer out to the block size boundary. */
        if (_write_padding(cpio->stream, CPIO_BLOCK_SIZE) != 0)
            goto done;
    }

    fclose(cpio->stream);
    memset(cpio, 0, sizeof(fs_cpio_t));
    free(cpio);

    ret = 0;

done:
    return ret;
}

/* Read next entry: HEADER + NAME + FILEDATA + PADDING */
int fs_cpio_read_entry(fs_cpio_t* cpio, fs_cpio_entry_t* entry_out)
{
    int ret = -1;
    cpio_header_t header;
    fs_cpio_entry_t entry;
    ssize_t r;
    long file_offset;
    size_t namesize;

    if (entry_out)
        memset(entry_out, 0, sizeof(fs_cpio_entry_t));

    if (!cpio || !cpio->stream)
        goto done;

    /* Set the position to the next entry. */
    if (fseek(cpio->stream, cpio->offset, SEEK_SET) != 0)
        goto done;

    if (fread(&header, 1, sizeof(header), cpio->stream) != sizeof(header))
        goto done;

    if (!_valid_header(&header))
        goto done;

    /* Get the file size. */
    {
        if ((r = _get_filesize(&header)) < 0)
            goto done;

        entry.size = (size_t)r;
    }

    /* Get the file mode. */
    {
        if ((r = _get_mode(&header)) < 0 || r >= UINT32_MAX)
            goto done;

        entry.mode = (uint32_t)r;
    }

    /* Get the name size. */
    {
        if ((r = _get_namesize(&header)) < 0 || r >= FS_PATH_MAX)
            goto done;

        namesize = (size_t)r;
    }

    /* Read the name. */
    if (fread(&entry.name, 1, namesize, cpio->stream) != namesize)
        goto done;

    /* Skip any padding after the name. */
    if (_skip_padding(cpio->stream) != 0)
        goto done;

    /* Save the file offset. */
    file_offset = ftell(cpio->stream);

    /* Skip over the file data. */
    if (fseek(cpio->stream, entry.size, SEEK_CUR) != 0)
        goto done;

    /* Save the file offset. */
    cpio->eof_offset = ftell(cpio->stream);

    /* Skip any padding after the file data. */
    if (_skip_padding(cpio->stream) != 0)
        goto done;

    /* Save the offset of the next entry. */
    cpio->offset = ftell(cpio->stream);

    /* Rewind to the file offset. */
    if (fseek(cpio->stream, file_offset, SEEK_SET) != 0)
        goto done;

    /* Check for end-of-file. */
    if (strcmp(entry.name, "TRAILER!!!") == 0)
    {
        ret = 0;
        goto done;
    }

    *entry_out = entry;

    ret = 1;

done:
    return ret;
}

ssize_t fs_cpio_read_data(fs_cpio_t* cpio, void* data, size_t size)
{
    ssize_t ret = -1;
    size_t rem;
    ssize_t n;
    long offset;

    if (!cpio || !cpio->stream || !data)
        goto done;

    offset = ftell(cpio->stream);

    if (offset > cpio->eof_offset)
        goto done;

    rem = cpio->eof_offset - offset;

    if (size > rem)
        size = rem;

    if ((n = fread(data, 1, size, cpio->stream)) != size)
        goto done;

    ret = n;

done:
    return ret;
}

int fs_cpio_write_entry(fs_cpio_t* cpio, const fs_cpio_entry_t* entry)
{
    int ret = -1;
    cpio_header_t h;
    size_t namesize;

    if (!cpio || !cpio->stream || !entry)
        goto done;

    /* Check file type. */
    if (!(entry->mode & FS_CPIO_MODE_IFREG) &&
        !(entry->mode & FS_CPIO_MODE_IFDIR))
    {
        goto done;
    }

    /* Calculate the size of the name */
    if ((namesize = strlen(entry->name) + 1) > FS_PATH_MAX)
        goto done;

    /* Write the CPIO header */
    {
        memset(&h, 0, sizeof(h));
        strcpy(h.magic, "070701");
        _uint_to_hex(h.ino, 0);
        _uint_to_hex(h.mode, entry->mode);
        _uint_to_hex(h.uid, 0);
        _uint_to_hex(h.gid, 0);
        _uint_to_hex(h.nlink, 1);
        _uint_to_hex(h.mtime, 0x56734BA4); /* hardcode a time */
        _uint_to_hex(h.filesize, (unsigned int)entry->size);
        _uint_to_hex(h.devmajor, 8);
        _uint_to_hex(h.devminor, 2);
        _uint_to_hex(h.rdevmajor, 0);
        _uint_to_hex(h.rdevminor, 0);
        _uint_to_hex(h.namesize, (unsigned int)namesize);
        _uint_to_hex(h.check, 0);

        if (fwrite(&h, 1, sizeof(h), cpio->stream) != sizeof(h))
            goto done;
    }

    /* Write the file name. */
    {
        if (fwrite(entry->name, 1, namesize, cpio->stream) != namesize)
            goto done;

        /* Pad to four-byte boundary. */
        if (_write_padding(cpio->stream, 4) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

ssize_t fs_cpio_write_data(fs_cpio_t* cpio, const void* data, size_t size)
{
    ssize_t ret = -1;

    if (!cpio || !cpio->stream || (size && !data) || !cpio->write)
        goto done;

    if (size)
    {
        if (fwrite(data, 1, size, cpio->stream) != size)
            goto done;
    }
    else
    {
        if (_write_padding(cpio->stream, 4) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

int fs_cpio_unpack(const char* source, const char* target)
{
    int ret = -1;
    fs_cpio_t* cpio = NULL;
    int r;
    fs_cpio_entry_t entry;
    char path[FS_PATH_MAX];
    FILE* os = NULL;

    if (!source || !target)
        goto done;

    if (!(cpio = fs_cpio_open(source, 0)))
        goto done;

    if (access(target, R_OK) != 0 && mkdir(target, 0766) != 0)
        goto done;

    while ((r = fs_cpio_read_entry(cpio, &entry)) > 0)
    {
        if (strcmp(entry.name, ".") == 0)
            continue;

        fs_strlcpy(path, target, sizeof(path));
        fs_strlcat(path, "/", sizeof(path));
        fs_strlcat(path, entry.name, sizeof(path));

        if (S_ISDIR(entry.mode))
        {
            if (access(path, R_OK) && mkdir(path, entry.mode) != 0)
                goto done;
        }
        else
        {
            char data[512];
            ssize_t n;

            if (!(os = fopen(path, "wb")))
                goto done;

            while ((n = fs_cpio_read_data(cpio, data, sizeof(data))) > 0)
            {
                if (fwrite(data, 1, n, os) != n)
                    goto done;
            }

            fclose(os);
            os = NULL;
        }
    }

    ret = 0;

done:

    if (cpio)
        fs_cpio_close(cpio);

    if (os)
        fclose(os);

    return ret;
}

static int _append_file(fs_cpio_t* cpio, const char* path)
{
    int ret = -1;
    struct stat st;
    FILE* is = NULL;
    ssize_t n;

    if (!cpio || !path)
        goto done;

    /* Stat the file to get the size and mode. */
    if (stat(path, &st) != 0)
        goto done;

    /* Write the CPIO header. */
    {
        fs_cpio_entry_t ent;

        memset(&ent, 0, sizeof(ent));

        if (S_ISDIR(st.st_mode))
            st.st_size = 0;
        else
            ent.size = st.st_size;

        ent.mode = st.st_mode;

        if (fs_strlcpy(ent.name, path, sizeof(ent.name)) >= sizeof(ent.name))
            goto done;

        if (fs_cpio_write_entry(cpio, &ent) != 0)
            goto done;
    }

    /* Write the CPIO data. */
    if (!S_ISDIR(st.st_mode))
    {
        char buf[512];

        if (!(is = fopen(path, "rb")))
            goto done;

        while ((n = fread(buf, 1, sizeof(buf), is)) > 0)
        {
            if (fs_cpio_write_data(cpio, buf, n) != 0)
                goto done;
        }

        if (n < 0)
            goto done;
    }

    if (fs_cpio_write_data(cpio, NULL, 0) != 0)
        goto done;

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

static int _pack(fs_cpio_t* cpio, const char* root)
{
    int ret = -1;
    DIR* dir = NULL;
    struct dirent* ent;
    char path[FS_PATH_MAX];
    fs_strarr_t dirs = FS_STRARR_INITIALIZER;

    if (!(dir = opendir(root)))
        goto done;

    /* Append this directory to the CPIO archive. */
    if (strcmp(root, ".") != 0)
    {
        if (_append_file(cpio, root) != 0)
            goto done;
    }

    /* Find all children of this directory. */
    while ((ent = readdir(dir)))
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        *path = '\0';

        if (strcmp(root, ".") != 0)
        {
            fs_strlcat(path, root, sizeof(path));
            fs_strlcat(path, "/", sizeof(path));
        }

        fs_strlcat(path, ent->d_name, sizeof(path));

        /* Append to dirs[] array */
        if (ent->d_type & FS_DT_DIR)
        {
            if (fs_strarr_append(&dirs, path) != 0)
                goto done;
        }
        else
        {
            /* Append this file to the CPIO archive. */
            if (_append_file(cpio, path) != 0)
                goto done;
        }
    }

    /* Recurse into child directories */
    {
        size_t i;

        for (i = 0; i < dirs.size; i++)
        {
            if (_pack(cpio, dirs.data[i]) != 0)
                goto done;
        }
    }

    ret = 0;

done:

    if (dir)
        closedir(dir);

    fs_strarr_release(&dirs);

    return ret;
}

int fs_cpio_pack(const char* source, const char* target)
{
    int ret = -1;
    fs_cpio_t* cpio = NULL;
    FILE* os = NULL;
    char cwd[FS_PATH_MAX];

    if (!source || !target)
        goto done;

    if (!(cpio = fs_cpio_open(target, FS_CPIO_FLAG_CREATE)))
        goto done;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        goto done;

    if (chdir(source) != 0)
        goto done;

    if (_pack(cpio, ".") != 0)
        goto done;

    if (chdir(cwd) != 0)
        goto done;

    ret = 0;

done:

    if (cpio)
        fs_cpio_close(cpio);

    if (os)
        fclose(os);

    return ret;
}
