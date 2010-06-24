# -*- mode: cmake -*-
#Cmakelists.txt
# Minimum version of CMake
cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(VERSION 2.6)
endif()

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Source and build dirs
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Project name - hftirc
set(PROJECT_NAME hftirc)
project(${PROJECT_NAME} C)

# Definition of the hftirc source
set(hftirc_src
  src/parse/parse.c
  src/config.c
  src/util.c
  src/ui.c
  src/input.c
  src/irc.c
  src/event.c
  src/hftirc.c
  )

# Set the executable from the hftirc_src
add_executable(hftirc ${hftirc_src})

# FLAGS
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ansi -D_GNU_SOURCE")

# Linker FLAGS
set(DEFAULT_LDFLAGS "-L /usr/local/lib -lncurses")

if(CMAKE_SYSTEM_NAME MATCHES NetBSD)
    message("-- NetBSD system found - Using /usr/pkg/lib for linker")
    set(LDFLAGS "${DEFAULT_LDFLAGS} -L /usr/pkg/lib")
else(CMAKE_SYSTEM_NAME MATCHES NetBSD)
    set(LDFLAGS ${DEFAULT_LDFLAGS})
endif(CMAKE_SYSTEM_NAME MATCHES NetBSD)

set_target_properties(hftirc PROPERTIES LINK_FLAGS ${LDFLAGS})
# Includes dir for libs in build_dir
include_directories(${BUILD_DIR}/src)

# Messages
message("Project version: ${VERSION}")
message("Using these CFLAGS: ${CMAKE_C_FLAGS}")
message("Using these LDFLAGS: ${LDFLAGS}")
message("Linking with theses libraries : ${LIBRARIES_TO_LINK}")

# sets
# {{{ Install path and configuration variables
if(DEFINED PREFIX)
    set(PREFIX ${PREFIX} CACHE PATH "install prefix")
    set(CMAKE_INSTALL_PREFIX ${PREFIX})
else()
    set(PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "install prefix")
endif()

#If a sysconfdir is specified, use it instead
#of the default configuration dir.
if(DEFINED SYSCONFDIR)
    set(SYSCONFDIR ${SYSCONFDIR} CACHE PATH "config directory")
else()
    set(SYSCONFDIR /etc CACHE PATH "config directory")
endif()

#If an XDG Config Dir is specificed, use it instead
#of the default XDG configuration dir.
if(DEFINED XDG_CONFIG_DIR)
    set(XDG_CONFIG_DIR ${XDG_CONFIG_SYS} CACHE PATH "xdg config directory")
else()
    set(XDG_CONFIG_DIR ${SYSCONFDIR}/xdg CACHE PATH "xdg config directory")
endif()

set(PROJECT_DATA_PATH share/${PROJECT_NAME})
set(PROJECT_TODO ${SOURCE_DIR}/TODO)
set(PROJECT_README ${SOURCE_DIR}/README)
set(PROJECT_DEFAULT_CONF ${SOURCE_DIR}/hftirc.conf)

# installs
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

install(FILES ${PROJECT_TODO} ${PROJECT_README}
  ${PROJECT_DEFAULT_CONF} DESTINATION ${PROJECT_DATA_PATH})

