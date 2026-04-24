if(CONFIG_FIRMUPS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/warn_all.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake)

    message(STATUS "FIRMUPS enabled")

    # Declare a Zephyr library
    zephyr_library()

    if(CONFIG_FIRMUPS_ENCRYPTION_CALLBACKS)
        target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE FIRMUPS_USE_CRYPTO_CALLBACKS)
        zephyr_compile_definitions(FIRMUPS_USE_CRYPTO_CALLBACKS)
        if(CONFIG_FIRMUPS_ENCRYPTION_AES)
            target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE FIRMUPS_USE_AES)
            zephyr_compile_definitions(FIRMUPS_USE_AES)
        endif()
    elseif(CONFIG_FIRMUPS_ENCRYPTION_AES)
        message(FATAL_ERROR CONFIG_FIRMUPS_ENCRYPTION_AES needs CONFIG_FIRMUPS_ENCRYPTION_CALLBACKS)
    endif()


    if(DEFINED CONFIG_FIRMUPS_LOG_LEVEL)
        target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE FIRMUPS_LOG_LEVEL=${CONFIG_FIRMUPS_LOG_LEVEL})
        zephyr_compile_definitions(FIRMUPS_LOG_LEVEL=${CONFIG_FIRMUPS_LOG_LEVEL})
    endif()

    if(CONFIG_FIRMUPS_GATEWAY)
        target_compile_definitions(${ZEPHYR_CURRENT_LIBRARY} PRIVATE FIRMUPS_GATEWAY)
        zephyr_compile_definitions(FIRMUPS_GATEWAY)
    endif()


    # Add your own sources if needed
    zephyr_library_sources(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sdk.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/codec/cbor_helper.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/codec/cose.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/codec/crypto.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/operation/device_info.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/operation/firmware.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/operation/parameter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/operation/error.c)

    # Add tinycbor dependency
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/tinycbor.cmake)
    target_link_libraries(tinycbor PUBLIC zephyr_interface)

    # Add ascon-c dependency
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ascon.cmake)
    target_link_libraries(crypto_aead_asconaead128_${FIRMUPS_ASCON_IMPL} PUBLIC zephyr_interface)

    zephyr_library_link_libraries(tinycbor)
    zephyr_library_link_libraries(crypto_aead_asconaead128_${FIRMUPS_ASCON_IMPL})

    warn_all(${ZEPHYR_CURRENT_LIBRARY})
    configure_firmups_sdk(${ZEPHYR_CURRENT_LIBRARY})

    # Add include directories from the SDK target
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    message(STATUS "FIRMUPS disabled")
endif()
