cmake_minimum_required(VERSION 3.12)

project(EMTest)

set(CMAKE_CXX_STANDARD 17)

set(IMGUI_SRC 
	3rdparty/IMGUI/imgui.cpp
	3rdparty/IMGUI/imgui_draw.cpp
	3rdparty/IMGUI/imgui_tables.cpp
	3rdparty/IMGUI/imgui_widgets.cpp
	3rdparty/IMGUI/backends/imgui_impl_sdl2.cpp
	3rdparty/IMGUI/backends/imgui_impl_opengl3.cpp
)
set(STB_IMAGE_SRC 
	3rdparty/stb_image/stb_image.cpp
)

add_executable(EMTest 
	${IMGUI_SRC}
	${STB_IMAGE_SRC}
	src/GLUtils.cpp
	src/AssetUtils.cpp
	src/GameLoop.cpp
	src/MapFilter.cpp
	src/MapViewer.cpp
	src/Main.cpp
)

target_include_directories(EMTest PRIVATE 
	3rdparty/glm/include
	3rdparty/IMGUI
	3rdparty/IMGUI/backends
	3rdparty/stb_image
	3rdparty/rapidjson
)

if(EMSCRIPTEN)
	target_compile_options(EMTest PRIVATE 
		-sUSE_SDL=2
		-sUSE_SDL_TTF=2
	)
	target_link_options(EMTest PRIVATE 
		-sUSE_SDL=2
		-sUSE_SDL_TTF=2
		-sWASM=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sALLOW_MEMORY_GROWTH=1
		--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets
	)
	target_compile_definitions(EMTest PRIVATE 
	)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/servre/index.html DESTINATION ${CMAKE_BINARY_DIR})
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/servre/start.py DESTINATION ${CMAKE_BINARY_DIR})
else()
	add_subdirectory(3rdparty/glad)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_TTF REQUIRED)
	target_link_libraries(EMTest PRIVATE
		SDL2::SDL2
		SDL2_ttf::SDL2_ttf
		glad
	)
	target_include_directories(EMTest PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_TTF_INCLUDE_DIRS}
	)
	target_compile_definitions(EMTest PRIVATE 
		SDL_MAIN_HANDLED
	)
	target_compile_options(EMTest PRIVATE
		/utf-8
	)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()
