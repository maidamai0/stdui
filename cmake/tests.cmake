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

add_executable(stdui_expression_tests
    ${CMAKE_CURRENT_LIST_DIR}/../tests/expressions_tests.cpp)
add_dependencies(stdui_expression_tests doctest)
target_include_directories(stdui_expression_tests PRIVATE ${SOURCE_DIR})
target_link_libraries(stdui_expression_tests PRIVATE stdui)
add_test(NAME stdui_expression_tests COMMAND stdui_expression_tests)

add_executable(stdui_geometry_tests
    ${CMAKE_CURRENT_LIST_DIR}/../tests/geometry_tests.cpp)
add_dependencies(stdui_geometry_tests doctest)
target_include_directories(stdui_geometry_tests PRIVATE ${SOURCE_DIR})
target_link_libraries(stdui_geometry_tests PRIVATE stdui)
add_test(NAME stdui_geometry_tests COMMAND stdui_geometry_tests)

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

    target_compile_options(stdui_expression_tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping)
    target_link_options(stdui_expression_tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping)

    set(STDUI_PROFILE_FILE "${CMAKE_BINARY_DIR}/coverage/stdui_expression_tests.profraw")

    add_custom_target(stdui_coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/coverage"
        COMMAND ${CMAKE_COMMAND} -E env
            "LLVM_PROFILE_FILE=${STDUI_PROFILE_FILE}"
            "$<TARGET_FILE:stdui_expression_tests>"
        COMMAND ${CMAKE_COMMAND}
            -DLLVM_COV_EXECUTABLE=${LLVM_COV_EXECUTABLE}
            -DLLVM_PROFDATA_EXECUTABLE=${LLVM_PROFDATA_EXECUTABLE}
            -DTEST_EXECUTABLE=$<TARGET_FILE:stdui_expression_tests>
            -DPROFILE_FILE=${STDUI_PROFILE_FILE}
            -DREPORT_DIR=${CMAKE_BINARY_DIR}/coverage
            -DTHRESHOLD=${STDUI_COVERAGE_THRESHOLD}
            -P ${CMAKE_CURRENT_LIST_DIR}/coverage.cmake
        DEPENDS stdui_expression_tests
        USES_TERMINAL
        COMMENT "Running instrumented tests and generating stdui coverage")
endif()
