# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_SOURCE_DIR = /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ex2.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ex2.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ex2.dir/flags.make

CMakeFiles/ex2.dir/demo_itimer.c.o: CMakeFiles/ex2.dir/flags.make
CMakeFiles/ex2.dir/demo_itimer.c.o: ../demo_itimer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/ex2.dir/demo_itimer.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/ex2.dir/demo_itimer.c.o   -c /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_itimer.c

CMakeFiles/ex2.dir/demo_itimer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ex2.dir/demo_itimer.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_itimer.c > CMakeFiles/ex2.dir/demo_itimer.c.i

CMakeFiles/ex2.dir/demo_itimer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ex2.dir/demo_itimer.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_itimer.c -o CMakeFiles/ex2.dir/demo_itimer.c.s

CMakeFiles/ex2.dir/demo_jmp.c.o: CMakeFiles/ex2.dir/flags.make
CMakeFiles/ex2.dir/demo_jmp.c.o: ../demo_jmp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/ex2.dir/demo_jmp.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/ex2.dir/demo_jmp.c.o   -c /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_jmp.c

CMakeFiles/ex2.dir/demo_jmp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ex2.dir/demo_jmp.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_jmp.c > CMakeFiles/ex2.dir/demo_jmp.c.i

CMakeFiles/ex2.dir/demo_jmp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ex2.dir/demo_jmp.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_jmp.c -o CMakeFiles/ex2.dir/demo_jmp.c.s

CMakeFiles/ex2.dir/demo_singInt_handler.c.o: CMakeFiles/ex2.dir/flags.make
CMakeFiles/ex2.dir/demo_singInt_handler.c.o: ../demo_singInt_handler.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/ex2.dir/demo_singInt_handler.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/ex2.dir/demo_singInt_handler.c.o   -c /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_singInt_handler.c

CMakeFiles/ex2.dir/demo_singInt_handler.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ex2.dir/demo_singInt_handler.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_singInt_handler.c > CMakeFiles/ex2.dir/demo_singInt_handler.c.i

CMakeFiles/ex2.dir/demo_singInt_handler.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ex2.dir/demo_singInt_handler.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/demo_singInt_handler.c -o CMakeFiles/ex2.dir/demo_singInt_handler.c.s

CMakeFiles/ex2.dir/uthreads.cpp.o: CMakeFiles/ex2.dir/flags.make
CMakeFiles/ex2.dir/uthreads.cpp.o: ../uthreads.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/ex2.dir/uthreads.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ex2.dir/uthreads.cpp.o -c /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/uthreads.cpp

CMakeFiles/ex2.dir/uthreads.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ex2.dir/uthreads.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/uthreads.cpp > CMakeFiles/ex2.dir/uthreads.cpp.i

CMakeFiles/ex2.dir/uthreads.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ex2.dir/uthreads.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/uthreads.cpp -o CMakeFiles/ex2.dir/uthreads.cpp.s

# Object files for target ex2
ex2_OBJECTS = \
"CMakeFiles/ex2.dir/demo_itimer.c.o" \
"CMakeFiles/ex2.dir/demo_jmp.c.o" \
"CMakeFiles/ex2.dir/demo_singInt_handler.c.o" \
"CMakeFiles/ex2.dir/uthreads.cpp.o"

# External object files for target ex2
ex2_EXTERNAL_OBJECTS =

ex2: CMakeFiles/ex2.dir/demo_itimer.c.o
ex2: CMakeFiles/ex2.dir/demo_jmp.c.o
ex2: CMakeFiles/ex2.dir/demo_singInt_handler.c.o
ex2: CMakeFiles/ex2.dir/uthreads.cpp.o
ex2: CMakeFiles/ex2.dir/build.make
ex2: CMakeFiles/ex2.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX executable ex2"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ex2.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ex2.dir/build: ex2

.PHONY : CMakeFiles/ex2.dir/build

CMakeFiles/ex2.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ex2.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ex2.dir/clean

CMakeFiles/ex2.dir/depend:
	cd /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2 /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2 /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug /mnt/c/Users/97254/Documents/acadmy/OS-git/ex2/cmake-build-debug/CMakeFiles/ex2.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ex2.dir/depend
