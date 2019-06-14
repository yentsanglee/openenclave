// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <limits.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>

#define COLOR_RED "\033[0;31m"
#define COLOR_NONE "\033[0m"

using namespace std;

static const char* arg0;

__attribute__((format(printf, 1, 2))) void err(const char* fmt, ...)
{
    fprintf(stderr, "%s", COLOR_RED);

    fprintf(stderr, "%s: error: ", arg0);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n\n");

    fprintf(stderr, "%s", COLOR_NONE);
    fflush(stderr);

    exit(1);
}

__attribute__((format(printf, 2, 3))) void err(
    const vector<string>& args,
    const char* fmt,
    ...)
{
    fprintf(stderr, "%s", COLOR_RED);
    fprintf(stderr, "%s: error: ", arg0);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    for (size_t i = 0; i < args.size(); i++)
    {
        fprintf(stderr, "%s", args[i].c_str());

        if (i + 1 != args.size())
            fprintf(stderr, " ");
    }

    fprintf(stderr, "\n\n");

    fprintf(stderr, "%s", COLOR_NONE);
    fflush(stderr);

    exit(1);
}

static bool is_dir(const string& path)
{
    struct stat buf;

    if (stat(path.c_str(), &buf) != 0)
        return false;

    return S_ISDIR(buf.st_mode);
}

static string get_cdb_root(void)
{
    static string _path;
    const char* path;

    if (_path.size())
        return _path;

    if (!(path = getenv("OE_CDB_ROOT")))
        err("Invalid or unset OE_CDB_ROOT environment variable");

    _path = path;

    return _path;
}

struct sha256_string
{
    char buf[65];
};

static int compute_hash(const char* path, sha256_string* string)
{
    int ret = -1;
    FILE* is = NULL;
    char buf[4096];
    size_t n;
    SHA256_CTX ctx;
    unsigned char hash[32];

    if (string)
        memset(string, 0, sizeof(sha256_string));

    if (!path || !string)
        goto done;

    if (!(is = fopen(path, "r")))
        goto done;

    if (!SHA256_Init(&ctx))
        goto done;

    while ((n = fread(buf, 1, sizeof(buf), is)) > 0)
    {
        if (!SHA256_Update(&ctx, buf, n))
            goto done;
    }

    if (!SHA256_Final(hash, &ctx))
        goto done;

    // Convert hash to string:
    for (size_t i = 0; i < sizeof(hash); i++)
    {
        snprintf(&string->buf[i * 2], 3, "%02x", hash[i]);
    }

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

static int split(const char* str, const char* delim, vector<string>& v)
{
    int ret = -1;
    char* copy = NULL;
    char* save;
    char* p;

    if (!str || !delim)
        goto done;

    if (!(copy = strdup(str)))
        goto done;

    for (p = strtok_r(copy, delim, &save); p; p = strtok_r(NULL, delim, &save))
        v.push_back(p);

    ret = 0;

done:
    free(copy);

    return ret;
}

static void print_args(const vector<string>& args, const char* color)
{
    printf("%s", color);

    for (size_t i = 0; i < args.size(); i++)
    {
        printf("%s", args[i].c_str());

        if (i + 1 != args.size())
            printf(" ");
    }

    printf("%s\n", COLOR_NONE);
}

static void argv_to_args(
    const int argc,
    const char* argv[],
    vector<string>& args)
{
    args.clear();

    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);
}

static char** args_to_argv(const vector<string>& args)
{
    char** ret = NULL;
    char** new_argv;
    size_t alloc_size;

    /* Calculate the total allocation size. */
    {
        /* Calculate the size of the null-terminated array of pointers. */
        alloc_size = (args.size() + 1) * sizeof(char*);

        /* Calculate the sizeo of the individual zero-terminated strings. */
        for (size_t i = 0; i < args.size(); i++)
            alloc_size += strlen(args[i].c_str()) + 1;
    }

    /* Allocate space for the array followed by strings. */
    if (!(new_argv = (char**)malloc(alloc_size)))
        return NULL;

    /* Copy each string. */
    {
        char* p = (char*)&new_argv[args.size() + 1];

        for (size_t i = 0; i < args.size(); i++)
        {
            size_t len = strlen(args[i].c_str());

            memcpy(p, args[i].c_str(), len + 1);
            new_argv[i] = p;
            p += len + 1;
        }

        new_argv[args.size()] = NULL;
    }

    ret = new_argv;

    return ret;
}

