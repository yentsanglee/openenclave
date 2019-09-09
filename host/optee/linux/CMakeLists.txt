# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## Append platform paths
##
##==============================================================================

list(APPEND PLATFORM_SDK_ONLY_SRC
  optee/linux/enclave.c)

##==============================================================================
##
## Append directory paths
##
##==============================================================================

list(APPEND OEHOST_PRIVATE_DIRECTORIES
  ${OE_TZ_OPTEE_CLIENT_INC})

##==============================================================================
##
## Overwrite parent scope variables
##
##==============================================================================

set(PLATFORM_SDK_ONLY_SRC ${PLATFORM_SDK_ONLY_SRC} PARENT_SCOPE)
set(OEHOST_PRIVATE_DIRECTORIES ${OEHOST_PRIVATE_DIRECTORIES})