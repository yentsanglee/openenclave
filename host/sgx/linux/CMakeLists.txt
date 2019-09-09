# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## Append OS paths
##
##==============================================================================

list(APPEND PLATFORM_HOST_ONLY_SRC
  sgx/linux/sgxquoteproviderloader.c)

list(APPEND PLATFORM_SDK_ONLY_SRC
  sgx/linux/aep.S
  sgx/linux/aesm.c
  sgx/linux/enter.S
  sgx/linux/entersim.S
  sgx/linux/exception.c
  sgx/linux/sgxioctl.c
  sgx/linux/xstate.c)

##==============================================================================
##
## Append libraries
##
##==============================================================================

list(APPEND OEHOST_PRIVATE_LIBRARIES
  oedebugrt)

##==============================================================================
##
## Overwrite parent scope variables
##
##==============================================================================

set(PLATFORM_HOST_ONLY_SRC ${PLATFORM_HOST_ONLY_SRC} PARENT_SCOPE)
set(PLATFORM_SDK_ONLY_SRC ${PLATFORM_SDK_ONLY_SRC} PARENT_SCOPE)
set(OEHOST_PRIVATE_LIBRARIES ${OEHOST_PRIVATE_LIBRARIES} PARENT_SCOPE)
