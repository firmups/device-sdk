if(FIRMUPS_TESTING)
    enable_testing()
endif()

set(TINYCBOR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/tinycbor)

set(TARGETS_EXPORT_NAME "TinyCBOR-targets")

option(WITH_TOOLS "Compile the TinyCBOR tools" OFF)
option(WITH_CBOR2JSON "Compile code to convert from CBOR to JSON" OFF)
option(WITH_FREESTANDING "Compile TinyCBOR in C freestanding mode" OFF)
if(WITH_FLOATING_POINT AND NOT WITH_FREESTANDING)
  option(WITH_FLOATING_POINT "Use floating point code in TinyCBOR" ON)
endif()

# Include additional modules that are used unconditionally
include(GenerateExportHeader)

add_library(tinycbor
  ${TINYCBOR_ROOT}/src/cborencoder.c
  ${TINYCBOR_ROOT}/src/cborencoder_close_container_checked.c
  ${TINYCBOR_ROOT}/src/cborerrorstrings.c
  ${TINYCBOR_ROOT}/src/cborparser.c
  ${TINYCBOR_ROOT}/src/cborpretty.c
  ${TINYCBOR_ROOT}/src/cborvalidation.c
  ${TINYCBOR_ROOT}/src/cbor.h
)
if(WITH_FREESTANDING)
  target_compile_options(tinycbor PUBLIC
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-ffreestanding>
  )
else()
  target_sources(tinycbor PRIVATE
    ${TINYCBOR_ROOT}/src/cborparser_dup_string.c
    ${TINYCBOR_ROOT}/src/cborpretty_stdio.c
  )
endif()
if(WITH_FLOATING_POINT)
  target_sources(tinycbor PRIVATE
    ${TINYCBOR_ROOT}/src/cborencoder_float.c
    ${TINYCBOR_ROOT}/src/cborparser_float.c
  )
  if(NOT WIN32)
    target_link_libraries(tinycbor m)
  endif()
else()
  target_compile_definitions(tinycbor PUBLIC CBOR_NO_FLOATING_POINT)
endif()

set_target_properties(tinycbor PROPERTIES
  # Force this library to link as C and compile as C99, to ensure we
  # don't use something of a newer language level.
  LINKER_LANGUAGE C
  C_EXTENSIONS OFF
  C_STANDARD 99

  # Set version and output name
  VERSION "0.${PROJECT_VERSION}"
  SOVERSION "0"
)
if(BUILD_SHARED_LIBS)
  set_target_properties(tinycbor PROPERTIES C_VISIBILITY_PRESET hidden)

  # Check if the linker supports "-z defs" (a.k.a "--no-undefined")
  check_linker_flag(C "-Wl,-z,defs" HAVE_NO_UNDEFINED)
  if(HAVE_NO_UNDEFINED)
    target_link_options(tinycbor PRIVATE "-Wl,-z,defs")
  endif()
else()
  target_compile_definitions(tinycbor PUBLIC CBOR_STATIC_DEFINE)
endif()

# Enable warnings
target_compile_options(tinycbor PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:-W3>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
    -Wall -Wextra
    -Werror=format-security
    -Werror=incompatible-pointer-types
    -Werror=implicit-function-declaration
    -Werror=int-conversion
  >
)

# Generate export macros
generate_export_header(tinycbor
  BASE_NAME "cbor"
  EXPORT_MACRO_NAME "CBOR_API"
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/export/tinycbor-export.h"
)

target_include_directories(tinycbor PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/export
  ${TINYCBOR_ROOT}/src
)

# Generate version header
configure_file(${TINYCBOR_ROOT}/src/tinycbor-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/export/tinycbor-version.h)
