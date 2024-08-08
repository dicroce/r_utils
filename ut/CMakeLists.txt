
add_executable(
    ut
    include/framework.h
    source/framework.cpp
    include/test_r_utils.h
    source/test_r_utils.cpp
)

if(MSVC)
    target_link_options(ut PUBLIC "/STACK:10000000")
endif()

target_include_directories(
    ut PUBLIC 
    include
    ../r_utils/include
)
target_link_libraries(
    ut LINK_PUBLIC 
    r_utils
)
if(CMAKE_SYSTEM MATCHES "Linux-")
    target_link_libraries(
        ut PUBLIC
        pthread
        dl
        uuid
    )
elseif(CMAKE_SYSTEM MATCHES "Windows")
endif(CMAKE_SYSTEM MATCHES "Linux-")
