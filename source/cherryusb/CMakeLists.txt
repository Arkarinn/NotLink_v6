cmake_minimum_required (VERSION 3.24)

project ("cherryusb" C)

add_library(${PROJECT_NAME}
	INTERFACE
)

target_sources(${PROJECT_NAME}
	INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/core/usbd_core.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/class/cdc/usbd_cdc_acm.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2/usb_dc_dwc2.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2/usb_glue_st.c"
)

target_compile_definitions(${PROJECT_NAME}
	INTERFACE
)

target_link_libraries(${PROJECT_NAME}
	INTERFACE
	"cmsis"
	"ulog"
	"hal"
	"bsp"
	"threadx"
)

target_include_directories(${PROJECT_NAME}
	INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/core"
	"${CMAKE_CURRENT_SOURCE_DIR}/common"
	"${CMAKE_CURRENT_SOURCE_DIR}/class/cdc"
	"${CMAKE_CURRENT_SOURCE_DIR}/class/hub"
	"${CMAKE_CURRENT_SOURCE_DIR}/port/dwc2"
)
