INCLUDE(CMakeForceCompiler)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>)
add_compile_definitions(_LIBCPP_ENABLE_EXPERIMENTAL)
add_link_options("-stdlib=libc++")
if ("${TARGET_PROCESSOR}" STREQUAL "")
	set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
	
else()
	set(CMAKE_SYSTEM_PROCESSOR "${TARGET_PROCESSOR}")
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")

	# Declare some constants
	set(CMAKE_SYSTEM_NAME Linux) # setting this explicitly tells cmake that we are crosscompiling

	set(TOOLCHAIN_PATH "/usr/${SYSTRIPLET}" CACHE PATH "Path to toolchain native sysroot")
	set(TARGET_SYSROOT_PATH "${CMAKE_SOURCE_DIR}/sysroots/${CMAKE_SYSTEM_PROCESSOR}" CACHE PATH "Path to toolchain target sysroot")

	
	add_compile_options( --sysroot=${TARGET_SYSROOT_PATH} -I${TARGET_SYSROOT_PATH}/usr/include/c++/${GCCVER} -I${TARGET_SYSROOT_PATH}/usr/include/${SYSTRIPLET}/c++/${GCCVER})
	
	add_link_options("-Wl,-rpath-link=${TARGET_SYSROOT_PATH}/usr/lib/${SYSTRIPLET}/  -Wl,-rpath-link=${TARGET_SYSROOT_PATH}/lib/${SYSTRIPLET}")

	# where is the target environment
	set(CMAKE_FIND_ROOT_PATH  ${TARGET_SYSROOT_PATH} ${TOOLCHAIN_PATH})
	set(CMAKE_SYSROOT ${TARGET_SYSROOT_PATH})

	# search for programs in the build host directories
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	# for libraries and headers in the target directories
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

endif()

if("${CMAKE_CLANG_VERSION}" STREQUAL "")
	if("$ENV{CLANG_VERSION}" STREQUAL "")
		find_program(CLANG_RAW clang REQUIRED)

		execute_process(COMMAND ${CLANG_RAW} --version COMMAND head -n1 COMMAND xargs -n1 COMMAND tail -n1 COMMAND cut -d "." -f 1 OUTPUT_VARIABLE CLANG_VERSION_MAJOR OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND ${CLANG_RAW} --version COMMAND head -n1 COMMAND xargs -n1 COMMAND tail -n1 COMMAND cut -d "." -f 2 OUTPUT_VARIABLE CLANG_VERSION_MINOR OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE "(" "" CLANG_VERSION_MAJOR "${CLANG_VERSION_MAJOR}")
		string(REPLACE ")" "" CLANG_VERSION_MAJOR "${CLANG_VERSION_MAJOR}")

		message(STATUS "clang ${CLANG_VERSION_MAJOR} ${CLANG_VERSION_MINOR}")
		find_program(CLANG NAMES clang-${CLANG_VERSION_MAJOR}.${CLANG_VERSION_MINOR} clang-${CLANG_VERSION_MAJOR} clang)
		find_program(CLANGPP NAMES clang++-${CLANG_VERSION_MAJOR}.${CLANG_VERSION_MINOR} clang++-${CLANG_VERSION_MAJOR} clang++)

	else()
		set(CMAKE_CLANG_VERSION $ENV{CLANG_VERSION})
		find_program(CLANG NAMES clang-${CMAKE_CLANG_VERSION} REQUIRED)
		find_program(CLANGPP NAMES clang++-${CMAKE_CLANG_VERSION} REQUIRED)

	endif()
endif()

add_compile_options(-gtemplate-alias -gcolumn-info -gfull -fclang-abi-compat=17)
add_link_options(-Wl,--build-id=sha1)

# specify the cross compiler
SET(CMAKE_FORCE_C_COMPILER ${CLANG})
SET(CMAKE_FORCE_CXX_COMPILER ${CLANGPP})
SET(CMAKE_C_COMPILER ${CMAKE_FORCE_C_COMPILER})
SET(CMAKE_CXX_COMPILER ${CMAKE_FORCE_CXX_COMPILER})

set(CDX_TOOLCHAIN "clang")