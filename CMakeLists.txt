cmake_minimum_required(VERSION 3.23)
project(DurinsCode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# ── Third-party ───────────────────────────────────────────────
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# ── Lexer ─────────────────────────────────────────────────────
add_library(durins_lexer src/lexer/lexer.cpp)
target_include_directories(durins_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ── Parser ────────────────────────────────────────────────────
add_library(durins_parser src/parser/parser.cpp)
target_include_directories(durins_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(durins_parser PUBLIC durins_lexer)

# ── Semantic ──────────────────────────────────────────────────
add_library(durins_semantic src/semantic/semantic.cpp)
target_include_directories(durins_semantic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(durins_semantic PUBLIC durins_parser)

# ── TAC ───────────────────────────────────────────────────────
add_library(durins_tac src/tac/tac.cpp src/tac/optimizer.cpp)
target_include_directories(durins_tac PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(durins_tac PUBLIC durins_semantic)

# ── Codegen ───────────────────────────────────────────────────
add_library(durins_codegen src/codegen/codegen.cpp)
target_include_directories(durins_codegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(durins_codegen PUBLIC durins_tac)

# ── VM ────────────────────────────────────────────────────────
add_library(durins_vm src/vm/vm.cpp)
target_include_directories(durins_vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(durins_vm PUBLIC durins_codegen)

# ── Main CLI executable ───────────────────────────────────────
add_executable(durinsc src/main.cpp)
target_link_libraries(durinsc PRIVATE durins_vm)

# ── Tests ─────────────────────────────────────────────────────
find_package(GTest REQUIRED)

add_executable(lexer_tests test/lexer_test.cpp)
target_link_libraries(lexer_tests PRIVATE durins_lexer GTest::gtest_main)

add_executable(parser_tests test/parser_test.cpp)
target_link_libraries(parser_tests PRIVATE durins_parser durins_lexer GTest::gtest_main)

add_executable(semantic_tests test/semantic_test.cpp)
target_link_libraries(semantic_tests PRIVATE durins_semantic GTest::gtest_main)

add_executable(tac_tests test/tac_test.cpp)
target_link_libraries(tac_tests PRIVATE durins_tac GTest::gtest_main)

add_executable(codegen_tests test/codegen_test.cpp)
target_link_libraries(codegen_tests PRIVATE durins_codegen GTest::gtest_main)

add_executable(vm_tests test/vm_test.cpp)
target_link_libraries(vm_tests PRIVATE durins_vm GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lexer_tests)
gtest_discover_tests(parser_tests)
gtest_discover_tests(semantic_tests)
gtest_discover_tests(tac_tests)
gtest_discover_tests(codegen_tests)
gtest_discover_tests(vm_tests)
