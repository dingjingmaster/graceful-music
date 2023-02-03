project(graceful-music C)

cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD 11)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

add_definitions(-w -g
        -D PACKAGE_NAME=\\"${PROJECT_NAME}\\"
        -D G_LOG_DOMAIN=\\"${PROJECT_NAME}\\"
        -D PACKAGE_VERSION=\\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}\\"
        )

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux ")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "current platform: Windows")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    MESSAGE(STATUS "current platform: Mac OS X")
else()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")

cmake_host_system_information(RESULT OS QUERY OS_NAME)
MESSAGE(STATUS "OS name: ${OS}")

cmake_host_system_information(RESULT RELEASE QUERY OS_RELEASE)
MESSAGE(STATUS "OS release: ${RELEASE}")

execute_process(COMMAND bash -c "[[ -f /etc/os-release ]] && cat /etc/os-release|grep -E '^NAME'|awk -F'=' '{print $2}'|sed 's/\"//g'|tr '\n' ' '|sed 's/ //g'"
        OUTPUT_VARIABLE DISTRIB)
MESSAGE(STATUS "OS distribute: '${DISTRIB}'")

execute_process(COMMAND bash -c "uname -m | xargs echo -n" OUTPUT_VARIABLE ARCH)
message(STATUS "Arch: '${ARCH}'")

find_package (PkgConfig)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)
#pkg_check_modules(GTK REQUIRED gtk4)
#pkg_check_modules(PCRE REQUIRED libpcre2-8)
#pkg_check_modules(VTE REQUIRED vte-2.91-gtk4)
#pkg_check_modules(GTOP REQUIRED libgtop-2.0)
#pkg_check_modules(ADWAITA REQUIRED libadwaita-1)
#pkg_check_modules(NAUTILUS4 libnautilus-extension-4)
#pkg_check_modules(GDS REQUIRED gsettings-desktop-schemas)

#include_directories(${VTE_INCLUDE_DIRS})
#include_directories(${GTK_INCLUDE_DIRS})
include_directories(${GIO_INCLUDE_DIRS})
#include_directories(${GDS_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})
#include_directories(${GTOP_INCLUDE_DIRS})
#include_directories(${PCRE_INCLUDE_DIRS})
#include_directories(${ADWAITA_INCLUDE_DIRS})
include_directories(${GIO_UNIX_INCLUDE_DIRS})

add_subdirectory(app)
add_subdirectory(demo)
