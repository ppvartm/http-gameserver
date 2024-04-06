cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(model_lib STATIC
        src/model/model.h
	src/model/model.cpp	
        src/model/loot_generator.h
        src/model/loot_generator.cpp
        src/model/collision_detector.h
        src/model/collision_detector.cpp	
)

target_link_libraries(model_lib PRIVATE CONAN_PKG::boost Threads::Threads)

add_executable(game_server
        src/extra/tagged.h
        src/extra/extra_data.h
        src/extra/extra_data.cpp
	src/main.cpp
	src/web/http_server.cpp
	src/web/http_server.h
	src/web/sdk.h
	src/json_tools/boost_json.cpp
	src/json_tools/json_loader.h
	src/json_tools/json_loader.cpp
	src/web/request_handler.cpp
	src/web/request_handler.h
        src/web/log.h
        src/web/log.cpp
        src/app/app.h
        src/app/app.cpp
        src/web/timer.h
        src/serialization/app_serialization.h
        src/serialization/app_serialization.cpp
        src/database_tools/postgres.h
        src/database_tools/postgres.cpp         
) 

target_link_libraries(game_server Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx model_lib CONAN_PKG::boost)
