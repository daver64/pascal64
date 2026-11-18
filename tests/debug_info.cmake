execute_process(COMMAND "${P64}" "${SOURCE}" -o "${WORK}/debug-test" --clang "${CLANG}" --debug
                --unit-path "${UNIT_PATH}" --link "${FFI_LIBRARY}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "debug compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${READELF}" --debug-dump=decodedline "${WORK}/debug-test"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "units.pas" OR NOT output MATCHES "BaseUnit.pas" OR NOT output MATCHES "BridgeUnit.pas")
  message(FATAL_ERROR "missing source line information: ${output}${error}")
endif()
execute_process(COMMAND "${READELF}" --debug-dump=info "${WORK}/debug-test"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "secret_text" OR NOT output MATCHES "DW_TAG_formal_parameter" OR NOT output MATCHES "amount" OR NOT output MATCHES "DW_TAG_variable" OR NOT output MATCHES "unit_count" OR NOT output MATCHES "hidden" OR NOT output MATCHES "String" OR NOT output MATCHES "references" OR NOT output MATCHES "length" OR NOT output MATCHES "bytes" OR NOT output MATCHES "debug_payload" OR NOT output MATCHES "DW_TAG_structure_type" OR NOT output MATCHES "DW_TAG_array_type" OR NOT output MATCHES "label_text")
  message(FATAL_ERROR "missing routine debug information: ${output}${error}")
endif()

file(WRITE "${WORK}/real_debug.pas" "program real_debug; var x: Real; begin x := 1.25; writeln(x) end.")
execute_process(COMMAND "${P64}" "${WORK}/real_debug.pas" -o "${WORK}/real-debug" --clang "${CLANG}" --debug
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "real debug compilation failed: ${output}${error}")
endif()
execute_process(COMMAND "${READELF}" --debug-dump=info "${WORK}/real-debug"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT output MATCHES "Real")
  message(FATAL_ERROR "missing Real debug type: ${output}${error}")
endif()
