INCLUDE(CMakeForceCompiler)
add_compile_definitions(_LIBCPP_ENABLE_EXPERIMENTAL)
if ("${TARGET_PROCESSOR}" STREQUAL "")
	set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
	add_compile_options("-stdlib=libc++")
	add_link_options("-stdlib=libc++")
else()
	set(CMAKE_SYSTEM_PROCESSOR "${TARGET_PROCESSOR}")
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")

	# Declare some constants
	set(CMAKE_SYSTEM_NAME Linux) # setting this explicitly tells cmake that we are crosscompiling

	set(TOOLCHAIN_PATH "/usr/${SYSTRIPLET}" CACHE PATH "Path to toolchain native sysroot")
	set(TARGET_SYSROOT_PATH "${CMAKE_SOURCE_DIR}/sysroots/${CMAKE_SYSTEM_PROCESSOR}" CACHE PATH "Path to toolchain target sysroot")


	add_compile_options( -stdlib=libc++ --sysroot=${TARGET_SYSROOT_PATH} -I${TARGET_SYSROOT_PATH}/usr/include/c++/${GCCVER} -I${TARGET_SYSROOT_PATH}/usr/include/${SYSTRIPLET}/c++/${GCCVER})
	add_link_options("-stdlib=libc++ -Wl,-rpath-link=${TARGET_SYSROOT_PATH}/usr/lib/${SYSTRIPLET}/  -Wl,-rpath-link=${TARGET_SYSROOT_PATH}/lib/${SYSTRIPLET}")

	# where is the target environment
	set(CMAKE_FIND_ROOT_PATH  ${TARGET_SYSROOT_PATH} ${TOOLCHAIN_PATH})
	set(CMAKE_SYSROOT ${TARGET_SYSROOT_PATH})

	# search for programs in the build host directories
	set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	# for libraries and headers in the target directories
	set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

endif()

find_program(AFL_CC afl-clang-lto REQUIRED)
find_program(AFL_CXX afl-clang-lto++ REQUIRED)

SET(CMAKE_FORCE_C_COMPILER ${AFL_CC})
SET(CMAKE_FORCE_CXX_COMPILER ${AFL_CXX})
SET(CMAKE_C_COMPILER ${CMAKE_FORCE_C_COMPILER})
SET(CMAKE_CXX_COMPILER ${CMAKE_FORCE_CXX_COMPILER})

SET(CDX_AFL TRUE)

set(CDX_TOOLCHAIN "afl")