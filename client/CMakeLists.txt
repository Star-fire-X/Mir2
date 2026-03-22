add_executable(client_app app/client_main.cc)
target_include_directories(client_app PRIVATE ${PROJECT_SOURCE_DIR})
set_project_warnings(client_app)
