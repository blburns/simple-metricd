# CMakeLists.txt for simple-metricd
# Simple Metrics Daemon - A lightweight Prometheus-style metrics daemon
# Copyright 2026 SimpleDaemons

cmake_minimum_required(VERSION 3.16)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()
project(simple-metricd VERSION 0.3.0 LANGUAGES CXX)

set(PROJECT_DESCRIPTION "Simple Metrics Daemon - A lightweight Prometheus-style metrics daemon")

if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix")
    endif()
endif()
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(PLATFORM_NAME "Windows")
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLATFORM_NAME "FreeBSD")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_PACKAGING "Enable package generation" ON)
option(ENABLE_SSL "Enable SSL/TLS support" OFF)
option(ENABLE_JSON "Enable JSON support" OFF)
option(ENABLE_SQLITE "Enable SQLite snapshot support" OFF)
option(ENABLE_STATIC_LINKING "Enable static linking for self-contained binaries" OFF)
set(METRIC_CLI_PREFIX "" CACHE STRING "Prefix for metricctl tool name (e.g. simple-)")

find_package(Threads REQUIRED)

if(ENABLE_SSL)
    if(APPLE)
        foreach(_openssl_hint IN ITEMS
                "/opt/homebrew/opt/openssl@3"
                "/opt/homebrew/opt/openssl"
                "/usr/local/opt/openssl@3"
                "/usr/local/opt/openssl")
            if(EXISTS "${_openssl_hint}")
                list(PREPEND CMAKE_PREFIX_PATH "${_openssl_hint}")
            endif()
        endforeach()
    endif()
    find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE LIB_SOURCES
    "src/simple-metricd/*.cpp"
    "src/simple-metricd/*.c"
)
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ws2_32)
endif()

if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC SIMPLE_METRICD_SSL=1)
endif()

set(_metric_warning_flags)
if(MSVC)
    set(_metric_warning_flags /W3 /WX)
else()
    set(_metric_warning_flags -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND _metric_warning_flags -Wno-unused-private-field)
    endif()
endif()
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${_metric_warning_flags})

