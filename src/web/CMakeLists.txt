
if(CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")

add_compile_definitions(EMSCRIPTEN_HAS_UNBOUND_TYPE_NAMES=0)


if ("${EMSCRIPTEN_ENV}" STREQUAL "web")

add_executable(index demangle-shell.cxx)
settings(index)
target_link_options(index PUBLIC  --shell-file ${CMAKE_SOURCE_DIR}/src/web/demangle-shell.html)
target_link_options(index PUBLIC -sEXPORTED_FUNCTIONS=_main,_malloc,_free
-sEXPORTED_RUNTIME_METHODS=UTF8ToString,stringToUTF8,lengthBytesUTF8,ccall,cwrap)

target_link_libraries(index PUBLIC   swartalf::demangle swartalf::version)

add_executable(grammar grammar-shell.cxx )
settings(grammar)
target_link_options(grammar PUBLIC  --shell-file ${CMAKE_SOURCE_DIR}/src/web/grammar-shell.html)
target_link_options(grammar PUBLIC -sEXPORTED_FUNCTIONS=_main,_malloc,_free
-sEXPORTED_RUNTIME_METHODS=UTF8ToString,stringToUTF8,lengthBytesUTF8,ccall,cwrap)
target_link_libraries(grammar PUBLIC  swartalf::demangle swartalf::version)

set(CONTENT ${CMAKE_SOURCE_DIR}/src/web/content)
add_custom_command(TARGET grammar PRE_BUILD  
    COMMAND ${CMAKE_COMMAND} -E copy  
        ${CONTENT}/favicon.svg  
        ${CMAKE_BINARY_DIR}/favicon.svg
    COMMAND ${CMAKE_COMMAND} -E copy  
        ${CONTENT}/style.css  
        ${CMAKE_BINARY_DIR}/style.css
)

file(COPY ${CONTENT}/fonts DESTINATION ${CMAKE_BINARY_DIR}/)


endif()

endif()