static int which(const char* filename, string& full_path)
{
    int ret = -1;
    const char* path;
    vector<string> v;

    full_path.clear();

    if (!filename)
        goto done;

    /* Handle absolute paths. */
    if (filename[0] == '/')
    {
        if (access(filename, X_OK) != 0)
            goto done;

        full_path = filename;
        ret = 0;
        goto done;
    }

    /* Handle relative paths. */
    if (strchr(filename, '/'))
    {
        char cwd[PATH_MAX];

        if (!getcwd(cwd, sizeof(cwd)))
            goto done;

        string tmp = string(cwd) + "/" + filename;

        if (access(tmp.c_str(), X_OK) != 0)
            goto done;

        full_path = tmp;
        ret = 0;
        goto done;
    }

    /* Search for filename on the path. */
    {
        if (!(path = getenv("PATH")))
            goto done;

        if (split(path, ":", v) != 0)
            goto done;

        for (size_t i = 0; i < v.size(); i++)
        {
            string tmp = v[i] + "/" + filename;

            if (access(tmp.c_str(), X_OK) == 0)
            {
                full_path = tmp;
                ret = 0;
                goto done;
            }
        }
    }

done:
    return ret;
}

static int exec(const vector<string>& args, int* exit_status)
{
    int ret = -1;
    int pid;
    char** new_argv = NULL;
    int r;
    int status;
    string path;

    if (exit_status)
        *exit_status = 0;

    if (!(new_argv = args_to_argv(args)))
        goto done;

    if ((pid = fork()) < 0)
        goto done;

    /* Resolve the location of the program. */
    if (which(args[0].c_str(), path) != 0)
        goto done;

    /* If child */
    if (pid == 0)
    {
        execv(path.c_str(), new_argv);
        exit(1);
    }

    /* Wait for the child to exit (ignore interrupts). */
    while ((r = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
        ;

    if (r != pid)
        goto done;

    if (exit_status)
        *exit_status = WEXITSTATUS(status);

    ret = 0;

done:
    free(new_argv);

    return ret;
}

static string base_name(const char* path)
{
    char* slash = strrchr(path, '/');

    if (slash)
        return string(slash + 1);
    else
        return string(path);
}

static void extract_opt_args(
    vector<string>& args,
    const string& opt,
    vector<string>& options)
{
    vector<string> result_args;

    options.clear();

    for (size_t i = 0; i < args.size();)
    {
        const string& arg = args[i];

        if (arg.substr(0, opt.size()) == opt)
        {
            if (arg == opt)
            {
                if (i + 1 != args.size())
                {
                    options.push_back(args[i + 1]);
                    i += 2;
                }
            }
            else
            {
                options.push_back(arg.substr(2));
                i++;
            }
        }
        else
        {
            result_args.push_back(arg);
            i++;
        }
    }

    args = result_args;
}

static void extract_flags(vector<string>& args, vector<string>& flags)
{
    vector<string> result_args;

    flags.clear();

    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];

        if (arg[0] == '-')
        {
            flags.push_back(arg);
        }
        else
        {
            result_args.push_back(arg);
        }
    }

    args = result_args;
}

static const char* source_exts[] = {
    ".c",
    ".cc",
    ".cpp",
    ".s",
    ".S",
    NULL,
};

static const char* object_exts[] = {
    ".o",
    NULL,
};

static const char* archive_exts[] = {
    ".a",
    NULL,
};

