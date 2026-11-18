option(P64_USE_RSIMLIB "Build rsimlib-backed p64simlib wrapper" OFF)
set(P64_RSIMLIB_DIR "/home/dave/Documents/Projects/rsimlib" CACHE PATH "Path to rsimlib (simlib) checkout")

# Build as a shared library so it can carry rsimlib + dependency linkage.
# This avoids requiring users to manually pass the long transitive link line
# to `p64 --link ...`.
add_library(p64simlib SHARED)
target_include_directories(p64simlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(P64_USE_RSIMLIB AND EXISTS "${P64_RSIMLIB_DIR}/CMakeLists.txt")
  # rsimlib is a large project with many optional dependencies; keep it opt-in.
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  add_subdirectory(${P64_RSIMLIB_DIR} ${CMAKE_CURRENT_BINARY_DIR}/rsimlib EXCLUDE_FROM_ALL)
  target_sources(p64simlib PRIVATE p64_simlib_rsimlib.cpp)
  target_link_libraries(p64simlib PUBLIC simlib)
  target_compile_definitions(p64simlib PRIVATE P64SIMLIB_USE_RSIMLIB=1)
else()
  target_sources(p64simlib PRIVATE p64_simlib_stub.cpp)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(p64simlib PRIVATE -Wall -Wextra -Wpedantic)
endif()

