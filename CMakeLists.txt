cmake_minimum_required(VERSION 3.16)
project(LuauUnscrambler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Options ---
option(USE_SYSTEM_SDL2 "Use system SDL2 if available" ON)

# --- Find SDL2 ---
if(USE_SYSTEM_SDL2)
    find_package(SDL2 QUIET)
endif()

if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found via find_package, using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
        GIT_SHALLOW TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)
    set(SDL2_LIBRARIES SDL2-static SDL2main)
    set(SDL2_INCLUDE_DIRS "")
endif()

# --- ImGui (vendored headers) ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

# --- Sources ---
set(APP_SOURCES
    src/main.cpp
    src/luau_unscrambler.cpp
    src/ui.cpp
)

add_executable(luau-unscrambler ${APP_SOURCES} ${IMGUI_SOURCES})

target_include_directories(luau-unscrambler PRIVATE
    include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(luau-unscrambler PRIVATE ${SDL2_LIBRARIES})

# --- Platform specifics ---
if(WIN32)
    target_link_libraries(luau-unscrambler PRIVATE winmm version imm32 setupapi)
    set_target_properties(luau-unscrambler PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    find_library(COCOA_LIB Cocoa)
    find_library(IOKIT_LIB IOKit)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    target_link_libraries(luau-unscrambler PRIVATE ${COCOA_LIB} ${IOKIT_LIB} ${COREFOUNDATION_LIB})
elseif(UNIX)
    target_link_libraries(luau-unscrambler PRIVATE dl pthread)
endif()

# --- Install ---
install(TARGETS luau-unscrambler DESTINATION bin)