static void extract_files(
    vector<string>& args,
    const char* exts[],
    vector<string>& sources)
{
    vector<string> result_args;

    sources.clear();

    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];
        bool found = false;

        for (size_t j = 0; exts[j]; j++)
        {
            const char* ext = exts[j];
            size_t pos = arg.find(ext);

            if (pos != string::npos && pos == (arg.size() - strlen(ext)))
            {
                sources.push_back(arg);
                found = true;
                break;
            }
        }

        if (!found)
            result_args.push_back(arg);
    }

    args = result_args;
}

static void extract_shared_objects(vector<string>& args, vector<string>& shobjs)
{
    vector<string> result_args;

    shobjs.clear();

    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];

        if (arg[0] != '-')
        {
            size_t pos = arg.find(".so");

            if (pos != string::npos && pos == (arg.size() - strlen(".so")))
            {
                shobjs.push_back(arg);
                continue;
            }

            if (arg.find(".so.") != string::npos)
            {
                shobjs.push_back(arg);
                continue;
            }
        }

        result_args.push_back(arg);
    }

    args = result_args;
}

static void extract_sources(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, source_exts, sources);
}

static void extract_objects(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, object_exts, sources);
}

static void extract_archives(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, archive_exts, sources);
}

static FILE* open_cdb_spec()
{
    const string root = get_cdb_root();

    if (root.empty())
        return NULL;

    const string path = root + "/cdb.spec";

    return fopen(path.c_str(), "a");
}

static int resolve_path(const string& path, string& relative, string& full)
{
    int ret = -1;
    char cwd[PATH_MAX];
    string tmp_path;
    const string root = get_cdb_root();

    if (!getcwd(cwd, sizeof(cwd)))
        goto done;

    if (path[0] == '/')
    {
        tmp_path = path;
        full = path;
    }
    else
    {
        tmp_path = string(cwd) + "/" + path;

        if (tmp_path.substr(0, root.size()) != root)
            goto done;

        if (tmp_path[root.size()] != '/')
            goto done;

        full = tmp_path;
        tmp_path = tmp_path.substr(root.size() + 1);
    }

    relative = tmp_path;

    ret = 0;

done:
    return ret;
}

