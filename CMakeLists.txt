cmake_minimum_required(VERSION 3.19)

project(itanium-demangler)

include (bld/compile_options.cmake)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Emscripten")
	add_compile_options(-Wno-experimental -Wno-error=experimental)
  if("${EMSCRIPTEN_ENV}" STREQUAL "node")
    add_link_options( -sNODERAWFS -s ENVIRONMENT=node,worker )
    
    if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
     add_link_options(-sSINGLE_FILE)
     else()
    
    add_link_options(-sNO_DISABLE_EXCEPTION_CATCHING)
     endif()

  endif()
  if("${EMSCRIPTEN_ENV}" STREQUAL "web")
    
    add_link_options( -s ENVIRONMENT=web,worker -sFETCH=1 )
    
  endif()
	
  add_compile_options(-matomics -mbulk-memory)
	
endif()


add_compile_definitions(BOOST_ASIO_HAS_PTHREADS)
add_subdirectory(src)
