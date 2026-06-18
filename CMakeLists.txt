cmake_minimum_required(VERSION 3.20)
project(SilnikZTGK C CXX)

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
list(FILTER SOURCES EXCLUDE REGEX ".*venv.*")
list(FILTER SOURCES EXCLUDE REGEX "tests/.*")

# ==========================================
# GŁÓWNY PROGRAM
# ==========================================
add_executable(${PROJECT_NAME} ${SOURCES})

# Wskazanie ścieżek do plików nagłówkowych (.h)
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

# Tryb Dystrybucji (Eksport Gry bez Edytora)
if (CS_IS_DISTRIBUTION)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CS_DISTRIBUTION)
endif()

# ==========================================
# TESTY JEDNOSTKOWE (GOOGLE TEST)
# ==========================================
include(FetchContent)

# Deklarujemy, skąd CMake ma pobrać Google Test
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.12.1
)

# Wymuszamy kompatybilność z bibliotekami standardowymi na Windowsie
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Pobieramy i udostępniamy GTest dla naszego projektu
FetchContent_MakeAvailable(googletest)

set(ENGINE_SOURCES ${SOURCES})
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "main\\.cpp")

# Tworzymy osobną aplikację (.exe), która będzie odpalać tylko testy
add_executable(SilnikZTGK_Tests
        tests/test_delivery_ai.cpp
        tests/test_event_bus.cpp
        tests/test_ecs.cpp
        tests/test_machines.cpp
        ${ENGINE_SOURCES}
        tests/test_integration.cpp
        tests/test_waiter_script.cpp
)

# Informacja gdzie leżą pliki nagłówkowe silnika
target_include_directories(SilnikZTGK_Tests PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SilnikZTGK
)

# Podpinamy GTest oraz niezbędne biblioteki
target_link_libraries(SilnikZTGK_Tests PRIVATE
        gtest
        gtest_main
        glm::glm
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        OpenGL::GL
        glfw
        glad::glad
        assimp::assimp
)