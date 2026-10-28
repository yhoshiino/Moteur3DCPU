cmake_minimum_required(VERSION 3.20)
project(Moteur3DCPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.18.zip
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

add_executable(Moteur3DCPU src/main.cpp "header/vec.h" "header/mat.h" "header/obj.h" "header/texture.h" "header/clip.h" "header/ecs/componentStorage.h" "header/ecs/entityManager.h" "header/ecs/sparse.h" "header/components/components.h" "header/components/transform.h" "header/components/physics.h" "header/components/render.h")

target_include_directories(Moteur3DCPU PRIVATE header)
target_include_directories(Moteur3DCPU PRIVATE assets)
target_include_directories(Moteur3DCPU PRIVATE assets/fonts)

target_link_libraries(Moteur3DCPU PRIVATE SDL3::SDL3)

target_compile_definitions(Moteur3DCPU PRIVATE
    WARP_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")