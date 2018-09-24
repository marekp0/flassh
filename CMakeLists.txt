cmake_minimum_required(VERSION 3.1)
project(flassh)

# compiler flags
set(CMAKE_CXX_STANDARD 17)

# There is no Findlibssh.cmake, so find libssh.so manually
find_library(SSH_LIBRARY NAMES ssh)
# Assume the header files are in the standard location that the compiler can see

# Multithreading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# get all the C++ source files in src/
file(GLOB_RECURSE flassh_SRC "src/*.hpp" "src/*.cpp")

add_executable(flassh ${flassh_SRC})
target_link_libraries(flassh ${SSH_LIBRARY} Threads::Threads)

# installation
install(TARGETS flassh
        RUNTIME DESTINATION bin)