static int handle_cc_subcommand(const vector<string>& args_)
{
    int exit_status;
    vector<string> args = args_;
    FILE* os = NULL;
    FILE* log = NULL;
    char* buf = NULL;
    size_t len = 0;
    string target_name;

    if (!(os = open_memstream(&buf, &len)))
        err("open_memstream() failed");

    // Open the CDB log:
    if (!(log = open_cdb_spec()))
        err("failed to open the log file");

    // Print the target:
    {
        vector<string> targets;

        extract_opt_args(args, "-o", targets);

        if (targets.size() != 1)
            err(args_, "cannot resolve target");

        string relative;
        string full;

        if (resolve_path(targets[0], relative, full) != 0)
            err("failed to resolve target: %s", targets[0].c_str());

        target_name = full;

        fprintf(os, "%s:\n", relative.c_str());
    }

    // Print CURDIR line:
    {
        char cwd[PATH_MAX];

        if (!getcwd(cwd, sizeof(cwd)))
            err("failed to get current directory");

        fprintf(os, "CURDIR=%s\n", cwd);
    }

#if 0
    // Print the COMMAND:
    {
        fprintf(os, "COMMAND=");

        for (size_t i = 0; i < args.size(); i++)
        {
            fprintf(os, "%s", args[i].c_str());

            if (i + 1 != args.size())
                printf(" ");
        }

        fprintf(os, "\n");
    }
#endif

    // Print CC line:
    fprintf(os, "CC=%s\n", args[0].c_str());
    args.erase(args.begin());

    // Print INCLUDE lines:
    {
        vector<string> includes;
        extract_opt_args(args, "-I", includes);

        for (size_t i = 0; i < includes.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(includes[i], relative, full) != 0)
                err(args_, "cannot resovle path: %s", includes[i].c_str());

            if (!is_dir(full))
                err(args_, "no such include directory: %s", full.c_str());

            fprintf(os, "INCLUDE=%s\n", relative.c_str());
        }
    }

    // Print DEFINE lines:
    {
        vector<string> defines;
        extract_opt_args(args, "-D", defines);

        for (size_t i = 0; i < defines.size(); i++)
            fprintf(os, "DEFINE=%s\n", defines[i].c_str());
    }

    // Print LDLIB lines:
    {
        vector<string> libs;
        extract_opt_args(args, "-l", libs);

        for (size_t i = 0; i < libs.size(); i++)
            fprintf(os, "LDLIB=%s\n", libs[i].c_str());
    }

    // Print LDPATH lines:
    {
        vector<string> libs;
        extract_opt_args(args, "-L", libs);

        for (size_t i = 0; i < libs.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(libs[i], relative, full) != 0)
                err(args_, "cannot resovle path: %s", libs[i].c_str());

            if (!is_dir(full))
                err(args_, "no such lib directory: %s", full.c_str());

            fprintf(os, "LDPATH=%s\n", relative.c_str());
        }
    }

    // Discard these options and their arguments:
    {
        vector<string> tmp;
        extract_opt_args(args, "-MF", tmp);
        extract_opt_args(args, "-MT", tmp);
        extract_opt_args(args, "-MMD", tmp);
    }

    // Print SOURCE lines:
    {
        vector<string> sources;
        extract_sources(args, sources);

        for (size_t i = 0; i < sources.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(sources[i], relative, full) != 0)
                err(args_, "cannot resovle path: %s", sources[i].c_str());

            if (access(full.c_str(), F_OK) != 0)
                err(args_, "no such source file: %s", full.c_str());

            fprintf(os, "SOURCE=%s\n", relative.c_str());
        }
    }

    // Print OBJECT lines:
    {
        vector<string> objects;
        extract_objects(args, objects);

        for (size_t i = 0; i < objects.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(objects[i], relative, full) != 0)
                err(args_, "cannot resovle path: %s", objects[i].c_str());

            if (access(full.c_str(), F_OK) != 0)
                err(args_, "no such object file: %s", full.c_str());

            fprintf(os, "OBJECT=%s\n", relative.c_str());

            sha256_string hash;

            if (compute_hash(full.c_str(), &hash) != 0)
                err("failed to compute object hash: %s", full.c_str());

            fprintf(os, "OBJHASH=%s\n", hash.buf);
        }
    }

    // Print ARCHIVE lines:
    {
        vector<string> archives;
        extract_archives(args, archives);

        for (size_t i = 0; i < archives.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(archives[i], relative, full) != 0)
                err(args_, "cannot resovle path: %s", archives[i].c_str());

            if (access(full.c_str(), F_OK) != 0)
                err(args_, "no such archive: %s", full.c_str());

            fprintf(os, "ARCHIVE=%s\n", relative.c_str());
        }
    }

    // Print SHOBJ lines:
    {
        vector<string> shared_objects;
        extract_shared_objects(args, shared_objects);

        for (size_t i = 0; i < shared_objects.size(); i++)
        {
            string relative;
            string full;

            if (resolve_path(shared_objects[i], relative, full) != 0)
                err(args_, "cannot resolve: %s", shared_objects[i].c_str());

            if (access(full.c_str(), F_OK) != 0)
                err(args_, "no such shared object: %s", full.c_str());

            fprintf(os, "SHOBJ=%s\n", relative.c_str());
        }
    }

    // Print CFLAG lines:
    {
        vector<string> cflags;
        extract_flags(args, cflags);

        for (size_t i = 0; i < cflags.size(); i++)
            fprintf(os, "CFLAG=%s\n", cflags[i].c_str());
    }

    // Fail if any remaining unhandled arguments:
    if (args.size())
        err(args, "unhandled arguments");

    if (exec(args_, &exit_status) != 0)
        err(args_, "exec failed");

    // Print HASH;
    if (exit_status == 0)
    {
        sha256_string hash;

        if (compute_hash(target_name.c_str(), &hash) != 0)
            err("failed to compute hash for %s", target_name.c_str());

        fprintf(os, "HASH=%s\n", hash.buf);
    }

    // Finish with a blank line.
    fprintf(os, "\n");

    if (os)
        fclose(os);

    if (fwrite(buf, 1, len, log) != len)
        err(args_, "failed to write to cdb log");

    if (log)
        fclose(log);

    free(buf);

    return exit_status;
}

