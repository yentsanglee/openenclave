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
    printf("Command:\n");

    for (size_t i = 0; i < args.size(); i++)
        printf("    %s\n", args[i].c_str());

    printf("\n");
}

void print_args(const vector<string>& args)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        printf("%s", args[i].c_str());

        if (i + 1 != args.size())
            printf(" ");
    }
    printf("\n");
}

bool contains(const vector<string>& args, const string& arg)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == arg)
            return true;
    }

    return false;
}

void delete_arg(vector<string>& args, const string& arg)
{
    vector<string> new_args;

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] != arg)
        {
            new_args.push_back(args[i]);
        }
    }

    args = new_args;
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

int serialize_argv(
    int argc,
    const char* argv[],
    void** argv_buf,
    size_t* argv_buf_size)
{
    int ret = -1;
    char** new_argv = NULL;
    size_t array_size;
    size_t alloc_size;

    if (argv_buf)
        *argv_buf = NULL;

    if (argv_buf_size)
        *argv_buf_size = 0;

    if (argc < 0 || !argv || !argv_buf || !argv_buf_size)
        goto done;

    /* Calculate the total allocation size. */
    {
        /* Calculate the size of the null-terminated array of pointers. */
        array_size = ((size_t)argc + 1) * sizeof(char*);

        alloc_size = array_size;

        /* Calculate the sizeo of the individual zero-terminated strings. */
        for (int i = 0; i < argc; i++)
            alloc_size += strlen(argv[i]) + 1;
    }

    /* Allocate space for the array followed by strings. */
    if (!(new_argv = (char**)malloc(alloc_size)))
        goto done;

    /* Copy each string. */
    {
        char* p = (char*)&new_argv[argc + 1];
        size_t offset = array_size;

        for (int i = 0; i < argc; i++)
        {
            size_t size = strlen(argv[i]) + 1;

            memcpy(p, argv[i], size);
            new_argv[i] = (char*)offset;
            p += size;
            offset += size;
        }

        new_argv[argc] = NULL;
    }

    *argv_buf = new_argv;
    *argv_buf_size = alloc_size;
    new_argv = NULL;

    ret = 0;

done:

    if (new_argv)
        free(new_argv);

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

void delete_args(vector<string>& args)
{
    vector<string> delete_args;
    vector<string> new_args;

    get_options("delete_arg", delete_args);

    for (size_t i = 0; i < args.size(); i++)
    {
        if (!contains(delete_args, args[i]))
            new_args.push_back(args[i]);
    }

    args = new_args;
}

int handle_shadow_cc(const vector<string>& args, const string& compiler)
{
    int exit_status;

    string cc_cflag = compiler + "_cflag";
    string cc_include = compiler + "_include";
    string cc_lib = compiler + "_lib";
    string cc_ldflag = compiler + "_ldflag";

    // Execute the shadow command:
    if (contains(args, "-c"))
    {
        vector<string> sargs;

        // Append the program name:
        sargs.push_back(args[0]);

        // Append the cflags:
        {
            vector<string> cflags;
            get_options(cc_cflag, cflags);

            sargs.insert(sargs.end(), cflags.begin(), cflags.end());
        }

        // Append the extra defines:
        {
            vector<string> defines;
            get_options("extra_define", defines);

            sargs.insert(sargs.end(), defines.begin(), defines.end());
        }

        // Append the extra includes:
        {
            vector<string> includes;
            get_options("extra_include", includes);

            sargs.insert(sargs.end(), includes.begin(), includes.end());
        }

        // Append the includes:
        {
            vector<string> includes;
            get_options(cc_include, includes);

            sargs.insert(sargs.end(), includes.begin(), includes.end());
        }

        // Append remaining arguments:
        sargs.insert(sargs.end(), args.begin() + 1, args.end());

        // Replace ".o" suffixes with ".enc.o":
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            size_t pos = tmp.find(".o");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.o";
        }

        delete_args(sargs);

#if 0
        printf("\n[SHADOW]: ");
        print_args(sargs);
        printf("\n");
#endif

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }

        if (exit_status != 0)
        {
            dump_args(sargs);
            exit(exit_status);
        }
    }
    else if (contains(args, "-o") && !contains(args, "-shared"))
    {
        vector<string> sargs = args;

        delete_args(sargs);

        // Replace ".o" suffixes with ".enc.o":
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            size_t pos = tmp.find(".o");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.o";
        }

        // Replace ".a" suffixes with ".enc.a":
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            size_t pos = tmp.find(".a");

            if (pos != string::npos && pos + 2 == tmp.size())
                sargs[i] = tmp.substr(0, pos) + ".enc.a";
        }

        // Rename the executable.
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            if (tmp == "-o" && (i + 1) != sargs.size())
            {
                sargs[i + 1] = sargs[i + 1] + ".enc";
                break;
            }
        }

        // Translate -lxyz to -lxyz.enc:
        for (size_t i = 0; i < sargs.size(); i++)
        {
            string tmp = sargs[i];

            if (tmp[0] == '-' && tmp[1] == 'l')
                sargs[i] = sargs[i] + ".enc";
        }

        // Append the ldflags:
        {
            vector<string> ldflags;
            get_options(cc_ldflag, ldflags);

            sargs.insert(sargs.end(), ldflags.begin(), ldflags.end());
        }

        // Append liboewrapenclave:
        {
            vector<string> libs;

            libs.push_back("-Wl,--whole-archive");
            libs.push_back("-loewrapenclave");
            libs.push_back("-Wl,--no-whole-archive");

            libs.push_back("-loehostfs");
            libs.push_back("-loehostsock");
            libs.push_back("-loehostresolver");

            sargs.insert(sargs.end(), libs.begin(), libs.end());
        }

        // Append the libs:
        {
            vector<string> libs;
            get_options(cc_lib, libs);

            sargs.insert(sargs.end(), libs.begin(), libs.end());
        }

