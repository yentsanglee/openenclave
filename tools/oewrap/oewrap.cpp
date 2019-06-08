// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>

using namespace std;

static const char* arg0;

int split(const char* str, const char* delim, vector<string>& v)
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

void dump_args(const vector<string>& args)
{
    printf("=== dump_args()\n");

    for (size_t i = 0; i < args.size(); i++)
        printf("args[%zu]=%s\n", i, args[i].c_str());

    printf("\n");
}

void argv_to_args(const int argc, const char* argv[], vector<string>& args)
{
    args.clear();

    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);
}

char** args_to_argv(const vector<string>& args)
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

int which(const char* filename, string& full_path)
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

int exec(const vector<string>& args, int* exit_status)
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

string base_name(const char* path)
{
    char* slash = strrchr(path, '/');

    if (slash)
        return string(slash + 1);
    else
        return string(path);
}

int handle_gcc(const vector<string>& args)
{
    int exit_status;

    if (exec(args, &exit_status) != 0)
    {
        fprintf(stderr, "%s: exec() failed\n", arg0);
        exit(1);
    }

    return exit_status;
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    vector<string> args;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s command-name args...\n", arg0);
        exit(1);
    }

    argv_to_args(argc - 1, argv + 1, args);
    dump_args(args);

    string cmd = base_name(args[0].c_str());

    if (cmd == "gcc")
    {
        return handle_gcc(args);
    }
    else
    {
        fprintf(stderr, "%s: unsupported command: %s\n", arg0, cmd.c_str());
        exit(1);
    }

    return 0;
}
