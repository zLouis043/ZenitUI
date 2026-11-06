if(NOT DEFINED TEST_EXECUTABLE OR NOT DEFINED TEST_OUTPUT_FILE)
    message(FATAL_ERROR "TEST_EXECUTABLE e TEST_OUTPUT_FILE are mandatory")
endif()

execute_process(
    COMMAND "${TEST_EXECUTABLE}" --list
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE  _stderr
    RESULT_VARIABLE _rc
)

if(NOT _rc EQUAL 0)
    message(FATAL_ERROR
        "ZenitTests --list has failed (rc=${_rc}):\n${_stderr}")
endif()

string(REPLACE "\r\n" "\n" _stdout "${_stdout}")
string(REPLACE "\n" ";"     _lines  "${_stdout}")

set(_script "# File auto-generated from GenerateZenitTests.cmake - do not modify.\n")
foreach(_t IN LISTS _lines)
    string(STRIP "${_t}" _t)
    if(_t STREQUAL "")
        continue()
    endif()
    string(APPEND _script
    "add_test(\"${_t}\" \"${TEST_EXECUTABLE}\" --filter=${_t})\n")
endforeach()

file(WRITE "${TEST_OUTPUT_FILE}" "${_script}")