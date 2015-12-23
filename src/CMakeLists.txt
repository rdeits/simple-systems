# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_executable(test_systems systems.cpp)
install(TARGETS test_systems DESTINATION bin)
