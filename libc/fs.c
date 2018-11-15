#include <stdio.h>
#include <openenclave/internal/fs.h>

oe_file_t* oe_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args)
{
    if (!fs || !fs->fs_fopen)
        return NULL;

    return fs->fs_fopen(fs, path, mode, args);
}

int32_t oe_fclose(oe_file_t* file)
{
    if (!file || !file->f_fclose)
        return -1;

    return file->f_fclose(file);
}

size_t oe_fread(void* ptr, size_t size, size_t nmemb, oe_file_t* file)
{
    if (!file || !file->f_fread)
        return -1;

    return file->f_fread(ptr, size, nmemb, file);
}

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, oe_file_t* file)
{
    if (!file || !file->f_fwrite)
        return -1;

    return file->f_fwrite(ptr, size, nmemb, file);
}

int64_t oe_ftell(oe_file_t* file)
{
    if (!file || !file->f_ftell)
        return -1;

    return file->f_ftell(file);
}

int32_t oe_fseek(oe_file_t* file, int64_t offset, int whence)
{
    if (!file || !file->f_fseek)
        return -1;

    return file->f_fseek(file, offset, whence);
}

int32_t oe_fflush(oe_file_t* file)
{
    if (!file || !file->f_fflush)
        return -1;

    return file->f_fflush(file);
}

int32_t oe_ferror(oe_file_t* file)
{
    if (!file || !file->f_ferror)
        return -1;

    return file->f_ferror(file);
}

int32_t oe_feof(oe_file_t* file)
{
    if (!file || !file->f_feof)
        return -1;

    return file->f_feof(file);
}

int32_t oe_clearerr(oe_file_t* file)
{
    if (!file || !file->f_clearerr)
        return -1;

    return file->f_clearerr(file);
}

oe_dir_t* oe_opendir(oe_fs_t* fs, const char* name, const void* args)
{
    if (!fs || !fs->fs_opendir)
        return NULL;

    return fs->fs_opendir(fs, name, args);
}

int32_t oe_readdir(oe_dir_t* dir, oe_dirent_t* entry, oe_dirent_t** result)
{
    if (!dir || !dir->d_readdir)
        return -1;

    return dir->d_readdir(dir, entry, result);
}

int32_t oe_closedir(oe_dir_t* dir)
{
    if (!dir || !dir->d_closedir)
        return -1;

    return dir->d_closedir(dir);
}

int32_t oe_release(oe_fs_t* fs)
{
    if (!fs || !fs->fs_release)
        return -1;

    return fs->fs_release(fs);
}

int32_t oe_stat(oe_fs_t* fs, const char* path, oe_stat_t* stat)
{
    if (!fs || !fs->fs_stat)
        return -1;

    return fs->fs_stat(fs, path, stat);
}

int32_t oe_unlink(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_unlink)
        return -1;

    return fs->fs_unlink(fs, path);
}

int32_t oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path)
{
    if (!fs || !fs->fs_rename)
        return -1;

    return fs->fs_rename(fs, old_path, new_path);
}

int32_t oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode)
{
    if (!fs || !fs->fs_mkdir)
        return -1;

    return fs->fs_mkdir(fs, path, mode);
}

int32_t oe_rmdir(oe_fs_t* fs, const char* path)
{
    if (!fs || !fs->fs_rmdir)
        return -1;

    return fs->fs_rmdir(fs, path);
}
