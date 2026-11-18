execute_process(COMMAND "${P64}" "${SOURCE}" -o "${WORK}/strings-asan" --clang "${CLANG}" --link -fsanitize=address
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "sanitized string compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${P64}" "${CMAKE_CURRENT_LIST_DIR}/exceptions.pas" -o "${WORK}/exceptions-asan" --clang "${CLANG}" --link -fsanitize=address
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "sanitized exception compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${P64}" "${CLASS_SOURCE}" -o "${WORK}/classes-asan" --clang "${CLANG}" --link -fsanitize=address
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "sanitized class compilation failed: ${output}${error}")
endif()

set(asan_detect_leaks 0)
if(DEFINED ENV{P64_ASAN_DETECT_LEAKS})
  set(asan_detect_leaks $ENV{P64_ASAN_DETECT_LEAKS})
else()
  # Best-effort auto-enable leak detection when we're not under ptrace.
  # Some sandboxed environments only ptrace the test binary, not CTest/CMake,
  # so we default to "off" for reliability.
  if(EXISTS "/proc/self/status")
    file(READ "/proc/self/status" process_status)
    string(REGEX MATCH "TracerPid:[ \t]*([0-9]+)" _match "${process_status}")
    if(CMAKE_MATCH_1 AND CMAKE_MATCH_1 STREQUAL "0")
      set(asan_detect_leaks 1)
    endif()
  endif()
endif()

set(asan_options "detect_leaks=${asan_detect_leaks}:halt_on_error=1")

execute_process(COMMAND "${CMAKE_COMMAND}" -E env "ASAN_OPTIONS=${asan_options}" "${WORK}/classes-asan"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 15)
if(NOT result EQUAL 0 OR NOT output STREQUAL "initially nil\nparent one child 1\nparent! 5\n0\nsame object\nconstructed 1\nbase! child\nchild?\nchild destroy child\nbase destroy base!\nderived own 1\nbase destroy derived own\n")
  message(FATAL_ERROR "sanitized class execution failed (${result}): ${output}${error}")
endif()
execute_process(COMMAND "${P64}" "${UNIT_SOURCE}" -o "${WORK}/units-asan" --clang "${CLANG}" --unit-path "${UNIT_PATH}" --link "${FFI_LIBRARY}" --link -fsanitize=address
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "sanitized unit compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "ASAN_OPTIONS=${asan_options}" "${WORK}/units-asan"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 15)
if(NOT result EQUAL 0 OR NOT output STREQUAL "base init\nbridge init 2\nunit loaded 7 5000000003\n5 private\nunit callback 15\nunit class 19 1\nlocal destroy 19\nunit destroy 19 private\nbridge fini 6\nbase fini private\n")
  message(FATAL_ERROR "sanitized unit execution failed (${result}): ${output}${error}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "ASAN_OPTIONS=${asan_options}" "${WORK}/strings-asan"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 15)
if(NOT result EQUAL 0 OR NOT output STREQUAL "empty\nbefore! before\nbefore!\nbefore\nbefore ✓\nxxxx\ncomparison\nempty result\ntitle item\nchanged title\nnew item!\nabcde\nconst ok\n")
  message(FATAL_ERROR "sanitized string execution failed (${result}): ${output}${error}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "ASAN_OPTIONS=${asan_options}" "${WORK}/exceptions-asan"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 15)
if(NOT result EQUAL 0 OR NOT output STREQUAL "caught before!\nafter before!\nenter finally\nin finally\ncaught2\nno raise\nfinally2\n")
  message(FATAL_ERROR "sanitized exception execution failed (${result}): ${output}${error}")
endif()
