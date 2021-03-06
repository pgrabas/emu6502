add_custom_target(build_all_libs ALL)
add_custom_target(build_all_test ALL)
add_custom_target(build_all_executables ALL)
add_custom_target(build_all_modules ALL)
add_custom_target(execute_all_test)

function(define_static_lib target_name)
  file(GLOB_RECURSE SRC src/*.cpp src/*.hpp include/*.hpp)

  message("* Adding static lib ${target_name}")
  add_library(${target_name} STATIC ${SRC})
  target_include_directories(${target_name} PUBLIC include)
  target_include_directories(${target_name} PRIVATE src)
  target_link_libraries(${target_name} PUBLIC fmt::fmt)
  add_dependencies(build_all_libs ${target_name})
  enable_clang_tidy(${target_name})

  set(TARGET
      ${target_name}
      PARENT_SCOPE)
  set(LIB_TARGET
      ${target_name}
      PARENT_SCOPE)
endfunction()

function(define_module target_name)
  set(module_name "emu.module.${target_name}")
  set(lib_name "emu_module_${target_name}")

  define_static_lib(${lib_name})
  target_link_libraries(${lib_name} PUBLIC emu_core emu_module_core)
  file(GLOB_RECURSE SRC module/*)

  message("* Adding module ${module_name}")
  add_library(${module_name} MODULE ${SRC})
  target_include_directories(${module_name} PUBLIC include)
  target_include_directories(${module_name} PRIVATE src)
  target_link_libraries(${module_name} PUBLIC fmt::fmt ${lib_name} emu_core emu_module_core)
  add_dependencies(build_all_modules ${module_name})
  enable_clang_tidy(${module_name})

  install(
    TARGETS ${module_name}
    COMPONENT modules
    DESTINATION bin)

  set(TARGET
      ${lib_name}
      PARENT_SCOPE)
  set(MODULE_TARGET
      ${module_name}
      PARENT_SCOPE)
  set(LIB_TARGET
      ${lib_name}
      PARENT_SCOPE)
endfunction()

function(define_executable target_name )
  file(GLOB_RECURSE SRC src/*.cpp src/*.hpp include/*.hpp)

  message("* Adding executable ${target_name}")
  add_executable(${target_name} ${SRC})
  target_include_directories(${target_name} PUBLIC include)
  target_include_directories(${target_name} PRIVATE src)
  target_link_libraries(${target_name} PUBLIC fmt::fmt)
  add_dependencies(build_all_executables ${target_name})
  enable_clang_tidy(${target_name})

  install(
    TARGETS ${target_name}
    COMPONENT executables
    DESTINATION bin)

  set(TARGET
      ${target_name}
      PARENT_SCOPE)
endfunction()

function(define_ut_target target_name ut_name)
  file(GLOB src_ut ${ut_name}/*)

  string(REGEX REPLACE "test(.*)" "\\1" short_ut_name ${ut_name})
  string(REPLACE "/" "_" valid_ut_name "${short_ut_name}")

  set(target_ut_name "${target_name}${valid_ut_name}_ut")
  message("* Adding UTs ${target_ut_name} ")

  add_executable(${target_ut_name} ${src_ut})
  target_include_directories(${target_ut_name} PRIVATE src test)
  target_link_libraries(${target_ut_name} PUBLIC ${target_name} fmt::fmt GTest::gmock GTest::gtest ${ut_runner})
  target_compile_definitions(${target_ut_name} PRIVATE -DWANTS_GTEST_MOCKS)

  add_custom_target(
    run_${target_ut_name}
    COMMAND ${target_ut_name} --gtest_shuffle
    WORKING_DIRECTORY ${TARGET_DESTINATTION}
    COMMENT "Running test ${target_ut_name}"
    DEPENDS ${target_ut_name} ${target_name})

  add_test(
    NAME test_${target_ut_name}
    COMMAND ${target_ut_name} --gtest_shuffle --gtest_output=xml:${TEST_RESULT_DIR}/${target_ut_name}.xml
    WORKING_DIRECTORY ${TARGET_DESTINATTION})

  set_property(
    TARGET run_${target_ut_name}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TEST_RESULT_DIR}/${target_ut_name}.xml)

  install(
    TARGETS ${target_ut_name}
    COMPONENT test
    DESTINATION test)

  add_dependencies(execute_all_test run_${target_ut_name})
  add_dependencies(build_all_test ${target_ut_name})
endfunction()

function(define_ut_multi_target target_name ut_name)
  file(
    GLOB src_ut
    LIST_DIRECTORIES true
    ${ut_name}/*)

  set(srclist "")

  foreach(child ${src_ut})
    file(RELATIVE_PATH rel_child ${CMAKE_CURRENT_SOURCE_DIR} ${child})
    if(IS_DIRECTORY ${child})
      define_ut_target(${target_name} ${rel_child})
    else()
      list(APPEND srclist ${rel_child})
    endif()
  endforeach()
  if(srclist)
    define_ut_target(${target_name} ${ut_name})
  endif()
endfunction()

function(define_static_lib_with_ut target_name)
  define_static_lib(${target_name})
  define_ut_multi_target(${target_name} test)
  set(TARGET
      ${target_name}
      PARENT_SCOPE)
endfunction()

function(define_module_with_ut target_name)
  define_module(${target_name})
  define_ut_multi_target(${LIB_TARGET} test)

  set(TARGET
      ${LIB_TARGET}
      PARENT_SCOPE)
endfunction()

set(FUNCTIONAL_IMAGES_DIR ${TARGET_DESTINATTION}/functional_test_images)
file(MAKE_DIRECTORY ${FUNCTIONAL_IMAGES_DIR})

add_test(
  NAME functional_tests
  COMMAND emu_functional_test_runner_ut --gtest_shuffle --gtest_output=xml:${TEST_RESULT_DIR}/functional_tests.xml
  WORKING_DIRECTORY ${TARGET_DESTINATTION})

add_custom_target(
  run_functional_tests
  COMMENT "Running functional tests"
  COMMAND emu_functional_test_runner_ut --gtest_shuffle
  WORKING_DIRECTORY ${TARGET_DESTINATTION}
  DEPENDS emu_functional_test_runner_ut build_all_modules build_all_test)

add_dependencies(execute_all_test run_functional_tests)

function(define_functional_test)
  set(options)
  set(oneValueArgs NAME IMAGE)
  set(multiValueArgs DEPENDS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME})
  message("* Adding functional test ${ARG_NAME}")

  get_filename_component(IMAGE_FILE_NAME ${ARG_IMAGE} NAME)
  set(TEST_IMAGE ${FUNCTIONAL_IMAGES_DIR}/${IMAGE_FILE_NAME})

  add_custom_command(
    OUTPUT ${TEST_IMAGE}
    COMMENT "Copy functional test image ${ARG_NAME}"
    COMMAND ${CMAKE_COMMAND} -E copy ${ARG_IMAGE} ${TEST_IMAGE}
    DEPENDS ${ARG_IMAGE}
    VERBATIM)

  add_custom_target(${ARG_NAME} DEPENDS emu_6502_runner build_all_modules ${TEST_IMAGE} ${ARG_DEPENDS} ${ARG_IMAGE})

  set_property(
    TARGET ${ARG_NAME}
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES ${TEST_IMAGE})

  add_dependencies(build_all_test ${ARG_NAME})
endfunction()
