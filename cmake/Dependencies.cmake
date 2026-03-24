include(FetchContent)

FetchContent_Declare(
  entt
  URL https://github.com/skypjack/entt/archive/refs/tags/v3.13.2.zip)
FetchContent_MakeAvailable(entt)

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()
