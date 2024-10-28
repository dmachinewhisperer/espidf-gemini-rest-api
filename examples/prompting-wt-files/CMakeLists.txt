# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common"
						 "../../gemini-rest-api"
#						 "${CMAKE_CURRENT_LIST_DIR}/../libraries/coreMQTT"
#						 "${CMAKE_CURRENT_LIST_DIR}/../libraries/common/posix_compat"
	)

	
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(prompting-wt-files)
