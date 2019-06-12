// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>

using namespace std;

static const char* arg0;

#define COLOR_RED "\033[0;31m"
#define COLOR_VIOLET "\033[0;35m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_NONE "\033[0m"

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
    fflush(stdout);

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
    fflush(stdout);

    exit(1);
}

void get_options(const string& keyword, vector<string>& values)
{
    for (size_t i = 0; i < _options.size(); i++)
    {
        if (_options[i].keyword == keyword)
            values.push_back(_options[i].value);
    }
}

int get_option(const string& keyword, string& value)
{
    value.clear();

    for (size_t i = 0; i < _options.size(); i++)
    {
        if (_options[i].keyword == keyword)
            value = _options[i].value;
    }

    return value.empty() ? -1 : 0;
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

void print_args(const vector<string>& args, const char* color)
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

        print_args(sargs, COLOR_CYAN);

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

        print_args(sargs, COLOR_CYAN);

        if (exec(sargs, &exit_status) != 0)
        {
            fprintf(stderr, "%s: shadow exec() failed\n", arg0);
            exit(1);
        }

        if (exit_status != 0)
        {
            fprintf(stderr, "%s", COLOR_RED);
            fprintf(stderr, "%s: error: shadow link failed\n", args[0].c_str());
            fprintf(stderr, "%s\n", COLOR_NONE);
        }
    }

    // print_args(args, COLOR_GREEN);

    // Execute the default command:
    if (exec(args, &exit_status) != 0)
    {
        fprintf(stderr, "%s: exec() failed\n", arg0);
        exit(1);
    }

    return exit_status;
}

bool has_sources(const vector<string>& args)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        const string& tmp = args[i];

        size_t pos = tmp.find(".c");

        if (pos != string::npos && pos + 2 == tmp.size())
            return true;

        pos = tmp.find(".cpp");

        if (pos != string::npos && pos + 4 == tmp.size())
            return true;

        pos = tmp.find(".cc");

        if (pos != string::npos && pos + 3 == tmp.size())
            return true;
    }

    return false;
}

int get_target(const vector<string>& args, string& path)
{
    path.clear();

    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];

        if (arg.substr(0, 2) == "-o")
        {
            if (arg == "-o")
            {
                if (i + 1 != args.size())
                {
                    path = args[i + 1];
                    return 0;
                }
            }
            else
            {
                path = arg.substr(2);
                return 0;
            }
        }
    }

    return -1;
}

int change_output_file_name(vector<string>& args, const string& path)
{
    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];

        if (arg.substr(0, 2) == "-o")
        {
            if (arg == "-o")
            {
                if (i + 1 != args.size())
                {
                    args[i + 1] = path;
                    return 0;
                }
            }
            else
            {
                args[i] = "-o" + path;
                return 0;
            }
        }
    }

    return -1;
}

string basename(const string& path)
{
    size_t pos = path.rfind('/');
    return (pos == string::npos) ? path : path.substr(pos + 1);
}

string dirname(const string& path)
{
    size_t pos = path.rfind('/');
    return (pos == string::npos) ? "." : path.substr(pos);
}

int copy_file(const string& old_path, const string& new_path)
{
    int ret = -1;
    FILE* is = NULL;
    FILE* os = NULL;
    int c;

    if (!(is = fopen(old_path.c_str(), "r")))
        goto done;

    if (!(os = fopen(new_path.c_str(), "w")))
        goto done;

    while ((c = fgetc(is)) != EOF)
    {
        if (fputc(c, os) == EOF)
            goto done;
    }

    if (ferror(is))
        goto done;

    if (!feof(is))
        goto done;

    ret = 0;

done:

    if (is)
        fclose(is);

    if (os)
        fclose(os);

    return ret;
}