if(ENABLE_STATIC_LINKING)
    set(BUILD_SHARED_LIBS OFF)
    if(MSVC)
        target_compile_options(${PROJECT_NAME}_lib PRIVATE /MT)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
        target_link_options(${PROJECT_NAME}_lib PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

set(METRIC_BINARIES
    simple-metricd
    metricctl
)

foreach(_bin IN LISTS METRIC_BINARIES)
    add_executable(${_bin} main/${_bin}.cpp)
    target_link_libraries(${_bin} PRIVATE ${PROJECT_NAME}_lib)
    target_compile_options(${_bin} PRIVATE ${_metric_warning_flags})
    if(METRIC_CLI_PREFIX AND NOT _bin STREQUAL "simple-metricd")
        set_target_properties(${_bin} PROPERTIES OUTPUT_NAME "${METRIC_CLI_PREFIX}${_bin}")
    endif()
endforeach()

set(METRIC_RUNTIME_TARGETS ${METRIC_BINARIES})

if(APPLE)
    install(TARGETS ${METRIC_RUNTIME_TARGETS}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(FILES deployment/launchd/com.simpledaemons.${PROJECT_NAME}.plist
        DESTINATION /Library/LaunchDaemons
    )
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
else()
    install(TARGETS ${METRIC_RUNTIME_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    install(DIRECTORY DESTINATION /var/lib/${PROJECT_NAME})
    install(DIRECTORY DESTINATION /var/log/${PROJECT_NAME})
    if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        install(FILES deployment/systemd/${PROJECT_NAME}.service
            DESTINATION lib/systemd/system
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.sysusers
            DESTINATION lib/sysusers.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/systemd/${PROJECT_NAME}.tmpfiles
            DESTINATION lib/tmpfiles.d
            RENAME ${PROJECT_NAME}.conf
        )
        install(FILES deployment/logrotate.d/${PROJECT_NAME}
            DESTINATION /etc/logrotate.d
        )
    endif()
endif()

if(NOT WIN32)
    set(_metric_confdir "/etc/${PROJECT_NAME}")
    install(DIRECTORY config/templates/
        DESTINATION ${_metric_confdir}/templates
        FILES_MATCHING PATTERN "*.conf"
    )
    install(DIRECTORY config/examples/
        DESTINATION ${_metric_confdir}/examples
    )
    install(FILES config/README.md
        DESTINATION ${_metric_confdir}
    )
    install(DIRECTORY DESTINATION ${_metric_confdir}/tls)
    install(FILES LICENSE README.md CHANGELOG.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
    install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
        FILES_MATCHING PATTERN "*.md"
    )
endif()

if(ENABLE_TESTS)
    enable_testing()
    function(add_metric_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE ${PROJECT_NAME}_lib)
        target_compile_options(${name} PRIVATE ${_metric_warning_flags})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_metric_test(test_metric_config tests/unit/test_metric_config.cpp)
    add_metric_test(test_metric_registry tests/unit/test_metric_registry.cpp)
    add_metric_test(test_metrics_http tests/unit/test_metrics_http.cpp)
endif()

if(ENABLE_PACKAGING)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "SimpleDaemons")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_CONTACT "info@simpledaemons.com")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")

    if(APPLE)
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/LICENSE.txt")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/readme.html")
        set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/packaging/assets/welcome.html")
    else()
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    endif()

    if(WIN32)
        set(CPACK_GENERATOR "WIX;ZIP;NSIS")
        set(CPACK_WIX_PRODUCT_GUID "c1e8a4d3-7b25-4f81-a9d6-e2f0b3c5d716")
        set(CPACK_WIX_UPGRADE_GUID "d2f9b5e4-8c36-5092-b0e7-f3a1c4d6e827")
        set(CPACK_NSIS_DISPLAY_NAME "Simple Metrics Daemon")
        set(CPACK_NSIS_PACKAGE_NAME "simple-metricd")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop;productbuild")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_PRODUCTBUILD_DOMAINS TRUE)
        set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        set(CPACK_POSTFLIGHT_Unspecified_SCRIPT
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/pkg/scripts/postinstall")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(CPACK_GENERATOR "FREEBSD")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_FREEBSD_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_FREEBSD_PACKAGE_ORIGIN "net/${PROJECT_NAME}")
        set(CPACK_FREEBSD_PACKAGE_LICENSE "APACHE20")
    else()
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_PACKAGING_INSTALL_PREFIX "/")
        set(CPACK_SET_DESTDIR TRUE)
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "SimpleDaemons <info@simpledaemons.com>")
        set(CPACK_DEBIAN_PACKAGE_SECTION "net")
        set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postinst"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/prerm"
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/deb/postrm"
        )
        set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
        set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
        set(CPACK_RPM_PACKAGE_URL "https://github.com/simpledaemons/${PROJECT_NAME}")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
        set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preinstall.sh")
        set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postinstall.sh")
        set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/preuninstall.sh")
        set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/rpm/postuninstall.sh")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
        set(PACKAGE_ARCH "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(PACKAGE_ARCH "arm64")
    else()
        set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(PLATFORM_ID "macos-apple")
        else()
            set(PLATFORM_ID "macos-intel")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(PLATFORM_ID "freebsd")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    elseif(UNIX)
        set(PLATFORM_ID "linux-generic")
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_ID}-${PACKAGE_ARCH}")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_NAME}-${PACKAGE_ARCH}")
    endif()

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/"
        "/dist/"
        "/.git/"
        "/.gitignore"
        "/.vscode/"
        "/CMakeCache.txt"
        "/CMakeFiles/"
        "/_CPack_Packages/"
        "/.DS_Store"
    )
    include(CPack)
endif()

message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Tests enabled: ${ENABLE_TESTS}")
message(STATUS "  Packaging enabled: ${ENABLE_PACKAGING}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  CLI prefix: ${METRIC_CLI_PREFIX}")
message(STATUS "")
