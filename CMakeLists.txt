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
    # Note: include_directories for Homebrew added later to avoid conflicts with submodules
    set(CMAKE_MACOSX_RPATH TRUE)
elseif(UNIX)
    # Linux: typical 64-bit lib dir
    link_directories(/usr/lib64)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/usr/lib64")
elseif(WIN32)
    # Windows: pkg-config or find_package handles this
endif()

# RPATH handling - FIXED for proper library resolution
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set RPATH to look for libraries in ../lib relative to the binary
# This works for both $HOME/.local and /usr/local installations
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")

# Options
option(USE_SHADERS "Enable OpenGL shader support" OFF)
option(BUILD_DOCS "Build documentation with Doxygen" ON)
option(USE_SYSTEM_SDL "Use system SDL3 libraries instead of submodules" OFF)

# Dependencies
find_package(OpenSSL REQUIRED)

if(USE_SYSTEM_SDL)
    # Use system-installed SDL3 libraries
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED sdl3)
    pkg_check_modules(SDL3_IMAGE sdl3-image)
    pkg_check_modules(SDL3_MIXER sdl3-mixer)
    pkg_check_modules(SDL3_TTF sdl3-ttf)

    set(SDL3_LIBRARIES_TO_LINK ${SDL3_LIBRARIES})
    set(SDL3_INCLUDE_DIRS_TO_USE ${SDL3_INCLUDE_DIRS})
else()
    # Build SDL3 from submodules
    message(STATUS "Building SDL3 from submodules")

    # Build SDL3 core
    set(SDL_SHARED ON CACHE BOOL "Build SDL3 as shared library")
    set(SDL_STATIC OFF CACHE BOOL "Don't build SDL3 as static library")
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Don't build SDL3 test library")
    add_subdirectory(submodules/SDL)

    # Build SDL3_image
    set(SDL3IMAGE_INSTALL OFF CACHE BOOL "Don't install SDL3_image")
    set(SDL3IMAGE_VENDORED ON CACHE BOOL "Use vendored dependencies")
    set(SDL3IMAGE_DEPS_SHARED OFF CACHE BOOL "Build dependencies as static")
    add_subdirectory(submodules/SDL_image)

    # Build SDL3_mixer (temporarily disabled due to build issues)
    # set(SDL3MIXER_INSTALL OFF CACHE BOOL "Don't install SDL3_mixer")
    # set(SDL3MIXER_VENDORED ON CACHE BOOL "Use vendored dependencies")
    # set(SDL3MIXER_VORBIS VORBIS_STB CACHE STRING "Use stb_vorbis instead of system vorbis")
    # set(SDL3MIXER_FLAC FLAC_DRFLAC CACHE STRING "Use dr_flac instead of system FLAC")
    # set(SDL3MIXER_MP3 MP3_DRMP3 CACHE STRING "Use dr_mp3 instead of system mpg123")
    # set(SDL3MIXER_DEPS_SHARED OFF CACHE BOOL "Build dependencies as static")
    # add_subdirectory(submodules/SDL_mixer)

    # Build SDL3_ttf
    set(SDL3TTF_INSTALL OFF CACHE BOOL "Don't install SDL3_ttf")
    set(SDL3TTF_VENDORED ON CACHE BOOL "Use vendored dependencies")
    add_subdirectory(submodules/SDL_ttf)

    # Set up include directories and libraries
    set(SDL3_LIBRARIES_TO_LINK SDL3::SDL3)
    set(SDL3_INCLUDE_DIRS_TO_USE
        ${CMAKE_SOURCE_DIR}/submodules/SDL/include
        ${CMAKE_BINARY_DIR}/submodules/SDL/include
    )

    # Always enable SDL_image and SDL_ttf when building from source
    # Note: SDL_mixer temporarily disabled due to build issues
    set(SDL3_IMAGE_FOUND TRUE)
    set(SDL3_MIXER_FOUND FALSE)
    set(SDL3_TTF_FOUND TRUE)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_INCLUDE_DIRS_TO_USE}
)

if(SDL3_IMAGE_FOUND)
    if(NOT USE_SYSTEM_SDL)
        include_directories(${CMAKE_SOURCE_DIR}/submodules/SDL_image/include)
    else()
        include_directories(${SDL3_IMAGE_INCLUDE_DIRS})
    endif()
    add_definitions(-DHAVE_SDL_IMAGE)
    message(STATUS "SDL3_image enabled")
