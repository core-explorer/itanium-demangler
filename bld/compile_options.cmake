set(PATH_CDX_COMMON ${CMAKE_CURRENT_LIST_DIR})
set(PATH_CDX_COMMON_SOURCE ${PATH_CDX_COMMON}/src/)
set(PATH_CDX_COMMON_LIB ${PATH_CDX_COMMON}/lib/)

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
	set(FLAGS_PI3 "-mfpu=crypto-neon-fp-armv8 -march=armv8-a+crc -mcpu=cortex-a53")
	set(FLAGS_PI4 "-mcpu=cortex-a72")
	add_compile_options(${FLAGS_PI4})
endif()

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
	add_compile_options(-march=x86-64)
	message(STATUS " * x86_64 ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  message(" * C++ compiler: Emscripten")
  set(CMAKE_CXX_COMPILER_ID "Emscripten")
  if ("${EMSCRIPTEN_ENV}" STREQUAL "web")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
	else()
  set(CMAKE_EXECUTABLE_SUFFIX ".js")
  endif()
  set(CMAKE_CXX_BYTE_ORDER "LITTLE_ENDIAN")
else()
  message(" * C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
	set(CMAKE_CXX_FLAGS_DEBUG "-gsource-map -O0 -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG -Oz")
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fno-omit-frame-pointer")
	add_compile_options(-gz=zlib)
	#add_link_options(-Wl,--compress-debug-sections=zlib)
endif()


# GCC -fno-semantic-interposition -fwhole-program -Wno-error=maybe-uninitialized
# lto -flto
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O2 -fno-omit-frame-pointer")
add_compile_options(  -pipe -W -Wall -Wsign-compare -pedantic -ffunction-sections -fdata-sections  -fno-stack-protector -U_FORTIFY_SOURCE    )

add_compile_options(-Wno-error=ignored-qualifiers -Wno-error=deprecated-copy)

if(${CDX_FATAL_ERRORS})
add_compile_options(-Wfatal-errors)
endif()

if( CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
	
add_compile_options(-Wno-dollar-in-identifier-extension )
set(CDX_LINK_FLAGS "")
endif()




set (CDX_DONT_STRIP OFF CACHE BOOL "Do not strip binaries")
function (split_debug_info _target)
    if(CDX_DONT_STRIP)
	add_custom_command (TARGET ${_target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${_target}> $<TARGET_FILE:${_target}>.debug
	)
	else()
	add_custom_command (TARGET ${_target} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${_target}> $<TARGET_FILE:${_target}>.debug
		COMMAND ${CMAKE_STRIP} ${CDX_STRIP_FLAGS} $<TARGET_FILE:${_target}>
		COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:${_target}>.debug $<TARGET_FILE:${_target}>
		COMMENT "Stripping ${_target}"
	)
	endif()
endfunction (split_debug_info _target)


option(CDX_ENABLE_ASAN "enable address sanitizer" ${CDX_ENABLE_ASAN})
option(CDX_ENABLE_ANALYZER "enable static analyzer" ${CDX_ENABLE_ANALYZER})
option(CDX_ENABLE_UBSAN "enable undefined behavior sanitizer" ${CDX_ENABLE_UBSAN})
option(CDX_ENABLE_LTO "enable link time optimizations (LTO)" ${CDX_ENABLE_LTO})
option(CDX_DISABLE_DBG "disable generation of debugging information" ${CDX_DISABLE_DBG})
option(CDX_ENABLE_DISTCC "enable distributed builds" ${CDX_ENABLE_DISTCC})
option(CDX_ENABLE_GOLD "enable gold linker" ${CDX_ENABLE_GOLD})
option(CDX_ENABLE_MSAN "enable memory sanitizer" ${CDX_ENABLE_MSAN})
option(CDX_FORCE_PTHREAD "force pthreads (even for emscripten)" ${CDX_FORCE_PTHREAD})

if(CDX_ENABLE_GOLD)
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
set(CDX_LINK_FLAGS "${CDX_LINK_FLAGS} -Wl,--icf=safe")
endif()

set(CDX_ADDITIONAL_LIBRARIES "")
if (CDX_ENABLE_ASAN)
	add_compile_options( -fsanitize=address -fno-omit-frame-pointer)
	add_link_options("-fsanitize=address")
endif()

if (CDX_ENABLE_ANALYZER)
	add_compile_options( --analyze)
	
endif()

if("${CDX_HARDENING}" STREQUAL "fast")
add_compile_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
add_compile_options(-Wunsafe-buffer-usage -fsafe-buffer-usage-suggestions)
endif()

if("${CDX_HARDENING}" STREQUAL "extensive")
add_compile_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
add_compile_options(-Wunsafe-buffer-usage -fsafe-buffer-usage-suggestions)
endif()

if("${CDX_HARDENING}" STREQUAL "debug")
add_compile_definitions(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
add_compile_options(-Wunsafe-buffer-usage -fsafe-buffer-usage-suggestions)
endif()

if(CDX_ENABLE_MSAN)
	add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
	add_link_options("-fsanitize=memory")
endif()

if("${CDX_PROFILE}" STREQUAL "MEASURE")
add_compile_options( -pg -fno-omit-frame-pointer)
	add_link_options("-pg")
endif()

if("${CDX_PROFILE}" STREQUAL "COLLECT")
add_compile_options( -fprofile-generate  -fno-omit-frame-pointer)
	add_link_options("-fprofile-generate")
endif()

if("${CDX_PROFILE}" STREQUAL "USE")
add_compile_options( -fprofile-use )
	add_link_options("-fprofile-use")
endif()


if (CDX_ENABLE_UBSAN)
set(SANITIZE -fsanitize=undefined,shift,shift-exponent,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,float-divide-by-zero,float-cast-overflow,nonnull-attribute,returns-nonnull-attribute,bool,enum,pointer-overflow,builtin -fno-sanitize-recover=all)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
set(SANITIZE ${SANITIZE} -frtti -fsanitize=vptr)
endif()
	#set (CDX_CXX_COMPILE_FLAGS "${CDX_CXX_COMPILE_FLAGS} ${SANITIZE} -fno-omit-frame-pointer")
	#list (APPEND CDX_ADDITIONAL_LIBRARIES "${SANITIZE}")
	add_compile_options(-fno-omit-frame-pointer ${SANITIZE})
	add_link_options( ${SANITIZE})
endif()


if (CDX_ENABLE_LTO)
	set (CDX_CXX_COMPILE_FLAGS "${CDX_CXX_COMPILE_FLAGS} -fomit-frame-pointer -fwhole-program -march=native -ffast-math -flto")
	list (APPEND CDX_ADDITIONAL_LIBRARIES "-flto")
endif()


if (CDX_ENABLE_DBG)
	message(STATUS enable debug symbols)
	set (CDX_CXX_COMPILE_FLAGS "${CDX_CXX_COMPILE_FLAGS} -g3")
	# set (CDX_LINK_FLAGS "${CDX_LINK_FLAGS} -Wl,--strip-all")
endif()

function (CDX_add_executable target_ ARG0)
	add_executable (${ARGV})
	set_target_properties (${target_} PROPERTIES COMPILE_FLAGS "${CDX_CXX_COMPILE_FLAGS}")
	set_target_properties (${target_} PROPERTIES LINK_FLAGS "${CDX_LINK_FLAGS}")
endfunction ()

function (CDX_add_test target_ ARG0)
	CDX_add_executable (${ARGV})
	add_test(NAME ${target_} COMMAND ${target_})
endfunction ()

