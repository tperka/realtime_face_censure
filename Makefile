# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tperka/SZCZR/sczr_moje

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tperka/SZCZR/sczr_moje

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/bin/ccmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tperka/SZCZR/sczr_moje/CMakeFiles /home/tperka/SZCZR/sczr_moje/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tperka/SZCZR/sczr_moje/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named C.out

# Build rule for target.
C.out: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 C.out
.PHONY : C.out

# fast build rule for target.
C.out/fast:
	$(MAKE) -f CMakeFiles/C.out.dir/build.make CMakeFiles/C.out.dir/build
.PHONY : C.out/fast

#=============================================================================
# Target rules for targets named D.out

# Build rule for target.
D.out: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 D.out
.PHONY : D.out

# fast build rule for target.
D.out/fast:
	$(MAKE) -f CMakeFiles/D.out.dir/build.make CMakeFiles/D.out.dir/build
.PHONY : D.out/fast

#=============================================================================
# Target rules for targets named A.out

# Build rule for target.
A.out: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 A.out
.PHONY : A.out

# fast build rule for target.
A.out/fast:
	$(MAKE) -f CMakeFiles/A.out.dir/build.make CMakeFiles/A.out.dir/build
.PHONY : A.out/fast

#=============================================================================
# Target rules for targets named B.out

# Build rule for target.
B.out: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 B.out
.PHONY : B.out

# fast build rule for target.
B.out/fast:
	$(MAKE) -f CMakeFiles/B.out.dir/build.make CMakeFiles/B.out.dir/build
.PHONY : B.out/fast

# target to build an object file
src/main_A.o:
	$(MAKE) -f CMakeFiles/A.out.dir/build.make CMakeFiles/A.out.dir/src/main_A.o
.PHONY : src/main_A.o

# target to preprocess a source file
src/main_A.i:
	$(MAKE) -f CMakeFiles/A.out.dir/build.make CMakeFiles/A.out.dir/src/main_A.i
.PHONY : src/main_A.i

# target to generate assembly for a file
src/main_A.s:
	$(MAKE) -f CMakeFiles/A.out.dir/build.make CMakeFiles/A.out.dir/src/main_A.s
.PHONY : src/main_A.s

# target to build an object file
src/main_B.o:
	$(MAKE) -f CMakeFiles/B.out.dir/build.make CMakeFiles/B.out.dir/src/main_B.o
.PHONY : src/main_B.o

# target to preprocess a source file
src/main_B.i:
	$(MAKE) -f CMakeFiles/B.out.dir/build.make CMakeFiles/B.out.dir/src/main_B.i
.PHONY : src/main_B.i

# target to generate assembly for a file
src/main_B.s:
	$(MAKE) -f CMakeFiles/B.out.dir/build.make CMakeFiles/B.out.dir/src/main_B.s
.PHONY : src/main_B.s

# target to build an object file
src/main_C.o:
	$(MAKE) -f CMakeFiles/C.out.dir/build.make CMakeFiles/C.out.dir/src/main_C.o
.PHONY : src/main_C.o

# target to preprocess a source file
src/main_C.i:
	$(MAKE) -f CMakeFiles/C.out.dir/build.make CMakeFiles/C.out.dir/src/main_C.i
.PHONY : src/main_C.i

# target to generate assembly for a file
src/main_C.s:
	$(MAKE) -f CMakeFiles/C.out.dir/build.make CMakeFiles/C.out.dir/src/main_C.s
.PHONY : src/main_C.s

# target to build an object file
src/main_D.o:
	$(MAKE) -f CMakeFiles/D.out.dir/build.make CMakeFiles/D.out.dir/src/main_D.o
.PHONY : src/main_D.o

# target to preprocess a source file
src/main_D.i:
	$(MAKE) -f CMakeFiles/D.out.dir/build.make CMakeFiles/D.out.dir/src/main_D.i
.PHONY : src/main_D.i

# target to generate assembly for a file
src/main_D.s:
	$(MAKE) -f CMakeFiles/D.out.dir/build.make CMakeFiles/D.out.dir/src/main_D.s
.PHONY : src/main_D.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... C.out"
	@echo "... D.out"
	@echo "... A.out"
	@echo "... B.out"
	@echo "... src/main_A.o"
	@echo "... src/main_A.i"
	@echo "... src/main_A.s"
	@echo "... src/main_B.o"
	@echo "... src/main_B.i"
	@echo "... src/main_B.s"
	@echo "... src/main_C.o"
	@echo "... src/main_C.i"
	@echo "... src/main_C.s"
	@echo "... src/main_D.o"
	@echo "... src/main_D.i"
	@echo "... src/main_D.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

