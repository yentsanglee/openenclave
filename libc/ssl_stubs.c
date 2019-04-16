#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <openenclave/internal/random.h>
#include <openssl/rand.h>




struct hostent *gethostbyname(const char *name)
{
    return NULL;
}


static int sgx_read_rand(uint8_t *buf, size_t size)
{
        uint32_t i;
        for(i=0;i<(uint32_t)size;++i){
            buf[i]=(uint8_t)rand();
        }
    return 0;
}

static int sgxssl_read_rand(unsigned char *rand_buf, int length_in_bytes) {

  int ret;

  if (rand_buf == NULL || length_in_bytes <= 0) {
    return 1;
  }

  ret = sgx_read_rand(rand_buf, length_in_bytes);
  if (ret != 0) {
    return 1;
  }

  return 0;
}

int sgx_rand_status(void) { return 1; }

int get_sgx_rand_bytes(unsigned char *buf, int num) {
  if (sgxssl_read_rand(buf, num) == 0) {
    return 1;
  } else {
    return 0;
  }
}

char *oe_gai_strerror(int err)
{
  char *str = strerror(err);

  return str;
}

int oe_getcontext(void *ucp)
{
  return -1;
}

int oe_setcontext(const void *ucp)
{
  return -1;
}

void oe_makecontext(void *ucp, void (*func)(), int argc, ...)
{
  return;
}


int oe_mprotect(void *addr, size_t len, int prot) {
  errno = EACCES;
  return -1;
}

int oe_madvise(void *addr, size_t len, int advice) {
  return 0;
}

int oe_mlock(const void *__addr, size_t __len) {
  return 0;
}


int oe_pthread_atfork(void (*prepare)(void), void (*parent)(void),
       void (*child)(void))
{
  return EPERM;
}

