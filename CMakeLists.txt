# -*- mode: CMAKE; -*-

cmake_minimum_required(VERSION 3.0)

project( replxx VERSION 0.0.2 LANGUAGES CXX C )

option(REPLXX_BuildExamples "Build the examples." ON)
option(BUILD_SHARED_LIBS "Build as a shared library" ON)
option(BUILD_STATIC_LIBS "Build as a static library" ON)

if (REPLXX_BuildExamples AND NOT BUILD_STATIC_LIBS)
	set( BUILD_STATIC_LIBS ON )
endif()

set( CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build" )

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE string "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

message(STATUS "Build mode: ${CMAKE_BUILD_TYPE}")

# INFO
set(REPLXX_DISPLAY_NAME "replxx")
set(REPLXX_URL_INFO_ABOUT "https://github.com/AmokHuginnsson/replxx")
set(REPLXX_CONTACT "amok@codestation.org")
set(REPLXX_FRIENDLY_STRING "replxx - Read Evaluate Print Loop library")

# compiler options
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Compiler type GNU: ${CMAKE_CXX_COMPILER}")
	set(BASE_COMPILER_OPTIONS "-std=c++11 -Wall -Wextra -D_GNU_SOURCE -pthread")
	set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} ${BASE_COMPILER_OPTIONS}")
	set(CMAKE_CXX_FLAGS_COVERAGE       "${CMAKE_CXX_FLAGS} ${BASE_COMPILER_OPTIONS} -O0 --coverage -fno-inline -fno-default-inline -fno-inline-small-functions")
	set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${BASE_COMPILER_OPTIONS} -O0 -g -ggdb -g3 -ggdb3")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} ${BASE_COMPILER_OPTIONS} -Os")
	set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} ${BASE_COMPILER_OPTIONS} -O3 -fomit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${BASE_COMPILER_OPTIONS} -O3 -g")
	set(CMAKE_C_FLAGS "-std=c99")
elseif(CMAKE_COMPILER_IS_CLANGCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# using regular Clang or AppleClang
	message(STATUS "Compiler type CLANG: ${CMAKE_CXX_COMPILER}")
	set(BASE_COMPILER_OPTIONS "-std=c++11 -Wall -Wextra -D_GNU_SOURCE -pthread")
	set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} ${BASE_COMPILER_OPTIONS}")
	set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${BASE_COMPILER_OPTIONS} -O0 -g")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} ${BASE_COMPILER_OPTIONS} -Os")
	set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} ${BASE_COMPILER_OPTIONS} -O3 -fomit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${BASE_COMPILER_OPTIONS} -O3 -g")
	set(CMAKE_C_FLAGS "-std=c99")
elseif(MSVC)
	message(STATUS "Compiler type MSVC: ${CMAKE_CXX_COMPILER}")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS=1")

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /LTCG /ignore:4099")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SUBSYSTEM:CONSOLE /ignore:4099")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:CONSOLE /ignore:4099")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SUBSYSTEM:CONSOLE /ignore:4099")
else()
	# unknown compiler
	message(STATUS "Compiler type UNKNOWN: ${CMAKE_CXX_COMPILER}")
	set(BASE_COMPILER_OPTIONS "-std=c++11 -Wall -Wextra -pthread")
	set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} ${BASE_COMPILER_OPTIONS}")
	set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${BASE_COMPILER_OPTIONS} -O0 -g")
	set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL} ${BASE_COMPILER_OPTIONS} -Os")
	set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} ${BASE_COMPILER_OPTIONS} -O3 -fomit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${BASE_COMPILER_OPTIONS} -O3 -g")
	set(CMAKE_C_FLAGS "-std=c99")
endif()

# build libreplxx
set(
	REPLXX_SOURCES
	src/conversion.cxx
	src/ConvertUTF.cpp
	src/escape.cxx
	src/history.cxx
	src/replxx_impl.cxx
	src/io.cxx
	src/prompt.cxx
	src/replxx.cxx
	src/util.cxx
	src/wcwidth.cpp
	src/windows.cxx
)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(GenerateExportHeader)

if ( BUILD_STATIC_LIBS )
	add_library(replxx-static STATIC ${REPLXX_SOURCES})

	target_include_directories(
		replxx-static
		PUBLIC ${PROJECT_SOURCE_DIR}/include
		PRIVATE ${PROJECT_SOURCE_DIR}/src
	)
	generate_export_header(replxx-static)
	if ( MSVC )
		set_target_properties( replxx-static PROPERTIES RELEASE_OUTPUT_NAME replxx-static DEBUG_OUTPUT_NAME replxx-static-d )
		target_compile_definitions(replxx-static PRIVATE REPLXX_STATIC)
	else ()
		set_target_properties( replxx-static PROPERTIES RELEASE_OUTPUT_NAME replxx DEBUG_OUTPUT_NAME replxx-d )
	endif ()
	set( TARGETS replxx-static )
endif()

if ( BUILD_SHARED_LIBS )
	add_library(replxx SHARED ${REPLXX_SOURCES})
	add_library(Replxx::Replxx ALIAS replxx)

	target_include_directories(
		replxx
		PUBLIC ${PROJECT_SOURCE_DIR}/include
		PRIVATE ${PROJECT_SOURCE_DIR}/src
	)
	generate_export_header(replxx)
	set_target_properties( replxx PROPERTIES RELEASE_OUTPUT_NAME replxx DEBUG_OUTPUT_NAME replxx-d )
	set( TARGETS ${TARGETS} replxx )
	target_compile_definitions(replxx PRIVATE REPLXX_BUILDING_DLL)
endif()

# install
if ( MSVC )
				install( TARGETS ${TARGETS} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib )
else()
	install( TARGETS ${TARGETS} DESTINATION lib )
endif()

# headers
install(FILES include/replxx.hxx include/replxx.h DESTINATION include)

if (REPLXX_BuildExamples)
	# build example
	add_executable(
		example-c-api
		examples/c-api.c
		examples/util.c
	)

	add_executable(
		example-cxx-api
		examples/cxx-api.cxx
		examples/util.c
	)

	target_link_libraries(
		example-cxx-api
		PRIVATE replxx-static
	)

	if ( NOT MSVC )
		set( CXX_LIB stdc++ )
	endif()

	if (MSVC)
		target_compile_definitions(example-cxx-api PRIVATE REPLXX_STATIC)
		target_compile_definitions(example-c-api PRIVATE REPLXX_STATIC)
	endif()

	target_link_libraries(
		example-c-api
		PRIVATE replxx-static ${CXX_LIB}
	)
endif()

# packaging
include(CPack)

set(CPACK_SET_DESTDIR ON)

set(CPACK_PACKAGE_VENDOR  "codestation.org")
set(CPACK_PACKAGE_CONTACT "amok@codestation.org")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

set(CPACK_STRIP_FILES "ON")

set(CPACK_PACKAGE_NAME "replxx")
set(CPACK_DEBIAN_PACKAGE_SECTION "utilities")

