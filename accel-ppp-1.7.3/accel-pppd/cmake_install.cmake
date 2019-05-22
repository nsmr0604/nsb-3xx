# Install script for directory: /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd"
         RPATH "/usr/lib/accel-ppp")
  ENDIF()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/sbin" TYPE EXECUTABLE FILES "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/accel-pppd")
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd")
    FILE(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd"
         OLD_RPATH "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/triton:"
         NEW_RPATH "/usr/lib/accel-ppp")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/:" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/sbin/accel-pppd")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man5" TYPE FILE FILES "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/accel-ppp.conf.5")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/etc/accel-ppp.conf.dist")
FILE(INSTALL DESTINATION "/etc" TYPE FILE RENAME "accel-ppp.conf.dist" FILES "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/accel-ppp.conf")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  EXECUTE_PROCESS(COMMAND mkdir -p /var/log/accel-ppp)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  EXECUTE_PROCESS(COMMAND mkdir -p /var/run/accel-ppp)
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  EXECUTE_PROCESS(COMMAND 'echo 0 > /var/run/accel-ppp/seq')
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/triton/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/ctrl/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/auth/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/logs/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra/cmake_install.cmake")
  INCLUDE("/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/ipv6/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

