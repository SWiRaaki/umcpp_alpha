if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif()

cmake_minimum_required( VERSION 3.10 )

project( umcpp C )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Release )
endif()
set( CMAKE_C_FLAGS "-DMLIBC ${CMAKE_C_FLAGS}" )
set( CMAKE_C_FLAGS_DEBUG "-DDEBUG" )
set( CMAKE_C_FLAGS_RELEASE "-DRELEASE" )
# set( CMAKE_VERBOSE_MAKEFILE ON )

if( WIN32 )
	# message( STATUS "Build for Windows starts.." )
	add_definitions( -DPLATFORM_WINDOWS )
	set( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "/clr:nostdlib /GS- /GR- /EHsc /Wall /WX /permissive /wd4820 /wd4668 /wd5039 /wd4514 /wd5045" )
	set( OS_LIBS "" )
	set(
		OS_SRC
	)
elseif( UNIX )
	# message( STATUS "Build for Linux starts.." )
	add_definitions( -DPLATFORM_LINUX )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nodefaultlibs -fno-exceptions -pedantic -Wall -Wextra -Wconversion -Wreturn-type -Werror=all -Werror=extra -Werror=conversion -Werror=return-type -Wno-unused-function" )
	set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g2" )
	set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3" )
	set( OS_LIBS c m )
	set(
		OS_SRC
	)
else()
	set( OS_LIBS "" )
endif()

set( 
	SRCFILES
)
add_library(
	umka STATIC IMPORTED
)
set_target_properties(
	umka PROPERTIES
	IMPORTED_LOCATION             "$ENV{HOME}/pkg/umka/libumka.a"
	INTERFACE_INCLUDE_DIRECTORIES "$ENV{HOME}/pkg/umka/include"
)
add_executable(
	umcpp
	main.c
)
target_compile_definitions(
	umcpp
	PRIVATE UMKA_BUILD_UTEST
)
target_link_libraries(
	umcpp
	umka
	${OS_LIBS}
)
target_include_directories(
	umcpp
	PRIVATE inc/
)
set_property(
	TARGET umcpp
	PROPERTY C_STANDARD 99
)
set_property(
	TARGET umcpp
	PROPERTY OUTPUT_NAME umcpp
)
include_directories(
	inc/
)
