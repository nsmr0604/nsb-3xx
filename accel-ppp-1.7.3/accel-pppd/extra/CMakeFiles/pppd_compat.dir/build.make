# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/igarashi/cortina/staging_dir/host/bin/cmake

# The command to remove a file.
RM = /home/igarashi/cortina/staging_dir/host/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /home/igarashi/cortina/staging_dir/host/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3

# Include any dependencies generated for this target.
include accel-pppd/extra/CMakeFiles/pppd_compat.dir/depend.make

# Include the progress variables for this target.
include accel-pppd/extra/CMakeFiles/pppd_compat.dir/progress.make

# Include the compile flags for this target's objects.
include accel-pppd/extra/CMakeFiles/pppd_compat.dir/flags.make

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o: accel-pppd/extra/CMakeFiles/pppd_compat.dir/flags.make
accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o: accel-pppd/extra/pppd_compat.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o"
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra && /home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/pppd_compat.dir/pppd_compat.c.o   -c /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra/pppd_compat.c

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pppd_compat.dir/pppd_compat.c.i"
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra && /home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-gcc  $(C_DEFINES) $(C_FLAGS) -E /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra/pppd_compat.c > CMakeFiles/pppd_compat.dir/pppd_compat.c.i

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pppd_compat.dir/pppd_compat.c.s"
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra && /home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/bin/arm-openwrt-linux-uclibcgnueabi-gcc  $(C_DEFINES) $(C_FLAGS) -S /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra/pppd_compat.c -o CMakeFiles/pppd_compat.dir/pppd_compat.c.s

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.requires:
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.requires

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.provides: accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.requires
	$(MAKE) -f accel-pppd/extra/CMakeFiles/pppd_compat.dir/build.make accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.provides.build
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.provides

accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.provides.build: accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.provides.build

# Object files for target pppd_compat
pppd_compat_OBJECTS = \
"CMakeFiles/pppd_compat.dir/pppd_compat.c.o"

# External object files for target pppd_compat
pppd_compat_EXTERNAL_OBJECTS =

accel-pppd/extra/libpppd_compat.so: accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o
accel-pppd/extra/libpppd_compat.so: accel-pppd/extra/libsigchld.so
accel-pppd/extra/libpppd_compat.so: accel-pppd/extra/CMakeFiles/pppd_compat.dir/build.make
accel-pppd/extra/libpppd_compat.so: accel-pppd/extra/CMakeFiles/pppd_compat.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C shared library libpppd_compat.so"
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pppd_compat.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
accel-pppd/extra/CMakeFiles/pppd_compat.dir/build: accel-pppd/extra/libpppd_compat.so
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/build

accel-pppd/extra/CMakeFiles/pppd_compat.dir/requires: accel-pppd/extra/CMakeFiles/pppd_compat.dir/pppd_compat.c.o.requires
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/requires

accel-pppd/extra/CMakeFiles/pppd_compat.dir/clean:
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra && $(CMAKE_COMMAND) -P CMakeFiles/pppd_compat.dir/cmake_clean.cmake
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/clean

accel-pppd/extra/CMakeFiles/pppd_compat.dir/depend:
	cd /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3 /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3 /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra /home/igarashi/cortina/build_dir/linux-g2_wan/accel-ppp-1.7.3/accel-pppd/extra/CMakeFiles/pppd_compat.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : accel-pppd/extra/CMakeFiles/pppd_compat.dir/depend
