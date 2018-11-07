#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include "sgx/sgx_tprotected_fs.h"
#include "secure_file_t.h"

void enc_test()
{
    printf("About to open a file\n");
    SGX_FILE* file = sgx_fopen_auto_key("/home/azureuser/temp/foo", "w");
    if (file != NULL)
    {
        printf("Successfully opened for write\n");

        char writedata[] = "hello world!";
        int bytes_written  = sgx_fwrite(writedata, 1, sizeof(writedata), file);
        if (bytes_written == sizeof(writedata))
        {
            printf("Successfully wrote data\n");
        }
        else
        {
            printf("Failed to write data. sgx_ferror=%d\n", sgx_ferror(file));
        }

        sgx_fclose(file);
        printf("Closed file\n");

        file = sgx_fopen_auto_key("/home/azureuser/temp/foo", "r");
        if (file != NULL)
        {
            printf("Successfully opened for read\n");

            char readdata[1000];
            int bytes_read = sgx_fread(readdata, 1, sizeof(readdata), file);
            if (bytes_read == bytes_written)
            {
                printf("Successfully read data\n");
                if (strcmp(writedata, readdata) == 0)
                {
                    printf("Read and write data were the same\n");
                }
                else
                {
                    printf("Read and write data were different\n");
                }
            }
            else
            {
                printf("Failed to read data. sgx_ferror=%d\n", sgx_ferror(file));
            }

            sgx_fclose(file);
            printf("Closed file\n");
        }
        else
        {
            printf("Failed to open file for read\n");
        }
    }
    else
    {
        printf("Failed to open file. errno=%d, \"%s\"\n", errno, strerror(errno));
    }
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