static int handle_ar_subcommand(const vector<string>& args_)
{
    int exit_status;
    FILE* os = NULL;
    FILE* log = NULL;
    vector<string> args = args_;
    vector<string> objects;
    char* buf = NULL;
    size_t len = 0;

    if (!(os = open_memstream(&buf, &len)))
        err("open_memstream() failed");

    // Open the cdb log:
    if (!(log = open_cdb_spec()))
        err("failed to open the CDB spec file");

    // Print the target line:
    {
        vector<string> archives;
        extract_archives(args, archives);

        if (archives.size() == 0)
            err(args_, "missing archive argument");
        else if (archives.size() != 1)
            err(args_, "too many archives arguments");

        string relative;
        string full;

        if (resolve_path(archives[0], relative, full) != 0)
            err("failed to resolve archive path");

        fprintf(os, "%s:\n", relative.c_str());
    }

    // Print CURDIR line:
    {
        char cwd[PATH_MAX];

        if (!getcwd(cwd, sizeof(cwd)))
            err("failed to get current directory");

        fprintf(os, "CURDIR=%s\n", cwd);
    }

#if 0
    // Print the COMMAND:
    {
        fprintf(os, "COMMAND=");

        for (size_t i = 0; i < args.size(); i++)
        {
            fprintf(os, "%s", args[i].c_str());

            if (i + 1 != args.size())
                printf(" ");
        }

        fprintf(os, "\n");
    }
#endif

    // Print AR line:
    fprintf(os, "AR=%s\n", args[0].c_str());
    args.erase(args.begin());

    // Extract object files.
    extract_objects(args, objects);

    // Only the options should remain:
    if (args.size() != 1)
        err(args, "unhandled arguments");

    // Print the ARFLAGS line:
    fprintf(os, "ARFLAGS=%s\n", args[0].c_str());

    // Print the objects.
    for (size_t i = 0; i < objects.size(); i++)
    {
        string relative;
        string full;

        if (resolve_path(objects[i], relative, full) != 0)
            err(args_, "cannot resovle path: %s", objects[i].c_str());

        if (access(full.c_str(), F_OK) != 0)
            err(args_, "no such object file: %s", full.c_str());

        fprintf(os, "OBJECT=%s\n", relative.c_str());

        sha256_string hash;

        if (compute_hash(full.c_str(), &hash) != 0)
            err("failed to compute object hash: %s", full.c_str());

        fprintf(os, "OBJHASH=%s\n", hash.buf);
    }

    // Finish with a blank line.
    fprintf(os, "\n");

    if (exec(args_, &exit_status) != 0)
        err(args_, "exec() failed");

    if (os)
        fclose(os);

    if (fwrite(buf, 1, len, log) != len)
        err(args_, "failed to write to cdb log");

    if (log)
        fclose(log);

    free(buf);

    return exit_status;
}

static void dump_keywords(const char* keyword, const vector<string>& values)
{
    for (size_t i = 0; i < values.size(); i++)
        printf("%s=%s\n", keyword, values[i].c_str());
}

struct Target
{
    string name;
    string hash;
    string curdir;
    string cc;
    string ar;
    string arflags;
    vector<string> ldlibs;
    vector<string> cflags;
    vector<string> defines;
    vector<string> includes;
    vector<string> shobjs;
    vector<string> objects;
    vector<string> objhashes;
    vector<string> archives;
    vector<string> sources;
    vector<string> ldpaths;

