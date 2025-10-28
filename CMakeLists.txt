cmake_minimum_required(VERSION 3.10)
project(vista VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

# Options
option(USE_SHADERS "Enable OpenGL shader support" OFF)
option(BUILD_DOCS "Build documentation with Doxygen" OFF)

# Find packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.c
    src/config.c
    src/thumbnails.c
    src/renderer.c
    src/wallpaper.c
)

# Add shader support if enabled
if(USE_SHADERS)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    list(APPEND SOURCES src/shader.c)
    add_definitions(-DUSE_SHADERS)
endif()

# Add library search paths before creating executable
link_directories(/usr/lib64)

# Create executable
add_executable(vista ${SOURCES})

# Link libraries  
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib64")
target_link_libraries(vista
    ${SDL2_LDFLAGS}
    ${SDL2_IMAGE_LDFLAGS}
    m
    stdc++
)

if(USE_SHADERS)
    find_package(OpenGL REQUIRED)
    pkg_check_modules(GLEW REQUIRED glew)
    target_link_libraries(vista
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
endif()

# Install targets
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
