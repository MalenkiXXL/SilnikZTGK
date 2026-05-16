cmake_minimum_required(VERSION 3.20)
project(SilnikZTGK CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Wyciszenie ostrzeżeń o polskich znakach (C4828)
if(MSVC)
    add_compile_options(/utf-8 /wd4828)
endif()

file(GLOB_RECURSE SOURCES
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
)

list(FILTER SOURCES EXCLUDE REGEX "cmake-build-.*")
list(FILTER SOURCES EXCLUDE REGEX "out/.*")
list(FILTER SOURCES EXCLUDE REGEX "build/.*")

add_executable(${PROJECT_NAME} ${SOURCES})

# ==========================================
# Wskazanie ścieżek do plików nagłówkowych (.h)
# ==========================================
# Ścieżka ${CMAKE_SOURCE_DIR} to główny folder Twojego projektu.
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SilnikZTGK
)

# VCPKG Zależności
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        glad::glad
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        glm::glm
        assimp::assimp
)