#if 0
        printf("\n[SHADOW]: ");
        printf("[SHADOW]: ");
        print_args(sargs);
        printf("\n");
#endif

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }

        if (exit_status != 0)
        {
            fprintf(stderr, "SHADOW_LINK_ERROR_IGNORED\n");
            dump_args(sargs);
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

int handle_cc(const vector<string>& args, const string& compiler)
{
    int exit_status = 1;

    string cc_cflag = compiler + "_cflag";
    string cc_include = compiler + "_include";
    string cc_lib = compiler + "_lib";
    string cc_ldflag = compiler + "_ldflag";

    // Execute the shadow command:
    if (contains(args, "-c"))
    {
        vector<string> sargs;

        // Append the program name:
        sargs.push_back(args[0]);

        // Append the cflags:
        {
            vector<string> cflags;
            get_options(cc_cflag, cflags);

            sargs.insert(sargs.end(), cflags.begin(), cflags.end());
        }

        // Append the extra defines:
        {
            vector<string> defines;
            get_options("extra_define", defines);

            sargs.insert(sargs.end(), defines.begin(), defines.end());
        }

        // Append the includes:
        {
            vector<string> includes;
            get_options(cc_include, includes);

            sargs.insert(sargs.end(), includes.begin(), includes.end());
        }

        // Append remaining arguments:
        sargs.insert(sargs.end(), args.begin() + 1, args.end());

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }

        if (exit_status != 0)
        {
            // dump_args(sargs);
            exit(exit_status);
        }
    }
    else if (contains(args, "-o") && !contains(args, "-shared"))
    {
        vector<string> sargs = args;

        delete_args(sargs);

        // Append the ldflags:
        {
            vector<string> ldflags;
            get_options(cc_ldflag, ldflags);

            sargs.insert(sargs.end(), ldflags.begin(), ldflags.end());
        }

        // Append liboewrapenclave:
        {
            vector<string> libs;

            libs.push_back("-Wl,--whole-archive");
            libs.push_back("-loewrapenclave");
            libs.push_back("-Wl,--no-whole-archive");

            libs.push_back("-loehostfs");
            libs.push_back("-loehostsock");
            libs.push_back("-loehostresolver");

            sargs.insert(sargs.end(), libs.begin(), libs.end());
        }

        // Append the libs:
        {
            vector<string> libs;
            get_options(cc_lib, libs);

            sargs.insert(sargs.end(), libs.begin(), libs.end());
        }

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }

        if (exit_status != 0)
        {
            // dump_args(sargs);
            exit(exit_status);
        }
    }

    return exit_status;
}

int handle_shadow_ar(const vector<string>& args)
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
            dump_args(sargs);
            exit(1);
        }

        if (exit_status != 0)
            exit(exit_status);
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

    string cmd = base_name(argv[1]);

    argv_to_args(argc - 2, argv + 2, args);

    if (cmd == "shadow-gcc")
    {
        return handle_shadow_cc(args, "gcc");
    }
    else if (cmd == "gcc")
    {
        return handle_cc(args, "gcc");
    }
    else if (cmd == "shadow-ar")
    {
        return handle_shadow_ar(args);
    }
    else
    {
        fprintf(stderr, "%s: unsupported command: %s\n", arg0, cmd.c_str());
        exit(1);
    }

    return 0;
}
