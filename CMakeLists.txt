cmake_minimum_required(VERSION 2.8)

project(CanonCamStreamSDL)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")

# SDL2
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Modules")
find_package(SDL2)
find_package(SDL2_image)
find_package(OpenGL)
find_package(GLEW)

# Python libraries for pyliaison
if (WIN32)
	add_definitions(-DGLEW_STATIC)
	#set(PYTHON_LIBRARY C:/Python35/libs/python35_d.lib)
	#set(PYTHON_INCLUDE_DIR C:/Python35/include)
	set(OCV_INCLUDE_DIR C:/Libraries/OpenCV3/include)
	set(OCV_LIB_DIR C:/Libraries/OpenCV3/lib/Win32/Debug)
	set(SDL2_LIBS ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARY} ${SDLMAIN_LIBRARY})
	set(GLM C:/Libraries/glm)
	set(EDSDK C:/Libraries/EDSDK34/EDSDK)
else(WIN32)
	set(PYTHON_LIBRARY /usr/local/lib/libpython3.5m.a)
	set(PYTHON_INCLUDE_DIR /usr/local/include/python3.5m)
	set(SDL2_LIBS ${SDL2_LIBRARY} ${SDL2IMAGE_INCLUDE_DIR})
	set(GLM /usr/local/include/glm)
endif(WIN32)

# Source files, include files, scripts
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.py)
file(GLOB SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*)

# Create source groups
source_group("Scripts" FILES ${SCRIPTS})
source_group("Shaders" FILES ${SHADERS})
source_group("Source" FILES ${SOURCES})
source_group("Include" FILES ${HEADERS})

# Pyliaison, which has its own folder and source file]
#file(GLOB PYL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pyl/*.cpp)
#file(GLOB PYL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/pyl/*.h)
#add_library(PyLiaison ${PYL_SOURCES} ${PYL_HEADERS})
#target_include_directories(PyLiaison PUBLIC ${PYL_HEADERS} ${PYTHON_INCLUDE_DIR} ${GLM})

# Add the CanonCamStreamSDL executable, which depends on source, include, and scripts
add_executable(CanonCamStreamSDL ${SOURCES} ${HEADERS} ${SCRIPTS} ${SHADERS})

# Make sure it gets its include paths
#target_include_directories(CanonCamStreamSDL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PYTHON_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/pyl ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM} ${EDSDK}/Header)
target_include_directories(CanonCamStreamSDL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${SDL2_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} ${GLM} ${EDSDK}/Header ${OCV_INCLUDE_DIR})
target_link_libraries(CanonCamStreamSDL LINK_PUBLIC ${SDL2_LIBS} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}  ${EDSDK}/Library/EDSDK.lib ${OCV_LIB_DIR}/opencv_core310d.lib ${OCV_LIB_DIR}/opencv_imgproc310d.lib ${OCV_LIB_DIR}/opencv_imgproc310d.lib ${OCV_LIB_DIR}/opencv_imgcodecs310d.lib ${OCV_LIB_DIR}/opencv_highgui310d.lib)
#target_link_libraries(CanonCamStreamSDL LINK_PUBLIC PyLiaison ${PYTHON_LIBRARY} ${SDL2_LIBS} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}  ${EDSDK}/Library/EDSDK.lib)
