set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(BOOST_ASIO_DISABLE_SOURCE_LOCATION=1)
add_compile_definitions(BOOST_ASIO_DISABLE_STD_SOURCE_LOCATION=1)
add_compile_definitions(BOOST_ASIO_DISABLE_STD_EXPERIMENTAL_SOURCE_LOCATION=1)
add_compile_definitions(BOOST_ASIO_DISABLE_BOOST_SOURCE_LOCATION=1)
#add_compile_definitions(BOOST_ASIO_ENABLE_HANDLER_TRACKING=1)
add_compile_options( -Wall -Wextra -pedantic -Wshadow -Werror -fno-omit-frame-pointer -fno-stack-protector)
if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	add_compile_options(-march=x86-64-v3)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Emscripten")
	add_compile_options(-Wno-experimental -Wno-error=experimental )
else()
    add_link_options( -Wl,-z,relro,-z,now,--as-needed )
endif()
#message(STATUS " * C++ compiler: ${CMAKE_CXX_COMPILER_ID}")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-flto=auto)
    add_link_options(-flto=auto)
endif()
    

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


function(settings target)
    target_compile_options(${target} PUBLIC  -fno-rtti  )
    target_compile_options(${target} PUBLIC -fvisibility=hidden  -fPIE )
    
    target_compile_features(${target} PUBLIC cxx_std_23)
    set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET ${target} PROPERTY UNITY_BUILD ${CALEIDO_ENABLE_UNITY})    

    target_include_directories(${target} PUBLIC ${LIBELF_INCLUDE_DIR})
endfunction()

function(create_header_test name)
    get_filename_component(name_without_extension ${name} NAME_WE)
    set(test_name "${name_without_extension}_test")
    set(test_file "${CMAKE_CURRENT_BINARY_DIR}/header_test/${test_name}.cxx")
    file(WRITE ${test_file} "#include \"${name}\"\n")
    endfunction()

add_compile_definitions(SWARTALF_ENABLE_ELF32=1)



add_subdirectory(cli)

add_subdirectory(itanium)
#add_subdirectory(swartalf)
add_subdirectory(util)
add_subdirectory(web)

if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "Emscripten")

	#add_subdirectory(dbgfind)
endif()

set(HEADERS)
set(HEADER_TESTS)

foreach(header ${HEADERS})
    get_filename_component(name_without_extension ${header} NAME_WE)
    set(test_name "${name_without_extension}_test")
    set(test_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.cxx")
    set( HEADER_TESTS ${HEADER_TESTS} ${test_file})

    create_header_test(${header})
endforeach()

#add_library(header_tests OBJECT ${HEADER_TESTS})
#target_include_directories(header_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(header_tests PRIVATE -fvisibility=hidden -fno-rtti -O0 -g0 -fno-lto)
