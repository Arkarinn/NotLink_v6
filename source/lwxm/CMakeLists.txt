cmake_minimum_required (VERSION 3.24)

project ("lwxm" C)

file(GLOB SOURCE_FILES "*.c")

add_library(${PROJECT_NAME} 
	INTERFACE
)

target_sources(${PROJECT_NAME} 
	INTERFACE
	${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
	INTERFACE
	"ulog"
)

target_include_directories(${PROJECT_NAME}
	INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}"
)
