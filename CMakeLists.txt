cmake_minimum_required(VERSION 3.14)
project(dylib)

set(CMAKE_VERBOSE_MAKEFILE off)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

option(WITH_TESTS "Enable testing" ON)

if(WITH_TESTS)
include(import/FetchGoogleTest.cmake)

if (UNIX)
    add_compile_options(-Wall -Wextra -Weffc++)
else ()
    add_compile_options(/W4)
endif()

add_library(dynlib SHARED test/myDynLib.cpp)
target_include_directories(dynlib PRIVATE .)
set_target_properties(dynlib PROPERTIES PREFIX "")

if (UNIX AND NOT APPLE)
    add_compile_options(--coverage)
endif()

enable_testing()

add_executable(unit_tests test/tests.cpp)

target_include_directories(unit_tests PRIVATE .)

if (UNIX AND NOT APPLE)
    target_link_libraries(unit_tests PRIVATE
        gcov
    )
endif()

if (UNIX)
    target_link_libraries(unit_tests PRIVATE
        dl
        pthread
    )
endif()

target_link_libraries(unit_tests PRIVATE
    gtest_main
)

include(GoogleTest)

gtest_discover_tests(unit_tests)

add_dependencies(unit_tests dynlib)
endif(WITH_TESTS)

set(CPACK_PACKAGE_NAME "dylib")
set(CPACK_PACKAGE_VENDOR "Martin Olivier")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "7")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "Cross-platform Dynamic Library Loader for C++")
set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")

include(CPackComponent)
include(CMakePackageConfigHelpers)

set(PackagingTemplatesDir "${CMAKE_CURRENT_SOURCE_DIR}/packaging")

set(LIB_NAME "dylib")
string(TOUPPER "${LIB_NAME}" LIB_NAME_UPPER)
set(CMAKE_EXPORT_NAME "${LIB_NAME}")
set(PKG_CONFIG_NAME "${LIB_NAME}")

cpack_add_component(dev
	DISPLAY_NAME "${LIB_NAME} header"
	DESCRIPTION "The header needed for using ${LIB_NAME} in own software"
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${LIB_NAME}.hpp"
	TYPE INCLUDE
	COMPONENT "dev"
)

set(PKG_CONFIG_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${PKG_CONFIG_NAME}.pc")
configure_file("${PackagingTemplatesDir}/pkgconfig.pc.in" "${PKG_CONFIG_FILE_NAME}" @ONLY)
install(FILES "${PKG_CONFIG_FILE_NAME}"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
	COMPONENT "dev"
)

set(CMAKE_CONFIG_FILE_BASENAME "${CMAKE_EXPORT_NAME}Config.cmake")
set(CMAKE_CONFIG_VERSION_FILE_BASENAME "${CMAKE_EXPORT_NAME}ConfigVersion.cmake")
set(CMAKE_CONFIG_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIG_FILE_BASENAME}")
configure_package_config_file("${PackagingTemplatesDir}/CMakeConfig.cmake.in" "${CMAKE_CONFIG_FILE_NAME}"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake"
)


set(CMAKE_CONFIG_VERSION_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIG_VERSION_FILE_BASENAME}")
write_basic_package_version_file(
	"${CMAKE_CONFIG_VERSION_FILE_NAME}"
	VERSION "100500.100500.100500"  # any version of same bitness suits. CMake cannot compare to infinity, so use a large number we expect to be greater than any future version
	COMPATIBILITY AnyNewerVersion
)

install(FILES "${CMAKE_CONFIG_FILE_NAME}" "${CMAKE_CONFIG_VERSION_FILE_NAME}"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_EXPORT_NAME}"
	COMPONENT "dev"
)

set("CPACK_DEBIAN_DEV_PACKAGE_NAME" "lib${LIB_NAME}-dev")
set("CPACK_DEBIAN_DEV_PACKAGE_DEPENDS" "libc6-dev")
set("CPACK_DEBIAN_DEV_PACKAGE_SUGGESTS" "cmake, pkg-config, pkg-conf")

set("CPACK_RPM_DEV_PACKAGE_NAME" "lib${LIB_NAME}-devel")
set("CPACK_RPM_DEV_PACKAGE_SUGGESTS" "${CPACK_DEBIAN_DEV_PACKAGE_SUGGESTS}")

set(CPACK_COMPONENTS_ALL "dev")

set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/tocola/dylib")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_MAINTAINER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_NSIS_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

include(CPack)
