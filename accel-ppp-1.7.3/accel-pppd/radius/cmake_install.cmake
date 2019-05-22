# Install script for directory: /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius

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
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so"
         RPATH "")
  ENDIF()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/accel-ppp" TYPE SHARED_LIBRARY FILES "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/libradius.so")
  IF(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/:" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/accel-ppp/libradius.so")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/accel-ppp/radius" TYPE FILE FILES
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc3162"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4675"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc3580"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc2867"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4818"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4849"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4679"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc2868"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc2869"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.cisco"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4372"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.microsoft"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc5176"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc3576"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc2866"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc2865"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.alcatel"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary"
    "/home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/radius/dict/dictionary.rfc4072"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

