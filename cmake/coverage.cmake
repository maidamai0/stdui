if(NOT EXISTS "${PROFILE_FILE}")
    message(FATAL_ERROR "Coverage profile was not generated: ${PROFILE_FILE}")
endif()

set(PROFILE_DATA_FILE "${REPORT_DIR}/stdui.profdata")

execute_process(
    COMMAND "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse
            "${PROFILE_FILE}" -o "${PROFILE_DATA_FILE}"
    RESULT_VARIABLE PROFDATA_RESULT)

if(NOT PROFDATA_RESULT EQUAL 0)
    message(FATAL_ERROR "llvm-profdata failed with exit code ${PROFDATA_RESULT}")
endif()

foreach(_wait RANGE 1 20)
    if(EXISTS "${PROFILE_DATA_FILE}")
        break()
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.1)
endforeach()

if(NOT EXISTS "${PROFILE_DATA_FILE}")
    message(FATAL_ERROR "llvm-profdata did not create ${PROFILE_DATA_FILE}")
endif()

set(IGNORE_REGEX "(doctest|expressions_tests.cpp|/usr/include|/Applications/Xcode)")

execute_process(
    COMMAND "${LLVM_COV_EXECUTABLE}" report "${TEST_EXECUTABLE}"
            -instr-profile "${PROFILE_DATA_FILE}"
            -ignore-filename-regex "${IGNORE_REGEX}"
    OUTPUT_VARIABLE COVERAGE_REPORT
    RESULT_VARIABLE REPORT_RESULT)

if(NOT REPORT_RESULT EQUAL 0)
    message(FATAL_ERROR "llvm-cov report failed with exit code ${REPORT_RESULT}")
endif()

message("${COVERAGE_REPORT}")

execute_process(
    COMMAND "${LLVM_COV_EXECUTABLE}" export "${TEST_EXECUTABLE}"
            -instr-profile "${PROFILE_DATA_FILE}"
            -ignore-filename-regex "${IGNORE_REGEX}"
            -format=text
    OUTPUT_VARIABLE COVERAGE_JSON
    RESULT_VARIABLE EXPORT_RESULT)

if(NOT EXPORT_RESULT EQUAL 0)
    message(FATAL_ERROR "llvm-cov export failed with exit code ${EXPORT_RESULT}")
endif()

string(JSON COVERAGE_PERCENT GET "${COVERAGE_JSON}" "data" 0 "totals" "lines" "percent")

if(NOT COVERAGE_PERCENT GREATER_EQUAL THRESHOLD)
    message(FATAL_ERROR
        "Coverage ${COVERAGE_PERCENT}% is below the required threshold of ${THRESHOLD}%")
endif()

execute_process(
    COMMAND "${LLVM_COV_EXECUTABLE}" show "${TEST_EXECUTABLE}"
            -instr-profile "${PROFILE_DATA_FILE}"
            -ignore-filename-regex "${IGNORE_REGEX}"
            -format=html
            -output-dir="${REPORT_DIR}/html"
    RESULT_VARIABLE HTML_RESULT)

if(NOT HTML_RESULT EQUAL 0)
    message(FATAL_ERROR "llvm-cov HTML generation failed with exit code ${HTML_RESULT}")
endif()

message(STATUS "Coverage ${COVERAGE_PERCENT}% meets threshold ${THRESHOLD}%")
