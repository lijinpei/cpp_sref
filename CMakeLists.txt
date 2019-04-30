project (cpp_static_reflect)
cmake_minimum_required(VERSION 3.14)

set (CMAKE_CXX_STANDARD 17)

# TODO: this doesn't seem to have realistic effect
find_package (Clang REQUIRED)
find_package (LLVM REQUIRED)

message (INFO " cmake is using clang from ${Clang_DIR}")
message (INFO " cmake is using llvm from ${LLVM_DIR}")
message (INFO " cmake is using llvm version ${LLVM_PACKAGE_VERSION}")

add_subdirectory (src)
