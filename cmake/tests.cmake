include(ExternalProject)

ExternalProject_Add(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.5.0
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND "")
ExternalProject_Get_Property(doctest SOURCE_DIR)

file(GLOB STDUI_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../tests/*_tests.cpp")

foreach(test_source IN LISTS STDUI_TEST_SOURCES)
    get_filename_component(test_name "${test_source}" NAME_WE)
    set(target_name "stdui_${test_name}")
    add_executable("${target_name}" "${test_source}")
    add_dependencies("${target_name}" doctest)
    target_include_directories("${target_name}" PRIVATE ${SOURCE_DIR})
    target_link_libraries("${target_name}" PRIVATE stdui)
    add_test(NAME "${target_name}" COMMAND "${target_name}")
    list(APPEND STDUI_TEST_TARGETS "${target_name}")
endforeach()

if(STDUI_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "STDUI_ENABLE_COVERAGE currently requires Clang/llvm-cov")
    endif()

    find_program(LLVM_COV_EXECUTABLE llvm-cov)
    if(NOT LLVM_COV_EXECUTABLE)
        execute_process(
            COMMAND xcrun --find llvm-cov
            OUTPUT_VARIABLE LLVM_COV_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_COV_FIND_RESULT)
        if(NOT LLVM_COV_FIND_RESULT EQUAL 0)
            message(FATAL_ERROR "Unable to locate llvm-cov")
        endif()
    endif()

    find_program(LLVM_PROFDATA_EXECUTABLE llvm-profdata)
    if(NOT LLVM_PROFDATA_EXECUTABLE)
        execute_process(
            COMMAND xcrun --find llvm-profdata
            OUTPUT_VARIABLE LLVM_PROFDATA_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE LLVM_PROFDATA_FIND_RESULT)
        if(NOT LLVM_PROFDATA_FIND_RESULT EQUAL 0)
            message(FATAL_ERROR "Unable to locate llvm-profdata")
        endif()
    endif()

    foreach(target_name IN LISTS STDUI_TEST_TARGETS)
        target_compile_options("${target_name}" PRIVATE
            -fprofile-instr-generate
            -fcoverage-mapping)
        target_link_options("${target_name}" PRIVATE
            -fprofile-instr-generate
            -fcoverage-mapping)
    endforeach()

    set(STDUI_PROFILE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(STDUI_COVERAGE_TEST_EXECUTABLES)
    set(STDUI_COVERAGE_RUN_COMMANDS)

    foreach(target_name IN LISTS STDUI_TEST_TARGETS)
        list(APPEND STDUI_COVERAGE_TEST_EXECUTABLES "$<TARGET_FILE:${target_name}>")
        list(APPEND STDUI_COVERAGE_RUN_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E env
                "LLVM_PROFILE_FILE=${STDUI_PROFILE_DIR}/${target_name}.profraw"
                "$<TARGET_FILE:${target_name}>")
    endforeach()

    string(JOIN "," STDUI_COVERAGE_TEST_EXECUTABLE_STRING
        ${STDUI_COVERAGE_TEST_EXECUTABLES})

    add_custom_target(stdui_coverage
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${STDUI_PROFILE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${STDUI_PROFILE_DIR}"
        ${STDUI_COVERAGE_RUN_COMMANDS}
        COMMAND ${CMAKE_COMMAND}
            -DLLVM_COV_EXECUTABLE=${LLVM_COV_EXECUTABLE}
            -DLLVM_PROFDATA_EXECUTABLE=${LLVM_PROFDATA_EXECUTABLE}
            -DTEST_EXECUTABLES_STRING=${STDUI_COVERAGE_TEST_EXECUTABLE_STRING}
            -DPROFILE_DIR=${STDUI_PROFILE_DIR}
            -DREPORT_DIR=${STDUI_PROFILE_DIR}
            -DTHRESHOLD=${STDUI_COVERAGE_THRESHOLD}
            -P ${CMAKE_CURRENT_LIST_DIR}/coverage.cmake
        DEPENDS ${STDUI_TEST_TARGETS}
        USES_TERMINAL
        COMMENT "Running instrumented tests and generating stdui coverage")
endif()