else()
    message(WARNING "SDL3_image not found - some image formats may not be supported")
endif()

if(SDL3_MIXER_FOUND)
    if(NOT USE_SYSTEM_SDL)
        include_directories(${CMAKE_SOURCE_DIR}/submodules/SDL_mixer/include)
    else()
        include_directories(${SDL3_MIXER_INCLUDE_DIRS})
    endif()
    add_definitions(-DHAVE_SDL_MIXER)
    message(STATUS "SDL3_mixer enabled")
else()
    message(STATUS "SDL3_mixer not found - building without audio")
endif()

if(SDL3_TTF_FOUND)
    if(NOT USE_SYSTEM_SDL)
        include_directories(${CMAKE_SOURCE_DIR}/submodules/SDL_ttf/include)
    else()
        include_directories(${SDL3_TTF_INCLUDE_DIRS})
    endif()
    add_definitions(-DHAVE_SDL_TTF)
    message(STATUS "SDL3_ttf enabled")
endif()

# Sources
set(SOURCES
    src/main.c
    src/config.c
    src/thumbnails.c
    src/renderer.c
    src/wallpaper.c
    src/roulette/roulette.c
    src/roulette/sound.c
)

# Optional shader support
if(USE_SHADERS)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    list(APPEND SOURCES src/shader.c)
    add_definitions(-DUSE_SHADERS)

    # Define shader installation path - use /usr/local on macOS to avoid SIP restrictions
    if(APPLE)
        add_definitions(-DSHADER_INSTALL_DIR="/usr/local/share/vista/shaders")
    else()
        add_definitions(-DSHADER_INSTALL_DIR="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/vista/shaders")
    endif()

    # Handle both old-style GLEW_LIBRARIES and modern GLEW::GLEW target
    if(NOT TARGET GLEW::GLEW AND GLEW_LIBRARIES)
        add_library(GLEW::GLEW UNKNOWN IMPORTED)
        set_target_properties(GLEW::GLEW PROPERTIES
            IMPORTED_LOCATION "${GLEW_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}"
        )
    endif()
endif()

# Executable
add_executable(vista ${SOURCES})

# Link libraries (portable)
if(USE_SYSTEM_SDL)
    target_link_libraries(vista
        ${SDL3_LIBRARIES}
        OpenSSL::Crypto
        m
    )

    if(SDL3_IMAGE_FOUND)
        target_link_libraries(vista ${SDL3_IMAGE_LIBRARIES})
    endif()

    if(SDL3_MIXER_FOUND)
        target_link_libraries(vista ${SDL3_MIXER_LIBRARIES})
    endif()

    if(SDL3_TTF_FOUND)
        target_link_libraries(vista ${SDL3_TTF_LIBRARIES})
    endif()
else()
    # Link against submodule-built libraries
    target_link_libraries(vista
        SDL3::SDL3
        OpenSSL::Crypto
        m
    )

    if(SDL3_IMAGE_FOUND)
        target_link_libraries(vista SDL3_image::SDL3_image)
    endif()

    if(SDL3_MIXER_FOUND)
        target_link_libraries(vista SDL3_mixer::SDL3_mixer)
    endif()

    if(SDL3_TTF_FOUND)
        target_link_libraries(vista SDL3_ttf::SDL3_ttf)
    endif()
endif()

if(USE_SHADERS)
    target_link_libraries(vista
        ${OPENGL_LIBRARIES}
        GLEW::GLEW
    )
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS vista DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES config/vista.conf.example DESTINATION ${CMAKE_INSTALL_DATADIR}/vista)

# Install shaders - use /usr/local on macOS to avoid SIP restrictions
if(APPLE)
    install(DIRECTORY shaders DESTINATION /usr/local/share/vista)
else()
    install(DIRECTORY shaders DESTINATION ${CMAKE_INSTALL_DATADIR}/vista)
endif()

# FIXED: Install SDL3 shared libraries when building from submodules
if(NOT USE_SYSTEM_SDL)
    # Install all SDL3 shared libraries to lib directory
    install(TARGETS SDL3-shared
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # For Windows DLLs
    )
    
    if(SDL3_IMAGE_FOUND)
        install(TARGETS SDL3_image-shared
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
    
    if(SDL3_MIXER_FOUND)
        install(TARGETS SDL3_mixer-shared
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
    
    if(SDL3_TTF_FOUND)
        install(TARGETS SDL3_ttf-shared
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

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