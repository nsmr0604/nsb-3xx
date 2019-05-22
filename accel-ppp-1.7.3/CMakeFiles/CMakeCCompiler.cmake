SET(CMAKE_C_COMPILER "/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-gcc")
SET(CMAKE_C_COMPILER_ARG1 "")
SET(CMAKE_C_COMPILER_ID "GNU")
SET(CMAKE_C_PLATFORM_ID "Linux")

SET(CMAKE_AR "/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-ar")
SET(CMAKE_RANLIB "/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-ranlib")
SET(CMAKE_LINKER "/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-ld")
SET(CMAKE_COMPILER_IS_GNUCC 1)
SET(CMAKE_C_COMPILER_LOADED 1)
SET(CMAKE_COMPILER_IS_MINGW )
SET(CMAKE_COMPILER_IS_CYGWIN )
IF(CMAKE_COMPILER_IS_CYGWIN)
  SET(CYGWIN 1)
  SET(UNIX 1)
ENDIF(CMAKE_COMPILER_IS_CYGWIN)

SET(CMAKE_C_COMPILER_ENV_VAR "CC")

IF(CMAKE_COMPILER_IS_MINGW)
  SET(MINGW 1)
ENDIF(CMAKE_COMPILER_IS_MINGW)
SET(CMAKE_C_COMPILER_ID_RUN 1)
SET(CMAKE_C_SOURCE_FILE_EXTENSIONS c)
SET(CMAKE_C_IGNORE_EXTENSIONS h;H;o;O;obj;OBJ;def;DEF;rc;RC)
SET(CMAKE_C_LINKER_PREFERENCE 10)

# Save compiler ABI information.
SET(CMAKE_C_SIZEOF_DATA_PTR "4")
SET(CMAKE_C_COMPILER_ABI "ELF")

IF(CMAKE_C_SIZEOF_DATA_PTR)
  SET(CMAKE_SIZEOF_VOID_P "${CMAKE_C_SIZEOF_DATA_PTR}")
ENDIF(CMAKE_C_SIZEOF_DATA_PTR)

IF(CMAKE_C_COMPILER_ABI)
  SET(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_C_COMPILER_ABI}")
ENDIF(CMAKE_C_COMPILER_ABI)

SET(CMAKE_C_HAS_ISYSROOT "")


SET(CMAKE_C_IMPLICIT_LINK_LIBRARIES "c")
SET(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib/gcc/arm-openwrt-linux-uclibcgnueabi/4.5.2;/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/arm-openwrt-linux-uclibcgnueabi/lib")
