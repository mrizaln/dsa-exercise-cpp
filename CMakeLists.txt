cmake_minimum_required(VERSION 3.14)
project(
  dsa
  LANGUAGES CXX
  VERSION 0.0.0
  DESCRIPTION "DSA in C++")

add_library(dsa INTERFACE)
target_include_directories(dsa INTERFACE include)
target_compile_features(dsa INTERFACE cxx_std_23)
set_target_properties(dsa PROPERTIES CXX_EXTENSIONS OFF)

# build tests only when this project not included as subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  include(cmake/prelude.cmake)
  include(cmake/mold.cmake)

  find_package(fmt REQUIRED)
  find_package(ut REQUIRED)

  function(make_test NAME)
    add_executable(${NAME} test/${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE dsa fmt::fmt Boost::ut)
    target_compile_features(${NAME} PRIVATE cxx_std_23)
    set_target_properties(${NAME} PROPERTIES CXX_EXTENSIONS OFF)

    target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wconversion)
    target_compile_options(${NAME} PRIVATE -fsanitize=address,leak,undefined)
    target_link_options(${NAME} PRIVATE -fsanitize=address,leak,undefined)

    add_test(NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)
    add_custom_command(
      TARGET ${NAME}
      POST_BUILD
      COMMAND ${NAME})
  endfunction()

  enable_testing()
  make_test(array_list)
  make_test(linked_list)
  make_test(doubly_linked_list)
  make_test(stack)
  make_test(queue)
  make_test(circular_buffer)
  make_test(deque)
  make_test(raw_buffer)
  make_test(fixed_array)
  make_test(rootish_array)

endif()
