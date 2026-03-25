add_library(shared_protocol STATIC
  protocol/wire_codec.cc)

target_include_directories(shared_protocol PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(shared_protocol)

if(BUILD_TESTING)
  add_executable(
    wire_codec_test
    tests/wire_codec_test.cc)

  target_include_directories(wire_codec_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(wire_codec_test PRIVATE shared_protocol GTest::gtest_main)
  set_project_warnings(wire_codec_test)

  include(GoogleTest)
  gtest_discover_tests(wire_codec_test)
endif()
