cmake_minimum_required(VERSION 3.10)
project(MetaInstaller)

# Read version from .env.version file
if(EXISTS "${CMAKE_SOURCE_DIR}/.env.version")
    file(READ "${CMAKE_SOURCE_DIR}/.env.version" ENV_VERSION_CONTENT)
    string(REGEX MATCH "VERSION=([^\n]+)" _ ${ENV_VERSION_CONTENT})
    if(CMAKE_MATCH_1)
        set(APP_VERSION ${CMAKE_MATCH_1})
        add_definitions(-DAPP_VERSION="${APP_VERSION}")
        message(STATUS "APP_VERSION defined as: \"${APP_VERSION}\"")
    else()
        message(WARNING "VERSION key not found in .env.version file")
        add_definitions(-DAPP_VERSION="unknown")
    endif()
else()
    message(WARNING ".env.version file not found")
    add_definitions(-DAPP_VERSION="unknown")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

# set(CMAKE_VERBOSE_MAKEFILE ON)

include(cmake/EmbedResources.cmake)

# Define paths to pre-downloaded dependencies
set(ASIO_DIR ${CMAKE_SOURCE_DIR}/asio)
set(CROW_DIR ${CMAKE_SOURCE_DIR}/Crow)


# # SQLite3 compile definitions to disable dynamic loading
# add_compile_definitions(
#     SQLITE_OMIT_LOAD_EXTENSION=1
#     SQLITE_OMIT_SHARED_CACHE=1
#     SQLITE_THREADSAFE=1
#     SQLITE_DQS=0
# )

# # httplib.h compile definitions for static linking
# add_compile_definitions(
#     CPPHTTPLIB_THREAD_POOL_COUNT=1
#     CPPHTTPLIB_KEEPALIVE_MAX_COUNT=1
# )
# Set ASIO include directory for Crow to find
set(ASIO_INCLUDE_DIR ${ASIO_DIR})

# Include Crow's CMake configuration
add_subdirectory(${CROW_DIR} ${CMAKE_BINARY_DIR}/Crow)

add_library(sqlite3_static STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite/sqlite3.c
)

target_link_options(
    sqlite3_static PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
)

target_include_directories(sqlite3_static PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite
)

# Disable features that require libdl
target_compile_definitions(sqlite3_static PRIVATE
    SQLITE_OMIT_LOAD_EXTENSION      # Disables dlopen() usage
    SQLITE_ENABLE_FTS5              # Full-text search
    SQLITE_ENABLE_JSON1             # JSON functions
    SQLITE_ENABLE_RTREE             # R-tree index
    SQLITE_THREADSAFE=1             # Thread safety
    SQLITE_DEFAULT_MEMSTATUS=0      # Reduce memory overhead
)



add_executable(
    ${PROJECT_NAME}
    src/main.cpp
    src/resourceextractor.cpp
    src/ProcessManager.cpp
    src/utils.cpp
    src/DockerManager.cpp
    src/ProjectManager.cpp
    src/FileManager.cpp
    src/json11.cpp
    src/dotenv.cpp
    src/MarkdownToHtml.cpp
    src/EnvConfig.cpp
    src/x_detector.cpp
    src/BrowserManager.cpp
    src/test.cpp
    src/argument_handler.cpp
    src/help_global.cpp
    # sqlite/sqlite3.c
    src/MetaDatabase.cpp
    src/SELinuxManager.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/Crow/include
    ${CMAKE_SOURCE_DIR}/Crow/include/crow
    ${CMAKE_SOURCE_DIR}/asio/asio/include
    ${CMAKE_SOURCE_DIR}/asio/asio/include/asio
    ${CMAKE_SOURCE_DIR}/fkyaml
    ${CMAKE_SOURCE_DIR}/sqlite
)

# Include ASIO headers (optional, but kept for consistency)

file(GLOB RCS_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "resources/*")
message(STATUS "rcs list = ${RCS_LIST},\nProject name = ${PROJECT_NAME}")
embed_resources(${PROJECT_NAME} ${RCS_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE ${ASIO_DIR})
# Link against Crow
# target_link_libraries(myapp PRIVATE crow)



# target_link_options(${PROJECT_NAME} PRIVATE -static)
target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
# set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")

find_package(Threads REQUIRED)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE Threads::Threads
    PRIVATE sqlite3_static
    # PRIVATE dl
)

# Create a custom target to symlink compile_commands.json from build directory to project root
# This will run every time CMake configure is executed
add_custom_target(
    symlink_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Creating symlink for compile_commands.json"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Symlinking compile_commands.json to project root"
    VERBATIM
    DEPENDS ${PROJECT_NAME}
)
