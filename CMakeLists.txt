cmake_minimum_required(VERSION 3.20)
project(Pascal64 VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(p64rt STATIC runtime/runtime.cpp)
add_executable(p64 src/main.cpp src/parser.cpp src/semantic.cpp src/codegen.cpp)
target_compile_definitions(p64 PRIVATE P64_RUNTIME_DIR="${CMAKE_CURRENT_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(p64 PRIVATE -Wall -Wextra -Wpedantic)
endif()
include(CTest)
find_program(CLANG_EXECUTABLE NAMES clang-18 clang-19 clang-17 clang)
if(CLANG_EXECUTABLE)
 add_library(p64testffi STATIC tests/ffi.cpp)
 add_library(p64testgtk STATIC tests/gtk_fake.cpp)
 add_test(NAME compile_hello COMMAND p64 ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.pas -o ${CMAKE_CURRENT_BINARY_DIR}/hello --clang ${CLANG_EXECUTABLE})
 add_test(NAME run_hello COMMAND ${CMAKE_CURRENT_BINARY_DIR}/hello)
 set_tests_properties(run_hello PROPERTIES DEPENDS compile_hello PASS_REGULAR_EXPRESSION "Hello from Pascal64 10")
 add_test(NAME compile_primes COMMAND p64 ${CMAKE_CURRENT_SOURCE_DIR}/examples/primes.pas -o ${CMAKE_CURRENT_BINARY_DIR}/primes --clang ${CLANG_EXECUTABLE})
 add_test(NAME run_primes COMMAND ${CMAKE_CURRENT_BINARY_DIR}/primes)
 set_tests_properties(run_primes PROPERTIES DEPENDS compile_primes PASS_REGULAR_EXPRESSION "2 3 5 7 11")
 add_test(NAME milestone2 COMMAND ${CMAKE_COMMAND}
  -DP64=$<TARGET_FILE:p64> -DCLANG=${CLANG_EXECUTABLE}
  -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/routines.pas -DRECORD_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/records.pas -DARRAY_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/arrays.pas -DSTRING_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/strings.pas -DCLASS_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/classes.pas -DFFI_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/ffi.pas -DFFI_LIBRARY=$<TARGET_FILE:p64testffi> -DUNIT_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/units.pas -DUNIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/units -DWORK=${CMAKE_CURRENT_BINARY_DIR}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate.cmake)
 add_test(NAME gtk_controls_mock COMMAND ${CMAKE_COMMAND}
  -DP64=$<TARGET_FILE:p64> -DCLANG=${CLANG_EXECUTABLE}
  -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/gtk_hello.pas -DUNIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/units
  -DGTK_LIBRARY=$<TARGET_FILE:p64testgtk> -DWORK=${CMAKE_CURRENT_BINARY_DIR}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/gtk_mock.cmake)
 find_library(GTK3_LIBRARY NAMES gtk-3)
 find_library(GOBJECT_LIBRARY NAMES gobject-2.0)
 if(GTK3_LIBRARY AND GOBJECT_LIBRARY)
  add_test(NAME gtk_interop COMMAND ${CMAKE_COMMAND}
   -DP64=$<TARGET_FILE:p64> -DCLANG=${CLANG_EXECUTABLE}
   -DGTK_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/gtk_hello.pas -DPROBE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/gtk_probe.pas
   -DGTK_LIBRARY=${GTK3_LIBRARY} -DGOBJECT_LIBRARY=${GOBJECT_LIBRARY} -DUNIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/units -DWORK=${CMAKE_CURRENT_BINARY_DIR}
   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/gtk_validate.cmake)
 endif()
 execute_process(COMMAND ${CLANG_EXECUTABLE} --print-file-name=libclang_rt.asan-x86_64.a
  OUTPUT_VARIABLE CLANG_ASAN_RUNTIME OUTPUT_STRIP_TRAILING_WHITESPACE)
 if(EXISTS "${CLANG_ASAN_RUNTIME}")
  add_test(NAME string_lifetime COMMAND ${CMAKE_COMMAND}
   -DP64=$<TARGET_FILE:p64> -DCLANG=${CLANG_EXECUTABLE}
   -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/strings.pas -DCLASS_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/classes.pas -DUNIT_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/units.pas
   -DUNIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/units -DFFI_LIBRARY=$<TARGET_FILE:p64testffi> -DWORK=${CMAKE_CURRENT_BINARY_DIR}
   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/string_lifetime.cmake)
 endif()
 find_program(READELF_EXECUTABLE readelf)
 if(READELF_EXECUTABLE)
  add_test(NAME debug_info COMMAND ${CMAKE_COMMAND}
   -DP64=$<TARGET_FILE:p64> -DCLANG=${CLANG_EXECUTABLE} -DREADELF=${READELF_EXECUTABLE}
   -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/units.pas -DUNIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/units
   -DFFI_LIBRARY=$<TARGET_FILE:p64testffi> -DWORK=${CMAKE_CURRENT_BINARY_DIR}
   -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/debug_info.cmake)
 endif()
endif()
