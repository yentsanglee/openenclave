#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <stdio.h>

int load_file(const char* path, void** data_out, size_t* size_out);

int main(int argc, const char* argv[])
{
    int ret = 1;
    BIO* bio = NULL;
    X509* x509 = NULL;
    void* pem_data = NULL;
    size_t pem_size;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s pem_certificate\n", argv[0]);
        goto done;
    }

    if (load_file(argv[1], &pem_data, &pem_size) != 0)
    {
        fprintf(stderr, "%s: failed to load %s\n", argv[0], argv[1]);
        goto done;
    }

    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    if (!(bio = BIO_new_mem_buf(pem_data, pem_size)))
        goto done;

    if (!(x509 = PEM_read_bio_X509(bio, NULL, 0, NULL)))
        goto done;

    printf("SUCCESS!\n");

    ret = 0;

done:

    if (bio)
        BIO_free(bio);

    if (x509)
        X509_free(x509);

    if (pem_data)
        free(pem_data);

    return ret;
}
