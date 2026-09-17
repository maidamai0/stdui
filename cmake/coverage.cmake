string(REPLACE "," ";" TEST_EXECUTABLES "${TEST_EXECUTABLES_STRING}")

file(GLOB RAW_PROFILES "${PROFILE_DIR}/*.profraw")

if(NOT RAW_PROFILES)
    message(FATAL_ERROR "No coverage profiles found in ${PROFILE_DIR}")
endif()

set(IGNORE_REGEX "(doctest|.*_tests.cpp|/usr/include|/Applications/Xcode)")
set(STDUI_COVERAGE_FILE_KEYS)

foreach(test_executable IN LISTS TEST_EXECUTABLES)
    get_filename_component(test_name "${test_executable}" NAME)
    file(GLOB TEST_RAW_PROFILES "${PROFILE_DIR}/${test_name}*.profraw")

    if(NOT TEST_RAW_PROFILES)
        message(FATAL_ERROR "No coverage profile found for ${test_name}")
    endif()

    set(TEST_PROFILE_DATA_FILE "${REPORT_DIR}/${test_name}.profdata")

    execute_process(
        COMMAND "${LLVM_PROFDATA_EXECUTABLE}" merge -sparse
                ${TEST_RAW_PROFILES} -o "${TEST_PROFILE_DATA_FILE}"
        RESULT_VARIABLE PROFDATA_RESULT)

    if(NOT PROFDATA_RESULT EQUAL 0)
        message(FATAL_ERROR
            "llvm-profdata failed for ${test_name} with exit code ${PROFDATA_RESULT}")
    endif()

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" report "${test_executable}"
                -instr-profile "${TEST_PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
        OUTPUT_VARIABLE COVERAGE_REPORT
        RESULT_VARIABLE REPORT_RESULT)

    if(NOT REPORT_RESULT EQUAL 0)
        message(FATAL_ERROR "llvm-cov report failed for ${test_name}: ${REPORT_RESULT}")
    endif()

    message("${COVERAGE_REPORT}")

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" export "${test_executable}"
                -instr-profile "${TEST_PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
                -format=text
        OUTPUT_VARIABLE COVERAGE_JSON
        RESULT_VARIABLE EXPORT_RESULT)

    if(NOT EXPORT_RESULT EQUAL 0)
        message(FATAL_ERROR "llvm-cov export failed for ${test_name}: ${EXPORT_RESULT}")
    endif()

    string(JSON FILES GET "${COVERAGE_JSON}" "data" 0 "files")
    string(JSON FILE_COUNT LENGTH "${FILES}")
    set(FILE_INDEX 0)

    while(FILE_INDEX LESS FILE_COUNT)
        string(JSON FILE_NAME GET "${FILES}" ${FILE_INDEX} "filename")
        string(JSON FILE_LINE_COUNT GET "${FILES}" ${FILE_INDEX}
            "summary" "lines" "count")
        string(JSON FILE_LINE_COVERED GET "${FILES}" ${FILE_INDEX}
            "summary" "lines" "covered")
        string(MD5 FILE_KEY "${FILE_NAME}")

        set(FILE_NAME_VAR "STDUI_COVERAGE_${FILE_KEY}_NAME")
        set(FILE_COUNT_VAR "STDUI_COVERAGE_${FILE_KEY}_COUNT")
        set(FILE_COVERED_VAR "STDUI_COVERAGE_${FILE_KEY}_COVERED")

        if(NOT DEFINED ${FILE_COUNT_VAR})
            list(APPEND STDUI_COVERAGE_FILE_KEYS "${FILE_KEY}")
            set(${FILE_NAME_VAR} "${FILE_NAME}")
            set(${FILE_COUNT_VAR} 0)
            set(${FILE_COVERED_VAR} 0)
        endif()

        if(FILE_LINE_COUNT GREATER ${${FILE_COUNT_VAR}})
            set(${FILE_COUNT_VAR} ${FILE_LINE_COUNT})
            set(${FILE_COVERED_VAR} ${FILE_LINE_COVERED})
        elseif(FILE_LINE_COUNT EQUAL ${${FILE_COUNT_VAR}}
               AND FILE_LINE_COVERED GREATER ${${FILE_COVERED_VAR}})
            set(${FILE_COVERED_VAR} ${FILE_LINE_COVERED})
        endif()

        math(EXPR FILE_INDEX "${FILE_INDEX} + 1")
    endwhile()

    execute_process(
        COMMAND "${LLVM_COV_EXECUTABLE}" show "${test_executable}"
                -instr-profile "${TEST_PROFILE_DATA_FILE}"
                -ignore-filename-regex "${IGNORE_REGEX}"
                -format=html
                -output-dir="${REPORT_DIR}/html/${test_name}"
        RESULT_VARIABLE HTML_RESULT)

    if(NOT HTML_RESULT EQUAL 0)
        message(FATAL_ERROR
            "llvm-cov HTML generation failed for ${test_name}: ${HTML_RESULT}")
    endif()
endforeach()

set(TOTAL_LINES 0)
set(TOTAL_COVERED 0)

foreach(FILE_KEY IN LISTS STDUI_COVERAGE_FILE_KEYS)
    set(FILE_NAME_VAR "STDUI_COVERAGE_${FILE_KEY}_NAME")
    set(FILE_COUNT_VAR "STDUI_COVERAGE_${FILE_KEY}_COUNT")
    set(FILE_COVERED_VAR "STDUI_COVERAGE_${FILE_KEY}_COVERED")
    set(FILE_LINE_COUNT ${${FILE_COUNT_VAR}})
    set(FILE_LINE_COVERED ${${FILE_COVERED_VAR}})
    get_filename_component(FILE_BASENAME "${${FILE_NAME_VAR}}" NAME)

    if(FILE_LINE_COUNT GREATER 0)
        math(EXPR FILE_LINE_PERCENT
            "100 * ${FILE_LINE_COVERED} / ${FILE_LINE_COUNT}")

        if(FILE_LINE_PERCENT LESS THRESHOLD)
            message(FATAL_ERROR
                "${FILE_BASENAME} line coverage ${FILE_LINE_PERCENT}% is below ${THRESHOLD}%")
        endif()

        math(EXPR TOTAL_LINES "${TOTAL_LINES} + ${FILE_LINE_COUNT}")
        math(EXPR TOTAL_COVERED "${TOTAL_COVERED} + ${FILE_LINE_COVERED}")
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
