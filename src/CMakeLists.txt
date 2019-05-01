add_library (static_reflection SHARED static_reflection.cpp)
target_link_libraries (static_reflection PRIVATE clang LLVM fmt)
