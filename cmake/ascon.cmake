if(FIRMUPS_TESTING)
    enable_testing()
endif()

set(ASCON_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/ascon-c)
set(TEST_PATH ${ASCON_ROOT}/tests)

if(MSVC)
  set(DEFAULT_REL_FLAGS /O2)
  set(DEFAULT_DBG_FLAGS /Od)
else()
  set(DEFAULT_REL_FLAGS -std=c99 -O2 -fomit-frame-pointer -march=native -mtune=native)
  set(DEFAULT_DBG_FLAGS -std=c99 -O2 -Wall -Wextra -Wshadow)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0" AND NOT WIN32 AND NOT CYGWIN AND NOT MSYS)
  # use sanitizers in default Debug build (not on windows and only of target_link_option is available)
  set(DEFAULT_DBG_FLAGS ${DEFAULT_DBG_FLAGS} -fsanitize=address,undefined)
endif()

# set cmake variables for version, algorithms, implementations, tests, flags, defs
set(REL_FLAGS ${DEFAULT_REL_FLAGS} CACHE STRING "Define custom Release (performance) flags.")
set(DBG_FLAGS ${DEFAULT_DBG_FLAGS} CACHE STRING "Define custom Debug (NIST) flags.")

set(IMPL_PATH dependencies/ascon-c/crypto_aead/asconaead128/${FIRMUPS_ASCON_IMPL})
message("Adding implementation ${IMPL_PATH}")
set(IMPL_NAME crypto_aead_asconaead128_${FIRMUPS_ASCON_IMPL})
file(GLOB IMPL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${IMPL_PATH}/*.[chS]")
if(${FIRMUPS_ASCON_IMPL} MATCHES protected.*)
    set(IMPL_FILES ${IMPL_FILES} ${TEST_PATH}/randombytes.h)
endif()

add_library(${IMPL_NAME} STATIC ${IMPL_FILES})
target_compile_options(${IMPL_NAME} PRIVATE -fvisibility=hidden)
target_include_directories(${IMPL_NAME} PUBLIC ${IMPL_PATH} ${TEST_PATH})
target_compile_definitions(${IMPL_NAME} PRIVATE ${COMPILE_DEFS})
target_compile_options(${IMPL_NAME} PUBLIC $<$<CONFIG:RELEASE>:${REL_FLAGS}>)
target_compile_options(${IMPL_NAME} PUBLIC $<$<CONFIG:DEBUG>:${DBG_FLAGS}>)

if(FIRMUPS_TESTING)
    set(TEST_FILES ${TEST_PATH}/crypto_aead.h ${TEST_PATH}/genkat_aead.c)
    string(TOUPPER CRYPTO_aead DEFINE_CRYPTO)
    if(${FIRMUPS_ASCON_IMPL} MATCHES protected)
    set(DEFINE_CRYPTO ${DEFINE_CRYPTO}_SHARED)
    endif()
    set(EXE_NAME genkat_${IMPL_NAME})
    add_executable(${EXE_NAME} ${TEST_FILES})
    target_compile_definitions(${EXE_NAME} PRIVATE ${DEFINE_CRYPTO} ${DEFINE_MAXMSGLEN})
    target_link_options(${EXE_NAME} PRIVATE $<$<CONFIG:DEBUG>:${DBG_FLAGS}>)
    target_link_options(${EXE_NAME} PRIVATE $<$<CONFIG:RELEASE>:${REL_FLAGS}>)
    target_link_libraries(${EXE_NAME} PRIVATE ${IMPL_NAME})
    add_test(NAME ${EXE_NAME} COMMAND ${CMAKE_COMMAND}
        -DEXE_NAME=${EXE_NAME} -DALG=asconaead128 -DCRYPTO=aead
        -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/dependencies/ascon-c -DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DCONFIG=$<CONFIGURATION> -P ${TEST_PATH}/genkat.cmake)
endif()
