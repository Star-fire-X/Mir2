include(FetchContent)

FetchContent_Declare(
  entt
  URL https://github.com/skypjack/entt/archive/refs/tags/v3.13.2.zip)
FetchContent_MakeAvailable(entt)

FetchContent_Declare(
  asio
  URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-34-2.zip)
FetchContent_MakeAvailable(asio)

add_library(asio_headers INTERFACE)
target_include_directories(asio_headers SYSTEM INTERFACE
                           ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_headers INTERFACE ASIO_STANDALONE)

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()
