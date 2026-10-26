cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # used by clang-tidy

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(LMS_BUILD_TESTS "Build the automated tests" ON)

# ---- Libraries from outside (Layer 4): SQLite and libsodium
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# ---- The schema file is built into the program, so the program has one file less to install.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql" SCHEMA_CONTENT)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/data/Schema.cpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/generated/Schema.cpp" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql")

# ---- Warnings, as the coding standard asks (CODING_STANDARD.md, section 7)
add_library(lms_warnings INTERFACE)
if(MSVC)
    target_compile_options(lms_warnings INTERFACE /W4)
else()
    target_compile_options(lms_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# ---- Layer 2: business logic. It has no include path to SQLite or libsodium (rule AR-2).
file(GLOB BUSINESS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/business/*.cpp")
add_library(lms_business STATIC ${BUSINESS_SOURCES})
target_include_directories(lms_business PUBLIC src/business)
target_link_libraries(lms_business PRIVATE lms_warnings)

# ---- Layer 3: data access and infrastructure
file(GLOB DATA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/data/*.cpp")
add_library(lms_data STATIC ${DATA_SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/generated/Schema.cpp")
target_include_directories(lms_data PUBLIC src/data)
target_link_libraries(lms_data PUBLIC lms_business SQLite::SQLite3 PkgConfig::SODIUM)
target_link_libraries(lms_data PRIVATE lms_warnings)

# ---- Layer 1: menus. They see only Layer 2 (rule AR-1).
file(GLOB UI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp")
list(REMOVE_ITEM UI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Application.cpp")
add_library(lms_ui STATIC ${UI_SOURCES})
target_include_directories(lms_ui PUBLIC src/ui)
target_link_libraries(lms_ui PUBLIC lms_business)
target_link_libraries(lms_ui PRIVATE lms_warnings)

# ---- The program. Application (the composition root) is the one file that sees all layers.
add_executable(lms src/main.cpp src/ui/Application.cpp)
target_link_libraries(lms PRIVATE lms_ui lms_data lms_warnings)

install(TARGETS lms RUNTIME DESTINATION bin)

# ---- Automated tests
if(LMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
