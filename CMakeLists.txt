project(mpack)
SET(mpack_VERSION 0)
SET(mpack_SOVERSION 0.8.1)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

add_definitions("-D__STDC_LIMIT_MACROS")
add_definitions(-fPIC -DPIC )

set(mpack_HEADERS
	src/mpack/mpack-common.h
	src/mpack/mpack-config.h
	src/mpack/mpack-expect.h
	src/mpack/mpack.h
	src/mpack/mpack-node.h
	src/mpack/mpack-platform.h
	src/mpack/mpack-reader.h
	src/mpack/mpack-writer.h
)

set(mpack_SOURCES
	${npPosix_SOURCES}
	src/mpack/mpack-common.c
	src/mpack/mpack-expect.c
	src/mpack/mpack-node.c
	src/mpack/mpack-platform.c
	src/mpack/mpack-reader.c
	src/mpack/mpack-writer.c
)

add_library(mpack SHARED
	${mpack_HEADERS}
	${mpack_SOURCES}
)

SET_TARGET_PROPERTIES(mpack PROPERTIES
	LINKER_LANGUAGE C
	VERSION ${mpack_VERSION}
	SOVERSION ${mpack_SOVERSION}
)

install(TARGETS mpack DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${mpack_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mpack)
