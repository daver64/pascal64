execute_process(COMMAND "${P64}" "${SOURCE}" -o "${WORK}/gtk-mock-test" --clang "${CLANG}"
                --unit-path "${UNIT_PATH}" --link "${GTK_LIBRARY}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "mock GTK compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${WORK}/gtk-mock-test" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
if(NOT result EQUAL 0 OR NOT output STREQUAL "Button clicked 1 times\nButton clicked 2 times\n")
  message(FATAL_ERROR "mock GTK execution failed (${result}): ${output}${error}")
endif()
