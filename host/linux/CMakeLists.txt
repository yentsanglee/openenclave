# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## Append general OS paths
##
##==============================================================================

list(APPEND PLATFORM_HOST_ONLY_SRC
  ../common/cert.c
  crypto/openssl/asn1.c
  crypto/openssl/cert.c
  crypto/openssl/crl.c
  crypto/openssl/ec.c
  crypto/openssl/init.c
  crypto/openssl/key.c
  crypto/openssl/rsa.c
  crypto/openssl/sha.c
  linux/hostthread.c)

list(APPEND PLATFORM_SDK_ONLY_SRC
  ../common/asn1.c
  crypto/openssl/hmac.c
  crypto/openssl/random.c
  linux/syscall.c
  linux/time.c
  linux/windows.c)

##==============================================================================
##
## Append libraries
##
##==============================================================================

list(APPEND OEHOST_PRIVATE_LIBRARIES
  crypto
  dl
  Threads::Threads)

if (OE_TRUSTZONE)
  list(APPEND OEHOST_PRIVATE_LIBRARIES
    ${OE_TZ_OPTEE_CLIENT_INC}
    teec)
  list(APPEND OEHOST_PRIVATE_DIRECTORIES
      ${OE_TZ_OPTEE_CLIENT_INC})
endif ()

list(APPEND OEHOST_VERIFY_PRIVATE_LIBRARIES
  crypto
  dl
  Threads::Threads)

list(APPEND OEHOST_VERIFY_INTERFACE_LIBRARIES
    -rdynamic
    -Wl,-z,noexecstack)

list(APPEND OEHOSTAPP_INTERFACE_LIBRARIES
    -rdynamic
    -Wl
    -z
    noexecstack)

##==============================================================================
##
## Append target compile options
##
##==============================================================================

list(APPEND OEHOST_TARGET_COMPILE_PRIVATE_OPTIONS
    -Wno-attributes -Wmissing-prototypes -fPIC ${PLATFORM_FLAGS})

list(APPEND OEHOST_TARGET_COMPILE_PUBLIC_OPTIONS
    -fstack-protector-strong)

##==============================================================================
##
## Append target compile definitions
##
##==============================================================================

list(APPEND OEHOST_TARGET_COMPILE_PUBLIC_DEFINITIONS
  $<$<NOT:$<CONFIG:debug>>:_FORTIFY_SOURCE=2>)

list(APPEND OEHOST_TARGET_COMPILE_PRIVATE_DEFINITIONS
  $<$<NOT:$<CONFIG:debug>>:_FORTIFY_SOURCE=2>
  _GNU_SOURCE)

##==============================================================================
##
## Find and append specified target configurable libraries
##
##==============================================================================

# TODO: Replace these with `find_package` and add as dependencies to
# the CMake package.

if (NOT TARGET crypto)
  find_library(CRYPTO_LIB NAMES crypto)
  if (NOT CRYPTO_LIB)
    message(FATAL_ERROR "-- Looking for crypto library - not found")
  else ()
    message("-- Looking for crypto library - found")
    add_library(crypto SHARED IMPORTED)
    set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${CRYPTO_LIB})
  endif ()
endif ()

if (NOT TARGET dlib)
  find_library(DL_LIB NAMES dl)
  if (NOT DL_LIB)
    message(FATAL_ERROR "-- Looking for dl library - not found")
  else ()
    message("-- Looking for dl library - found")
    add_library(dl SHARED IMPORTED)
    set_target_properties(dl PROPERTIES IMPORTED_LOCATION ${DL_LIB})
  endif ()
endif ()

##==============================================================================
##
## Append System configurable paths
##
##==============================================================================

if(USE_LIBSGX)
  find_library(LIBSGX_COMMON NAMES sgx_enclave_common PATHS ${LIBPATHS})
  find_library(LIBSGX_QE NAMES sgx_dcap_ql PATHS ${LIBPATHS})
  if (NOT LIBSGX_COMMON OR NOT LIBSGX_QE)
    message(FATAL_ERROR "No SGX libraries found, aborting! Set -DUSE_LIBSGX=OFF to ignore.")
  endif ()

  list(APPEND SGX_ENCLAVE_COMMON_PROPERTIES
    PROPERTIES
    IMPORTED_LOCATION
    ${LIBSGX_COMMON})

  list(APPEND SGX_DCAP_Q1_PROPERTIES
    PROPERTIES
    IMPORTED_LOCATION
    ${LIBSGX_QE})

  # SGX Properties written to parent scope here as not common required
  set(SGX_ENCLAVE_COMMON_PROPERTIES ${SGX_ENCLAVE_COMMON_PROPERTIES} PARENT_SCOPE)
  set(SGX_DCAP_Q1_PROPERTIES ${SGX_DCAP_Q1_PROPERTIES} PARENT_SCOPE)

endif()

##==============================================================================
##
## Overwrite parent scope variables
##
##==============================================================================

set(PLATFORM_HOST_ONLY_SRC ${PLATFORM_HOST_ONLY_SRC} PARENT_SCOPE)
set(PLATFORM_SDK_ONLY_SRC ${PLATFORM_SDK_ONLY_SRC} PARENT_SCOPE)

# Libraries
set(OEHOST_PRIVATE_LIBRARIES ${OEHOST_PRIVATE_LIBRARIES} PARENT_SCOPE)
set(OEHOST_VERIFY_PRIVATE_LIBRARIES ${OEHOST_VERIFY_PRIVATE_LIBRARIES} PARENT_SCOPE)
set(OEHOST_VERIFY_INTERFACE_LIBRARIES ${OEHOST_VERIFY_INTERFACE_LIBRARIES} PARENT_SCOPE)
set(OEHOSTAPP_INTERFACE_LIBRARIES ${OEHOSTAPP_INTERFACE_LIBRARIES} PARENT_SCOPE)

# Directories
set(OEHOST_PRIVATE_DIRECTORIES ${OEHOST_PRIVATE_DIRECTORIES} PARENT_SCOPE)

# Compile options/definitions
set(OEHOST_TARGET_COMPILE_PUBLIC_OPTIONS ${OEHOST_TARGET_COMPILE_PUBLIC_OPTIONS} PARENT_SCOPE)
set(OEHOST_TARGET_COMPILE_PRIVATE_OPTIONS ${OEHOST_TARGET_COMPILE_PRIVATE_OPTIONS} PARENT_SCOPE)
set(OEHOST_TARGET_COMPILE_PUBLIC_DEFINITIONS ${OEHOST_TARGET_COMPILE_PUBLIC_DEFINITIONS} PARENT_SCOPE)
set(OEHOST_TARGET_COMPILE_PRIVATE_DEFINITIONS ${OEHOST_TARGET_COMPILE_PRIVATE_DEFINITIONS} PARENT_SCOPE)