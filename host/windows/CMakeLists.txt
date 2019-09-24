# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## these rules build the oehost target.
##
##==============================================================================

# OS specific but arch agnostic files.

list(APPEND PLATFORM_HOST_ONLY_SRC
  ../common/cert.c
  crypto/bcrypt/cert.c
  crypto/bcrypt/crl.c
  crypto/bcrypt/ec.c
  crypto/bcrypt/key.c
  crypto/bcrypt/rsa.c
  crypto/bcrypt/sha.c
  crypto/bcrypt/pem.c
  windows/hostthread.c)

list(APPEND PLATFORM_SDK_ONLY_SRC
  ../3rdparty/mbedtls/mbedtls/library/bignum.c
  ../common/asn1.c
  ../common/cert.c
  crypto/bcrypt/cert.c
  crypto/bcrypt/crl.c
  crypto/bcrypt/ec.c
  crypto/bcrypt/hmac.c
  crypto/bcrypt/key.c
  crypto/bcrypt/pem.c
  crypto/bcrypt/random.c
  crypto/bcrypt/rsa.c
  crypto/bcrypt/sha.c
  windows/hostthread.c
  windows/syscall.c
  windows/time.c)

list(APPEND OEHOST_PUBLIC_LIBRARIES
  ws2_32)

list(APPEND OEHOST_PRIVATE_LIBRARIES
  bcrypt
  Crypt32)

list(APPEND OEHOST_VERIFY_PRIVATE_LIBRARIES
  bcrypt
  Crypt32)

list(APPEND OEHOST_PRIVATE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/3rdparty/mbedtls/mbedtls/include)

list(APPEND OEHOST_VERIFY_PRIVATE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/3rdparty/mbedtls/mbedtls/include)

##==============================================================================
##
## LIBSGX logic
##
##==============================================================================

if(USE_LIBSGX)
  set(LIBPATHS
    ${CMAKE_SOURCE_DIR}/prereqs/nuget/EnclaveCommonAPI/lib/native/x64-Release
    ${CMAKE_SOURCE_DIR}/prereqs/nuget/DCAP_Components/build/lib/native/Libraries)
  
  set(INCPATHS
    "${CMAKE_SOURCE_DIR}/prereqs/nuget/EnclaveCommonAPI/Header Files"
    "${CMAKE_SOURCE_DIR}/prereqs/nuget/DCAP_Components/build/Header Files")
  
  set(WINSYSLOCATION $ENV{WINDIR}/System32)

  find_library(LIBSGX_COMMON NAMES sgx_enclave_common PATHS ${LIBPATHS})
  find_library(LIBSGX_QE NAMES sgx_dcap_ql PATHS ${LIBPATHS})
  if (NOT LIBSGX_COMMON OR NOT LIBSGX_QE)
    message(FATAL_ERROR "No SGX libraries found, aborting! Set -DUSE_LIBSGX=OFF to ignore.")
  endif ()

  list(APPEND SGX_ENCLAVE_COMMON_PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${INCPATHS}"
    IMPORTED_LOCATION ${WINSYSLOCATION}
    IMPORTED_IMPLIB ${LIBSGX_COMMON})

  list(APPEND SGX_DCAP_Q1_PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${INCPATHS}"
    IMPORTED_LOCATION ${WINSYSLOCATION}
    IMPORTED_IMPLIB ${LIBSGX_QE})

  set(LIBSGX_COMMON ${LIBSGX_COMMON} PARENT_SCOPE)
  set(SGX_ENCLAVE_COMMON_PROPERTIES ${SGX_ENCLAVE_COMMON_PROPERTIES} PARENT_SCOPE)
  set(SGX_DCAP_Q1_PROPERTIES ${SGX_DCAP_Q1_PROPERTIES} PARENT_SCOPE)
endif()


set(PLATFORM_HOST_ONLY_SRC ${PLATFORM_HOST_ONLY_SRC} PARENT_SCOPE)
set(PLATFORM_SDK_ONLY_SRC ${PLATFORM_SDK_ONLY_SRC} PARENT_SCOPE)

set(OEHOST_VERIFY_PRIVATE_LIBRARIES ${OEHOST_VERIFY_PRIVATE_LIBRARIES} PARENT_SCOPE)
set(OEHOST_PUBLIC_LIBRARIES ${OEHOST_PUBLIC_LIBRARIES} PARENT_SCOPE)
set(OEHOST_PRIVATE_DIRECTORIES ${OEHOST_PRIVATE_DIRECTORIES} PARENT_SCOPE)
set(OEHOST_VERIFY_PRIVATE_DIRECTORIES ${OEHOST_VERIFY_PRIVATE_DIRECTORIES} PARENT_SCOPE)

set(OEHOST_PRIVATE_LIBRARIES ${OEHOST_PRIVATE_LIBRARIES} PARENT_SCOPE)
set(OEHOST_LIBRARIES ${OEHOST_LIBRARIES} PARENT_SCOPE)

set(LIBPATHS ${LIBPATHS} PARENT_SCOPE)
set(INCPATHS ${INCPATHS} PARENT_SCOPE)
set(WINSYSLOCATION ${WINSYSLOCATION} PARENT_SCOPE)