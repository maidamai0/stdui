string(REPLACE "," ";" TEST_EXECUTABLES "${TEST_EXECUTABLES_STRING}")

file(GLOB RAW_PROFILES "${PROFILE_DIR}/*.profraw")

if(NOT RAW_PROFILES)
    message(FATAL_ERROR "No coverage profiles found in ${PROFILE_DIR}")
endif()

set(PROFILE_DATA_FILE "${REPORT_DIR}/stdui.profdata")

execute_process(
    COMMAND "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse
            ${RAW_PROFILES} -o "${PROFILE_DATA_FILE}"
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

set(IGNORE_REGEX "(doctest|.*_tests.cpp|/usr/include|/Applications/Xcode)")
set(TOTAL_LINES 0)
set(TOTAL_COVERED 0)

foreach(test_executable IN LISTS TEST_EXECUTABLES)
    get_filename_component(test_name "${test_executable}" NAME)

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" report "${test_executable}"
                -instr-profile "${PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
        OUTPUT_VARIABLE COVERAGE_REPORT
        RESULT_VARIABLE REPORT_RESULT)

    if(NOT REPORT_RESULT EQUAL 0)
        message(FATAL_ERROR "llvm-cov report failed for ${test_name}: ${REPORT_RESULT}")
    endif()

    message("${COVERAGE_REPORT}")

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" export "${test_executable}"
                -instr-profile "${PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
                -format=text
        OUTPUT_VARIABLE COVERAGE_JSON
        RESULT_VARIABLE EXPORT_RESULT)

    if(NOT EXPORT_RESULT EQUAL 0)
        message(FATAL_ERROR "llvm-cov export failed for ${test_name}: ${EXPORT_RESULT}")
    endif()

    string(JSON FILE_LINES GET "${COVERAGE_JSON}" "data" 0 "totals" "lines" "count")
    string(JSON FILE_COVERED GET "${COVERAGE_JSON}" "data" 0 "totals" "lines" "covered")
    math(EXPR TOTAL_LINES "${TOTAL_LINES} + ${FILE_LINES}")
    math(EXPR TOTAL_COVERED "${TOTAL_COVERED} + ${FILE_COVERED}")

    string(JSON FILES GET "${COVERAGE_JSON}" "data" 0 "files")
    string(JSON FILE_COUNT LENGTH "${FILES}")
    set(FILE_INDEX 0)

    while(FILE_INDEX LESS FILE_COUNT)
        string(JSON FILE_NAME GET "${FILES}" ${FILE_INDEX} "filename")
        get_filename_component(FILE_BASENAME "${FILE_NAME}" NAME)

        string(JSON FILE_LINE_COUNT GET "${FILES}" ${FILE_INDEX}
            "summary" "lines" "count")
        string(JSON FILE_LINE_COVERED GET "${FILES}" ${FILE_INDEX}
            "summary" "lines" "covered")
        math(EXPR FILE_LINE_PERCENT
            "100 * ${FILE_LINE_COVERED} / ${FILE_LINE_COUNT}")

        if(FILE_LINE_PERCENT LESS THRESHOLD)
            message(FATAL_ERROR
                "${FILE_BASENAME} line coverage ${FILE_LINE_PERCENT}% is below ${THRESHOLD}%")
        endif()

        math(EXPR FILE_INDEX "${FILE_INDEX} + 1")
    endwhile()

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" show "${test_executable}"
                -instr-profile "${PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
                -format=html
                -output-dir="${REPORT_DIR}/html/${test_name}"
        RESULT_VARIABLE HTML_RESULT)

    if(NOT HTML_RESULT EQUAL 0)
        message(FATAL_ERROR "llvm-cov HTML generation failed for ${test_name}: ${HTML_RESULT}")
    endif()
endforeach()

if(TOTAL_LINES EQUAL 0)
    message(FATAL_ERROR "No instrumented source lines were reported")
endif()

math(EXPR COVERAGE_PERCENT "100 * ${TOTAL_COVERED} / ${TOTAL_LINES}")

if(NOT COVERAGE_PERCENT GREATER_EQUAL THRESHOLD)
    message(FATAL_ERROR
        "Coverage ${COVERAGE_PERCENT}% is below the required threshold of ${THRESHOLD}%")
endif()

message(STATUS "Coverage ${COVERAGE_PERCENT}% meets threshold ${THRESHOLD}%")
