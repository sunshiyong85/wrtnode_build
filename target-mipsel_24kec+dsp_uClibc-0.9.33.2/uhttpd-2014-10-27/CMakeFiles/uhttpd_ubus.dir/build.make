# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

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
CMAKE_COMMAND = /home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/host/bin/cmake

# The command to remove a file.
RM = /home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/host/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/host/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27

# Include any dependencies generated for this target.
include CMakeFiles/uhttpd_ubus.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/uhttpd_ubus.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/uhttpd_ubus.dir/flags.make

CMakeFiles/uhttpd_ubus.dir/ubus.c.o: CMakeFiles/uhttpd_ubus.dir/flags.make
CMakeFiles/uhttpd_ubus.dir/ubus.c.o: ubus.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/uhttpd_ubus.dir/ubus.c.o"
	/home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/uhttpd_ubus.dir/ubus.c.o   -c /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27/ubus.c

CMakeFiles/uhttpd_ubus.dir/ubus.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/uhttpd_ubus.dir/ubus.c.i"
	/home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc  $(C_DEFINES) $(C_FLAGS) -E /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27/ubus.c > CMakeFiles/uhttpd_ubus.dir/ubus.c.i

CMakeFiles/uhttpd_ubus.dir/ubus.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/uhttpd_ubus.dir/ubus.c.s"
	/home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-uclibc-gcc  $(C_DEFINES) $(C_FLAGS) -S /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27/ubus.c -o CMakeFiles/uhttpd_ubus.dir/ubus.c.s

CMakeFiles/uhttpd_ubus.dir/ubus.c.o.requires:
.PHONY : CMakeFiles/uhttpd_ubus.dir/ubus.c.o.requires

CMakeFiles/uhttpd_ubus.dir/ubus.c.o.provides: CMakeFiles/uhttpd_ubus.dir/ubus.c.o.requires
	$(MAKE) -f CMakeFiles/uhttpd_ubus.dir/build.make CMakeFiles/uhttpd_ubus.dir/ubus.c.o.provides.build
.PHONY : CMakeFiles/uhttpd_ubus.dir/ubus.c.o.provides

CMakeFiles/uhttpd_ubus.dir/ubus.c.o.provides.build: CMakeFiles/uhttpd_ubus.dir/ubus.c.o

# Object files for target uhttpd_ubus
uhttpd_ubus_OBJECTS = \
"CMakeFiles/uhttpd_ubus.dir/ubus.c.o"

# External object files for target uhttpd_ubus
uhttpd_ubus_EXTERNAL_OBJECTS =

uhttpd_ubus.so: CMakeFiles/uhttpd_ubus.dir/ubus.c.o
uhttpd_ubus.so: CMakeFiles/uhttpd_ubus.dir/build.make
uhttpd_ubus.so: /home/sun/workspace/sun/wrtnode/wrtnode/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/lib/libjson-c.so
uhttpd_ubus.so: CMakeFiles/uhttpd_ubus.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C shared module uhttpd_ubus.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/uhttpd_ubus.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/uhttpd_ubus.dir/build: uhttpd_ubus.so
.PHONY : CMakeFiles/uhttpd_ubus.dir/build

CMakeFiles/uhttpd_ubus.dir/requires: CMakeFiles/uhttpd_ubus.dir/ubus.c.o.requires
.PHONY : CMakeFiles/uhttpd_ubus.dir/requires

CMakeFiles/uhttpd_ubus.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/uhttpd_ubus.dir/cmake_clean.cmake
.PHONY : CMakeFiles/uhttpd_ubus.dir/clean

CMakeFiles/uhttpd_ubus.dir/depend:
	cd /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27 /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27 /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27 /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27 /home/sun/workspace/sun/wrtnode/wrtnode/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/uhttpd-2014-10-27/CMakeFiles/uhttpd_ubus.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/uhttpd_ubus.dir/depend

