# set mpack source files for library creation
set(MPACK_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-common.c
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-expect.c
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-node.c
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-platform.c
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-reader.c
	${CMAKE_CURRENT_SOURCE_DIR}/mpack-writer.c
	CACHE INTERNAL "mpack source"
)

# cmake trick to create both static and shared libraries with same output name
#  but with different target names.
#add_library(mpack-static  STATIC ${MPACK_SRC})
#add_library(mpack-dynamic SHARED ${MPACK_SRC})
#set_target_properties(mpack-static  PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
#set_target_properties(mpack-dynamic PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
