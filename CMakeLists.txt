cmake_minimum_required(VERSION 3.20)
project(SilnikZTGK C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# USTAWIENIA KOMPILATORA
# ==========================================
if(MSVC)
    # Windows (Visual Studio) - polskie znaki i podstawowe ostrzeżenia
    add_compile_options(/utf-8 /wd4828 /W3)
else()
    # Linux/MacOS (GCC/Clang) - standardowe ostrzeżenia
    add_compile_options(-Wall -Wextra)
endif()

# Zbieranie plików źródłowych
file(GLOB_RECURSE SOURCES
        "*.cpp"
        "*.c"
        "*.h"
        "*.hpp"
)

# Filtrowanie śmieci i testów (złączone w jeden czytelny Regex)
list(FILTER SOURCES EXCLUDE REGEX "(cmake-build-.*|out/.*|build/.*|.*venv.*|tests/.*)")

# ==========================================
# ZALEŻNOŚCI (Cross-Platform: dnf/apt / vcpkg)
# ==========================================
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)

# Szukamy nagłówków STB w systemie/vcpkg (rozwiązuje problem sztywnych ścieżek)
find_path(STB_INCLUDE_DIR stb_image.h PATH_SUFFIXES stb stb_image)

# Automatyczny wybór targetu spdlog (naprawia błąd ABI na Linuksie, działa na Windows)
if(TARGET spdlog::spdlog_header_only)
    set(SPDLOG_TARGET spdlog::spdlog_header_only)
else()
    set(SPDLOG_TARGET spdlog::spdlog)
endif()

# ==========================================
# GŁÓWNY PROGRAM
# ==========================================
add_executable(${PROJECT_NAME} ${SOURCES})

# Wskazanie ścieżek do plików nagłówkowych (.h)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SilnikZTGK
)

# Jeśli znaleziono STB, dodajemy jego folder do ścieżek
if(STB_INCLUDE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIR})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        ${SPDLOG_TARGET}
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
        tests/test_PrefabSerializer.cpp
)

# Informacja gdzie leżą pliki nagłówkowe silnika
target_include_directories(SilnikZTGK_Tests PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/SilnikZTGK
)

if(STB_INCLUDE_DIR)
    target_include_directories(SilnikZTGK_Tests PRIVATE ${STB_INCLUDE_DIR})
endif()

# Podpinamy GTest oraz niezbędne biblioteki
target_link_libraries(SilnikZTGK_Tests PRIVATE
        gtest
        gtest_main
        glm::glm
        ${SPDLOG_TARGET}
        nlohmann_json::nlohmann_json
        OpenGL::GL
        glfw
        assimp::assimp
)