    void clear()
    {
        name.clear();
        hash.clear();
        curdir.clear();
        cc.clear();
        ar.clear();
        arflags.clear();
        ldlibs.clear();
        cflags.clear();
        defines.clear();
        includes.clear();
        shobjs.clear();
        objects.clear();
        objhashes.clear();
        archives.clear();
        sources.clear();
        ldpaths.clear();
    }

    void dump() const
    {
        if (cc.size())
        {
            printf("%s:\n", name.c_str());

            if (curdir.size())
                printf("CURDIR=%s\n", curdir.c_str());

            if (cc.size())
                printf("CC=%s\n", cc.c_str());

            dump_keywords("INCLUDE", includes);
            dump_keywords("DEFINE", defines);
            dump_keywords("LDLIB", ldlibs);
            dump_keywords("LDPATH", ldpaths);
            dump_keywords("SOURCE", sources);
            dump_keywords("OBJECT", objects);
            dump_keywords("OBJHASH", objhashes);
            dump_keywords("ARCHIVE", archives);
            dump_keywords("SHOBJ", shobjs);
            dump_keywords("CFLAG", cflags);
            printf("HASH=%s\n", hash.c_str());

            printf("\n");
        }
        else if (ar.size())
        {
            printf("%s:\n", name.c_str());

            if (curdir.size())
                printf("CURDIR=%s\n", curdir.c_str());

            if (ar.size())
                printf("AR=%s\n", ar.c_str());

            printf("ARFLAGS=%s\n", arflags.c_str());

            dump_keywords("OBJECT", objects);
            dump_keywords("OBJHASH", objhashes);
            printf("HASH=%s\n", hash.c_str());

            printf("\n");
        }
        else
        {
            err("unknown target");
        }
    }
};

static int find_target(
    const vector<Target>& targets,
    const string& name,
    Target& target)
{
    target.clear();

    for (size_t i = 0; i < targets.size(); i++)
    {
        if (targets[i].name == name)
        {
            target = targets[i];
            return 0;
        }
    }

    return -1;
}

static int find_target_by_hash(
    const vector<Target>& targets,
    const string& hash,
    Target& target)
{
    target.clear();

    for (size_t i = 0; i < targets.size(); i++)
    {
        if (targets[i].hash == hash)
        {
            target = targets[i];
            return 0;
        }
    }

    return -1;
}

static int find_archive_objects(
    const vector<Target>& targets,
    const Target& archive,
    vector<Target>& objects)
{
    objects.clear();

    for (size_t i = 0; i < archive.objects.size(); i++)
    {
        const string& name = archive.objects[i];
        const string& hash = archive.objhashes[i];

        Target object;

        if (find_target(targets, name, object) != 0 &&
            find_target_by_hash(targets, hash, object) != 0)
        {
            err("failed to find object: %s: %s", name.c_str(), hash.c_str());
            return -1;
        }

        objects.push_back(object);
    }

    return 0;
}

