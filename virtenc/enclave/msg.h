#ifndef _VE_ENCLAVE_MSG_H
#define _VE_ENCLAVE_MSG_H

#include "../common/msg.h"

int ve_msg_print(const char* data);

int ve_handle_init(void);

int ve_handle_messages(void);

#endif /* _VE_ENCLAVE_MSG_H */
