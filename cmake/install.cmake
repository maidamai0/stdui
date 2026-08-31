include(CMakePackageConfigHelpers)

set(STDUI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/stdui"
    CACHE STRING "Installation directory for the stdui CMake package")

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/stduiConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/stduiConfig.cmake
    INSTALL_DESTINATION ${STDUI_INSTALL_CMAKEDIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/stduiConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(TARGETS stdui EXPORT stduiTargets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT stduiTargets
    FILE stduiTargets.cmake
    NAMESPACE stdui::
    DESTINATION ${STDUI_INSTALL_CMAKEDIR})
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stduiConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/stduiConfigVersion.cmake
    DESTINATION ${STDUI_INSTALL_CMAKEDIR})