int handle_cc(const vector<string>& args_, const string& compiler)
{
    int exit_status = 1;

    string cc_cflag = compiler + "_cflag";
    string cc_include = compiler + "_include";
    string cc_lib = compiler + "_lib";
    string cc_ldflag = compiler + "_ldflag";
    vector<string> sargs;
    string old_out_file;
    string new_out_file;
    bool found = false;
    vector<string> args = args_;

    delete_args(args);

    // Append the program name:
    sargs.push_back(args[0]);

    delete_args(sargs);

    // If there are any source files:
    if (has_sources(args))
    {
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

        found = true;
    }

    // Append the ldflags:
    if (contains(args, "-o") && !contains(args, "-c") &&
        !contains(args, "-shared"))
    {
        // Append the ldflags:
        {
            vector<string> ldflags;
            get_options(cc_ldflag, ldflags);

            sargs.insert(sargs.end(), ldflags.begin(), ldflags.end());
        }
    }

    // Append caller arguments:
    sargs.insert(sargs.end(), args.begin() + 1, args.end());

    // Append the libs
    if (contains(args, "-o") && !contains(args, "-c") &&
        !contains(args, "-shared"))
    {
        string path;

        if (get_target(args, path) != 0)
        {
            fprintf(stderr, "%s: output file not found\n", arg0);
            exit(1);
        }

        old_out_file = path;
        new_out_file = dirname(path) + "/" + basename(path) + ".enc";

        remove(old_out_file.c_str());
        remove(new_out_file.c_str());

        if (change_output_file_name(sargs, new_out_file) != 0)
        {
            fprintf(stderr, "%s: failed to change output file name\n", arg0);
            exit(1);
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

        found = true;
    }

    if (!found)
    {
        fprintf(stderr, "%s: invalid input\n", arg0);
        exit(1);
    }

    print_args(sargs, COLOR_CYAN);

    if (exec(sargs, &exit_status) != 0)
    {
        fprintf(stderr, "%s: shadow exec() failed\n", arg0);
        exit(1);
    }

    if (exit_status != 0)
    {
        fprintf(stderr, "%s", COLOR_RED);
        fprintf(stderr, "%s: error\n", args[0].c_str());
        fprintf(stderr, "%s\n", COLOR_NONE);
        exit(exit_status);
    }

    if (access(new_out_file.c_str(), X_OK) == 0)
    {
        string oerun;

        if (which("oerun", oerun) != 0)
        {
            fprintf(stderr, "%s: no on path: oerun\n", arg0);
            exit(1);
        }

        if (copy_file(oerun, old_out_file) != 0)
        {
            fprintf(
                stderr,
                "%s: failed to create: %s\n",
                arg0,
                old_out_file.c_str());
            exit(1);
        }

        mode_t mode = 0;
        mode |= (S_IRUSR | S_IWUSR | S_IXUSR);
        mode |= (S_IRGRP | S_IXGRP);
        mode |= (S_IROTH | S_IXOTH);

        if (chmod(old_out_file.c_str(), mode) != 0)
        {
            fprintf(
                stderr, "%s: chmod failed: %s\n", arg0, old_out_file.c_str());
            exit(1);
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

void make_substitutions(vector<option>& options, option& opt)
{
    (void)options;
    (void)opt;
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

        option opt(keyword, value);
        make_substitutions(options, opt);

        options.push_back(opt);
    }

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

void extract_opt_args(
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

void extract_flags(vector<string>& args, vector<string>& flags)
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

void extract_files(
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

void extract_shared_objects(vector<string>& args, vector<string>& sources)
{
    vector<string> result_args;

    sources.clear();

    for (size_t i = 0; i < args.size(); i++)
    {
        const string& arg = args[i];

        size_t pos = arg.find(".so");

        if (pos != string::npos && pos == (arg.size() - strlen(".so")))
        {
            sources.push_back(arg);
            continue;
        }

        if (arg.find(".so.") != string::npos)
        {
            sources.push_back(arg);
            continue;
        }

        result_args.push_back(arg);
    }

    args = result_args;
}

void extract_sources(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, source_exts, sources);
}

void extract_objects(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, object_exts, sources);
}

void extract_archives(vector<string>& args, vector<string>& sources)
{
    return extract_files(args, archive_exts, sources);
}

static FILE* open_cdb_log()
{
    vector<string> options;

    get_options("cdb_log", options);

    if (options.size() != 1)
        return NULL;

    return fopen(options[0].c_str(), "a");
}

int resolve_path(string& path)
{
    int ret = -1;
    string root;
    char cwd[PATH_MAX];
    string tmp_path;

    if (get_option("cdb_root", root) != 0)
        goto done;

    if (!getcwd(cwd, sizeof(cwd)))
        goto done;

    if (path[0] == '/')
    {
        tmp_path = path;
    }
    else
    {
        tmp_path = string(cwd) + "/" + path;

        if (tmp_path.substr(0, root.size()) != root)
            goto done;

        if (tmp_path[root.size()] != '/')
            goto done;

        tmp_path = tmp_path.substr(root.size() + 1);
    }

    path = tmp_path;

    ret = 0;

done:
    return ret;
}

bool is_dir(const string& path)
{
    struct stat buf;

    if (stat(path.c_str(), &buf) != 0)
        return false;

    return S_ISDIR(buf.st_mode);
}

int handle_cdb_cc(const vector<string>& args_)
{
    int exit_status;
    FILE* os = NULL;
    vector<string> args = args_;

    // Open the cdb log:
    if (!(os = open_cdb_log()))
        err("failed to open the log file");

    // Print the target:
    {
        vector<string> targets;

        extract_opt_args(args, "-o", targets);

        if (targets.size() != 1)
            err(args_, "cannot resolve target");

        fprintf(os, "%s:\n", targets[0].c_str());
    }

    // Print CURDIR line:
    {
        char cwd[PATH_MAX];

        if (!getcwd(cwd, sizeof(cwd)))
            err("failed to get current directory");

        fprintf(os, "CURDIR=%s\n", cwd);
    }

    // Print CC line:
    fprintf(os, "CC=%s\n", args[0].c_str());
    args.erase(args.begin());

    // Print INCLUDE lines:
    {
        vector<string> includes;
        extract_opt_args(args, "-I", includes);

        for (size_t i = 0; i < includes.size(); i++)
        {
            string path = includes[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (!is_dir(path))
                err(args_, "no such directory: %s", path.c_str());

            fprintf(os, "INCLUDE=%s\n", path.c_str());
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
            string path = libs[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (!is_dir(path))
                err(args_, "no such directory: %s", path.c_str());

            fprintf(os, "LDPATH=%s\n", path.c_str());
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
            string path = sources[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (access(path.c_str(), F_OK) != 0)
                err(args_, "no such source file: %s", path.c_str());

            fprintf(os, "SOURCE=%s\n", path.c_str());
        }
    }

    // Print OBJECT lines:
    {
        vector<string> objects;
        extract_objects(args, objects);

        for (size_t i = 0; i < objects.size(); i++)
        {
            string path = objects[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (access(path.c_str(), F_OK) != 0)
                err(args_, "no such object file: %s", path.c_str());

            fprintf(os, "OBJECT=%s\n", path.c_str());
        }
    }

    // Print ARCHIVE lines:
    {
        vector<string> archives;
        extract_archives(args, archives);

        for (size_t i = 0; i < archives.size(); i++)
        {
            string path = archives[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (access(path.c_str(), F_OK) != 0)
                err(args_, "no such archive: %s", path.c_str());

            fprintf(os, "ARCHIVE=%s\n", path.c_str());
        }
    }

    // Print SHOBJ lines:
    {
        vector<string> shared_objects;
        extract_shared_objects(args, shared_objects);

        for (size_t i = 0; i < shared_objects.size(); i++)
        {
            string path = shared_objects[i];

            if (resolve_path(path) != 0)
                err(args_, "cannot resovle path: %s", path.c_str());

            if (access(path.c_str(), F_OK) != 0)
                err(args_, "no such shared object: %s", path.c_str());

            fprintf(os, "SHOBJ=%s\n", path.c_str());
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

    // Finish with a blank line.
    fprintf(os, "\n");

    if (exec(args_, &exit_status) != 0)
        err(args_, "exec failed");

    if (os)
        fclose(os);

    return exit_status;
}

int handle_cdb_ar(const vector<string>& args_)
{
    int exit_status;
    FILE* os = NULL;
    vector<string> args = args_;
    vector<string> objects;

    // Open the cdb log:
    if (!(os = open_cdb_log()))
        err("failed to open the log file");

    // Print the target line:
    {
        vector<string> archives;
        extract_archives(args, archives);

        if (archives.size() == 0)
            err(args_, "missing archive argument");
        else if (archives.size() != 1)
            err(args_, "too many archives arguments");

        fprintf(os, "%s:\n", archives[0].c_str());
    }

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
        string path = objects[i];

        if (resolve_path(path) != 0)
            err(args_, "cannot resovle path: %s", path.c_str());

        if (access(path.c_str(), F_OK) != 0)
            err(args_, "no such object file: %s", path.c_str());

        fprintf(os, "OBJECT=%s\n", path.c_str());
    }

    // Finish with a blank line.
    fprintf(os, "\n");

    if (exec(args_, &exit_status) != 0)
        err(args_, "exec() failed");

    if (os)
        fclose(os);

    return exit_status;
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

    if (cmd == "gcc")
    {
        return handle_cc(args, "gcc");
    }
    else if (cmd == "shadow-gcc")
    {
        return handle_shadow_cc(args, "gcc");
    }
    else if (cmd == "clang")
    {
        return handle_cc(args, "clang");
    }
    else if (cmd == "shadow-clang")
    {
        return handle_shadow_cc(args, "clang");
    }
    else if (cmd == "shadow-ar")
    {
        return handle_shadow_ar(args);
    }
    else if (cmd == "cdb-cc")
    {
        return handle_cdb_cc(args);
    }
    else if (cmd == "cdb-ar")
    {
        return handle_cdb_ar(args);
    }
    else
    {
        fprintf(stderr, "%s: unsupported command: %s\n", arg0, cmd.c_str());
        exit(1);
    }

    return 0;
}
