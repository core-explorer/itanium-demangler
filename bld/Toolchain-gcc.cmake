
if ("${TARGET_PROCESSOR}" STREQUAL "")
	set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu" CACHE STRING "" FORCE)
else()
	set(CMAKE_SYSTEM_PROCESSOR "${TARGET_PROCESSOR}" CACHE STRING "" FORCE)
	set(SYSTRIPLET "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu" CACHE STRING "" FORCE)

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


set(GCC_ROOT "/usr/bin")

if("${GCCVER}" STREQUAL "")
	if("$ENV{GCCVER}" STREQUAL "")
		find_program(GCC_RAW gcc)

		execute_process(COMMAND ${GCC_RAW} --version COMMAND head -n1 COMMAND cut -d " " -f3 COMMAND cut -d "." -f 1 OUTPUT_VARIABLE GCC_VERSION_MAJOR OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND ${GCC_RAW} --version COMMAND head -n1 COMMAND cut -d " " -f3 COMMAND cut -d "." -f 2 OUTPUT_VARIABLE GCC_VERSION_MINOR OUTPUT_STRIP_TRAILING_WHITESPACE)
		
		find_program(GCCV NAMES gcc-${GCC_VERSION_MAJOR}.${GCC_VERSION_MINOR} PATH ${GCC_ROOT})
		if(GCCV)
			set(GCCVER "-${GCC_VERSION_MAJOR}.${GCC_VERSION_MINOR}")
			# message(STATUS "GCC ${GCCV} version ${GCCVER}")

		else()
			find_program(GCCV NAMES gcc-${GCC_VERSION_MAJOR} PATH ${GCC_ROOT})
			if(GCCV)
				set(GCCVER "-${GCC_VERSION_MAJOR}")
				# message(STATUS "GCC ${GCCV} version ${GCCVER}")
			else()
				# message(STATUS "explict GCC not found")
				find_program(GCCV NAMES gcc PATH ${GCC_ROOT} REQUIRED)
				set(GCCVER "")
			endif()

		endif()	

	else()
		set(GCCVER $ENV{GCCVER})
	endif()
endif()

find_program(GCC_EXPLICIT NAMES ${SYSTRIPLET}-gcc${GCCVER} PATH ${GCC_ROOT})
find_program(GXX_EXPLICIT NAMES ${SYSTRIPLET}-g++${GCCVER} PATH ${GCC_ROOT})

if(NOT GCC_EXPLICIT OR NOT GXX_EXPLICIT)
	find_program(GCC_EXPLICIT NAMES gcc${GCCVER} PATH ${GCC_ROOT} REQUIRED)
	find_program(GXX_EXPLICIT NAMES g++${GCCVER} PATH ${GCC_ROOT} REQUIRED)
endif()

#add_compile_options(-fimplicit-constexpr)

# specify the cross compiler
SET(CMAKE_C_COMPILER ${GCC_EXPLICIT} CACHE STRING "" FORCE)
SET(CMAKE_CXX_COMPILER ${GXX_EXPLICIT} CACHE STRING "" FORCE)
SET(CMAKE_ASM_COMPILER ${GCC_EXPLICIT} CACHE STRING "" FORCE)

SET(CMAKE_AR /usr/bin/${SYSTRIPLET}-gcc-ar${GCCVER} CACHE STRING "" FORCE)
SET(CMAKE_RANLIB /usr/bin/${SYSTRIPLET}-gcc-ranlib${GCCVER} CACHE STRING "" FORCE)

SET(CMAKE_STRIP /usr/bin/${SYSTRIPLET}-strip CACHE STRING "" FORCE)
SET(CMAKE_OBJCOPY /usr/bin/${SYSTRIPLET}-objcopy CACHE STRING "" FORCE)

set(CDX_TOOLCHAIN "gcc")

