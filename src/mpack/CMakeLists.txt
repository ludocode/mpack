# set cmake source dir and sourcefiles for library creation
set(MPACK_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "MPack source folder")
set(MPACK_SRC
	mpack-common.c
	mpack-expect.c
	mpack-internal.c
	mpack-node.c
	mpack-platform.c
	mpack-reader.c
	mpack-writer.c
)

# cmake trick to create both static and shared libraries with same output name
#  but with different target names.
add_library(mpack-static  STATIC ${MPACK_SRC})
add_library(mpack-dynamic SHARED ${MPACK_SRC})
set_target_properties(mpack-static  PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(mpack-dynamic PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
