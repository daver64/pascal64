foreach(source IN ITEMS "${PROBE_SOURCE}" "${GTK_SOURCE}")
  get_filename_component(name "${source}" NAME_WE)
  execute_process(COMMAND "${P64}" "${source}" -o "${WORK}/${name}-test" --clang "${CLANG}"
                  --unit-path "${UNIT_PATH}" --link "${GTK_LIBRARY}" --link "${GOBJECT_LIBRARY}"
                  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "GTK compilation failed: ${output}${error}")
  endif()
endforeach()
execute_process(COMMAND "${WORK}/gtk_probe-test" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
if(NOT result EQUAL 0 OR NOT output STREQUAL "3\n")
  message(FATAL_ERROR "unexpected GTK version (${result}): ${output}${error}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY GDK_BACKEND=x11 "${WORK}/gtk_hello-test"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 5)
if(NOT result EQUAL 0 OR NOT output STREQUAL "GTK could not open a display\n")
  message(FATAL_ERROR "unexpected headless GTK output (${result}): ${output}${error}")
endif()
