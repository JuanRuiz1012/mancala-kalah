cmake_minimum_required(VERSION 3.16)
project(mancala_motor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags de optimización y OpenMP
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
find_package(OpenMP REQUIRED)

# ---------------------------------------------------------------------------
# Fuentes compartidas (tablero + motores)
# ---------------------------------------------------------------------------
set(MOTOR_SOURCES
    src/board.cpp
    src/alphabeta.cpp
    src/mcts.cpp
)

# ---------------------------------------------------------------------------
# Ejecutable principal: servidor HTTP del motor
# ---------------------------------------------------------------------------
add_executable(mancala_motor src/main.cpp ${MOTOR_SOURCES})
target_link_libraries(mancala_motor PRIVATE OpenMP::OpenMP_CXX)

# ---------------------------------------------------------------------------
# Ejecutable de benchmark independiente
# ---------------------------------------------------------------------------
add_executable(mancala_bench src/bench.cpp ${MOTOR_SOURCES})
target_link_libraries(mancala_bench PRIVATE OpenMP::OpenMP_CXX)

# ---------------------------------------------------------------------------
# Tests con Google Test
# Descarga GTest automáticamente si no está instalado en el sistema
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(test_board tests/test_board.cpp ${MOTOR_SOURCES})
target_link_libraries(test_board PRIVATE GTest::gtest_main OpenMP::OpenMP_CXX)
target_include_directories(test_board PRIVATE src)

enable_testing()
include(GoogleTest)
gtest_discover_tests(test_board)