static int load_cdb_spec(const string& path, vector<Target>& targets)
{
    int ret = -1;
    FILE* is = NULL;
    targets.clear();
    char buf[4096];
    unsigned int line = 0;
    Target target;
    bool inside = false;

    targets.clear();

    if (!(is = fopen(path.c_str(), "r")))
        goto done;

    while (fgets(buf, sizeof(buf), is) != NULL)
    {
        char* p = buf;
        size_t len;

        line++;

        // Remove leading whitespace:
        while (isspace(*p))
            p++;

        // Skip comments:
        if (p[0] == '#')
            continue;

        // Remove trailing whitespace:
        {
            char* end = p + strlen(p);

            while (end != p && isspace(end[-1]))
                *--end = '\0';
        }

        len = strlen(p);

        // If end of target:
        if (len == 0)
        {
            if (target.name.empty())
                err("syntax error");

            targets.push_back(target);
            target.clear();
            inside = false;
            continue;
        }

        // If a target line:
        if (!inside && p[len - 1] == ':')
        {
            target.name = string(p).substr(0, len - 1);
            inside = true;
            continue;
        }

        // Handle KEYWORD=VALUE pairs:
        {
            string keyword;
            {
                const char* start = p;

                while (isalpha(*p) || isalnum(*p) || *p == '_')
                    p++;

                if (p == start)
                {
                    fprintf(
                        stderr,
                        "%s: %s(%u): syntax error\n",
                        arg0,
                        path.c_str(),
                        line);
                    exit(1);
                }

                keyword.assign(start, (size_t)(p - start));
            }

            // Expect '=':
            {
                while (isspace(*p))
                    p++;

                if (*p++ != '=')
                {
                    fprintf(
                        stderr,
                        "%s: %s(%u): syntax error\n",
                        arg0,
                        path.c_str(),
                        line);
                    exit(1);
                }

                while (isspace(*p))
                    p++;
            }

            // Get the value:
            string value;
            {
                const char* start = p;

                while (*p)
                    p++;

                value.assign(start, (size_t)(p - start));
            }

            if (keyword == "CC")
                target.cc = value;
            else if (keyword == "AR")
                target.ar = value;
            else if (keyword == "ARFLAGS")
                target.arflags = value;
            else if (keyword == "ARCHIVE")
                target.archives.push_back(value);
            else if (keyword == "CFLAG")
                target.cflags.push_back(value);
            else if (keyword == "DEFINE")
                target.defines.push_back(value);
            else if (keyword == "INCLUDE")
                target.includes.push_back(value);
            else if (keyword == "SHOBJ")
                target.shobjs.push_back(value);
            else if (keyword == "OBJECT")
                target.objects.push_back(value);
            else if (keyword == "OBJHASH")
                target.objhashes.push_back(value);
            else if (keyword == "CURDIR")
                target.curdir = value;
            else if (keyword == "SOURCE")
                target.sources.push_back(value);
            else if (keyword == "LDLIB")
                target.ldlibs.push_back(value);
            else if (keyword == "LDPATH")
                target.ldpaths.push_back(value);
            else if (keyword == "HASH")
                target.hash = value;
            else
            {
                err("unknown keyword on line %u: %s\n", line, keyword.c_str());
            }

            continue;
        }
    }

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

static int handle_dump_subcommand(const vector<string>& args)
{
    int ret = -1;

    if (args.size() != 1)
    {
        fprintf(stderr, "Usage: %s cdb-dump CDB_SPEC\n", arg0);
        print_args(args, COLOR_RED);
        exit(1);
    }

    vector<Target> targets;

    if (load_cdb_spec(args[0], targets) != 0)
        err("failed to open CDB specification: %s\n", args[0].c_str());

    for (size_t i = 0; i < targets.size(); i++)
    {
        const Target& target = targets[i];
        target.dump();
    }

    ret = 0;

    return ret;
}

static string short_libname(const string& name)
{
    string s;

    if (name.substr(0, 3) != "lib")
        return string();

    for (size_t i = 3; i < name.size(); i++)
    {
        if (name.substr(i) == ".a")
            break;

        s += name[i];
    }

    return s;
}

static string cmake_name(const string& name)
{
    string s = name;

    for (size_t i = 3; i < name.size(); i++)
    {
        char c = s[i];

        if (i == 0)
            s[i] = (isalpha(c) || c == '_') ? c : '_';
        else
            s[i] = (isalnum(c) || c == '_') ? c : '_';
    }

    return s;
}

static int handle_cmake_subcommand(const vector<string>& args)
{
    int ret = -1;
    vector<Target> targets;
    Target archive;

    if (args.size() != 2)
    {
        fprintf(stderr, "Usage: %s cdb-dump CDB_SPEC archive-name\n", arg0);
        print_args(args, COLOR_RED);
        exit(1);
    }

    const string cdb_spec = args[0];
    const string archive_name = args[1];

    // Load the CDB specification:
    if (load_cdb_spec(cdb_spec, targets) != 0)
        err("failed to open CDB specification: %s\n", args[0].c_str());

    // If not a library:
    {
        size_t pos = archive_name.find(".a");

        if (pos == string::npos || pos != (archive_name.size() - 2))
            err("not a library: %s", archive_name.c_str());
    }

    // Find this archive target:
    if (find_target(targets, archive_name, archive) != 0)
        err("no such target: %s", archive_name.c_str());

    const string libname = short_libname(archive_name);

    vector<Target> objects;

    if (find_archive_objects(targets, archive, objects) != 0)
        err("failed to find sources for %s", archive_name.c_str());

    // add_library(name OBJECT ...)
    for (size_t i = 0; i < objects.size(); i++)
    {
        const Target& object = objects[i];

        if (object.sources.size() != 1)
            err("unexpected");

        const string name = cmake_name(object.name);
        const string source = object.sources[0];

        printf("add_library(\n");
        printf("    %s\n", name.c_str());
        printf("    OBJECT\n");
        printf("    %s)\n\n", source.c_str());
    }

    // target_include_directories()
    for (size_t i = 0; i < objects.size(); i++)
    {
        const Target& object = objects[i];
        const string name = cmake_name(object.name);

        if (object.includes.size())
        {
            printf("target_include_directories(\n");
            printf("    %s\n", name.c_str());
            printf("    PRIVATE\n");

            for (size_t j = 0; j < object.includes.size(); j++)
            {
                const string& include = object.includes[j];

                printf("    %s", include.c_str());

                if (j + 1 == object.includes.size())
                    printf(")");

                printf("\n");
            }

            printf("\n");
        }
    }

    // target_compile_definitions()
    for (size_t i = 0; i < objects.size(); i++)
    {
        const Target& object = objects[i];
        const string name = cmake_name(object.name);

        if (object.includes.size())
        {
            printf("target_compile_definitions(\n");
            printf("    %s\n", name.c_str());
            printf("    PRIVATE\n");

            for (size_t j = 0; j < object.defines.size(); j++)
            {
                const string& include = object.defines[j];

                printf("    %s", include.c_str());

                if (j + 1 == object.defines.size())
                    printf(")");

                printf("\n");
            }

            printf("\n");
        }
    }

    // add_library(libname $<TARGET_OBJECTS:?> ...)
    {
        printf("add_library(\n");
        printf("    %s\n", libname.c_str());
        printf("    STATIC\n");

        for (size_t i = 0; i < objects.size(); i++)
        {
            const Target& object = objects[i];
            const string name = cmake_name(object.name);

            printf("    $<TARGET_OBJECTS:%s>", name.c_str());

            if (i + 1 != objects.size())
                printf("\n");
            else
                printf(")");
        }

        printf("\n");
    }

    ret = 0;

    return ret;
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    vector<string> args;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s subcommand args...\n", arg0);
        exit(1);
    }

    string cmd = base_name(argv[1]);

    argv_to_args(argc - 2, argv + 2, args);

    // Check that OE_CDB_ROOT refers to a directory.
    {
        string path = get_cdb_root();

        if (!is_dir(path))
            err("Not a directory: ${OE_CDB_ROOT}=%s", path.c_str());
    }

    // Handle the subcommand:
    if (cmd == "cc")
    {
        return handle_cc_subcommand(args);
    }
    else if (cmd == "ar")
    {
        return handle_ar_subcommand(args);
    }
    else if (cmd == "dump")
    {
        return handle_dump_subcommand(args);
    }
    else if (cmd == "cmake")
    {
        return handle_cmake_subcommand(args);
    }
    else
    {
        fprintf(stderr, "%s: unknown subcommand: %s\n", arg0, cmd.c_str());
        exit(1);
    }

    return 0;
}
