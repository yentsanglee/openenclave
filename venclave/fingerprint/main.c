// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctype.h>
#include <openenclave/bits/defs.h>
#include <openenclave/internal/fingerprint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* arg0;

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    int ret = 1;
    oe_fingerprint_t fingerprint;
    FILE* os = NULL;
    const char* infile;
    const char* outfile;
    const char* varname;

    /* Require three parameters. */
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s infile outfile varname\n", argv[0]);
        goto done;
    }

    /* Collect arguments. */
    infile = argv[1];
    outfile = argv[2];
    varname = argv[3];

    /* Validate the variable name. */
    {
        if (!isalpha(varname[0]) && varname[0] != '_')
            err("bad variable name: %s", varname);

        for (size_t i = 1, n = strlen(varname); i < n; i++)
        {
            if (!isalnum(varname[i]) && varname[i] != '_')
                err("bad variable name: %s", varname);
        }
    }

    /* Compute the finger print of the input file. */
    if (oe_compute_fingerprint(infile, &fingerprint) != 0)
        err("failed to compute fingerprint for %s", infile);

    /* Open the output file */
    if (!(os = fopen(outfile, "w")))
        err("failed to open %s", infile);

    /* Write the fingerprint into the output source file. */
    {
        fprintf(os, "#include <openenclave/internal/fingerprint.h>\n");
        fprintf(os, "\n");
        fprintf(os, "const oe_fingerprint_t %s =\n", varname);

        fprintf(os, "{\n");
        fprintf(os, "    %zu,\n", fingerprint.size);
        fprintf(os, "    {\n");

        for (size_t i = 0; i < sizeof(fingerprint.hash); i++)
        {
            fprintf(os, "        0x%02x,\n", fingerprint.hash[i]);
        }

        fprintf(os, "    }\n");
        fprintf(os, "};\n");
    }

    ret = 0;

done:

    if (os)
        fclose(os);

    return ret;
}
