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

struct option
{
    option(const string& keyword_, const string& value_)
        : keyword(keyword_), value(value_)
    {
    }

    string keyword;
    string value;
};

static vector<option> _options;

void get_options(const string& keyword, vector<string>& values)
{
    for (size_t i = 0; i < _options.size(); i++)
    {
        if (_options[i].keyword == keyword)
            values.push_back(_options[i].value);
    }
}

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

bool has_arg(const vector<string>& args, const string& arg)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == arg)
            return true;
    }

    return false;
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

    // Execute the shadow command:
    if (has_arg(args, "-c"))
    {
        vector<string> sargs = args;

        // Prepend the CFLAGS:
        {
            vector<string> cflags;
            get_options("gcc_cflag", cflags);

            sargs.insert(sargs.begin() + 1, cflags.begin(), cflags.end());
        }

        // Replace ".o" suffixes with ".enc.o":
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            size_t pos = tmp.find(".o");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.o";
        }

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }
    }

    // Execute the default command:
    if (exec(args, &exit_status) != 0)
    {
        fprintf(stderr, "%s: exec() failed\n", arg0);
        exit(1);
    }

    return exit_status;
}

int handle_ar(const vector<string>& args)
{
    int exit_status;

    // Execute the shadow command first:
    {
        vector<string> sargs = args;

        // Replace ".o" suffixes with ".enc.o":
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            size_t pos = tmp.find(".o");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.o";

            pos = tmp.find(".a");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.a";
        }

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }
    }

    // Execute the default command:
    if (exec(args, &exit_status) != 0)
    {
        fprintf(stderr, "%s: exec() failed\n", arg0);
        exit(1);
    }

    return exit_status;
}

int load_config(vector<option>& options)
{
    int ret = -1;
    const char* home;
    FILE* is = NULL;
    string path;
    char buf[1024];
    unsigned int line = 0;

    options.clear();

    if (!(home = getenv("HOME")))
        goto done;

    path = string(home) + "/.oewraprc";

    if (!(is = fopen(path.c_str(), "r")))
        goto done;

    while (fgets(buf, sizeof(buf), is) != NULL)
    {
        char* p = buf;

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

        // Skip blank lines:
        if (!*p)
            continue;

        // Get the keyword:
        string keyword;
        {
            const char* start = p;

            while (isalpha(*p) || *p == '_')
                p++;

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

        options.push_back(option(keyword, value));
    }

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    vector<string> args;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s command command-args...\n", arg0);
        exit(1);
    }

    // Load the configuration file:
    if (load_config(_options) != 0)
    {
        fprintf(stderr, "%s: cannot open ${HOME}/.oewraprc\n", arg0);
        exit(1);
    }

    argv_to_args(argc - 2, argv + 2, args);
    string cmd = base_name(args[0].c_str());

    if (cmd == "gcc")
    {
        return handle_gcc(args);
    }
    else if (cmd == "ar")
    {
        return handle_ar(args);
    }
    else
    {
        fprintf(stderr, "%s: unsupported command: %s\n", arg0, cmd.c_str());
        exit(1);
    }

    return 0;
}
