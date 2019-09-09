# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## Append platform paths
##
##==============================================================================

list(APPEND PLATFORM_SDK_ONLY_SRC
  optee/log.c)

##==============================================================================
##
## Append specific OS paths based on system architecture
##
##==============================================================================

if (UNIX)
  add_subdirectory(linux)
elseif(WIN32)
  add_subdirectory(windows)
else()
  message(FATAL_ERROR "OP-TEE is not yet supported on platforms other than Linux.")
endif()

list(APPEND PLATFORM_FLAGS "")

##==============================================================================
##
## Overwrite parent scope variables
##
##==============================================================================

set(PLATFORM_SDK_ONLY_SRC ${PLATFORM_SDK_ONLY_SRC} PARENT_SCOPE)
set(PLATFORM_FLAGS ${PLATFORM_FLAGS} PARENT_SCOPE)
set(OEHOST_PRIVATE_DIRECTORIES ${OEHOST_PRIVATE_DIRECTORIES})
