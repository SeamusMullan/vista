cmake_minimum_required(VERSION 3.10)
project(vista VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

# Platform-specific library search paths and linker flags
if(APPLE)
    # macOS: Homebrew installs here
    link_directories(/opt/homebrew/lib)
    set(CMAKE_MACOSX_RPATH TRUE)
elseif(UNIX)
    # Linux: typical 64-bit lib dir
    link_directories(/usr/lib64)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib64")
elseif(WIN32)
    # Windows: pkg-config or find_package handles this
endif()

# RPATH handling (portable)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Options
option(USE_SHADERS "Enable OpenGL shader support" OFF)
option(BUILD_DOCS "Build documentation with Doxygen" ON)

# Dependencies
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

# Sources
set(SOURCES
    src/main.c
    src/config.c
    src/thumbnails.c
    src/renderer.c
    src/wallpaper.c
    src/roulette/roulette.c
)

# Optional shader support
if(USE_SHADERS)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    list(APPEND SOURCES src/shader.c)
    add_definitions(-DUSE_SHADERS)
endif()

# Executable
add_executable(vista ${SOURCES})

# Link libraries (portable)
target_link_libraries(vista
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    OpenSSL::Crypto
    m
)

if(USE_SHADERS)
    target_link_libraries(vista
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
endif()

# Install rules
install(TARGETS vista DESTINATION bin)
install(FILES config/vista.conf.example DESTINATION share/vista)
install(DIRECTORY shaders DESTINATION share/vista)

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${CMAKE_SOURCE_DIR}/docs/generate_docs.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating documentation with Doxygen"
        )
    endif()
endif()

# Uninstall target
add_custom_target(uninstall
    COMMAND xargs rm -f < install_manifest.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

