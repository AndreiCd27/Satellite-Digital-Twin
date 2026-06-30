cmake_minimum_required(VERSION 3.15)

project(CRender VERSION 1.0.0)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           PYBIND11

# Python DIRS
set(PYBIND11_FINDPYTHON ON CACHE BOOL "Use CMake FindPython" FORCE)
set(Pybind11_PYTHON_FREE_THREADED OFF CACHE BOOL "Disable free-threaded Python" FORCE)

find_package(Python3 3.12 COMPONENTS Interpreter Development.Module Development.Embed REQUIRED)

include(FetchContent)
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11
  GIT_TAG        v2.13.1
)
FetchContent_MakeAvailable(pybind11)

#           GEOJSON

# Download nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

#           CMAKE CONFIG

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors by default
if(DEFINED WARNINGS_AS_ERRORS)
    # already defined
elseif(DEFINED ENV{WARNINGS_AS_ERRORS})
    set(WARNINGS_AS_ERRORS $ENV{WARNINGS_AS_ERRORS})
else()
    set(WARNINGS_AS_ERRORS ON)
endif()

if (WIN32) 
    if (MSVC)
        set(GLFW_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib/glfw/win-msvc/glfw3.lib)
        message("MSCV GLFW library used")
    else ()
        set(GLFW_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib/glfw/win-mingw/libglfw3.a)
        message("MINGW_64 GLFW library used")
    endif()
    set(EXTRA_LIBS gdi32 user32 shell32)
elseif (APPLE)
    set(GLFW_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib/glfw/macos/libglfw3.a)
    message("MACOS GLFW library used")

    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(QUARTZ_FRAMEWORK QuartzCore)
    set(EXTRA_LIBS ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK} ${QUARTZ_FRAMEWORK})
elseif (UNIX AND NOT APPLE)
    find_package(glfw3 REQUIRED)
    set(GLFW_LIB glfw)
    message("Linux GLFW library used")
endif()

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           EXTERNAL LIBRARIES

#Additional Cmakelists
add_subdirectory(Libraries/include/glm)
add_subdirectory(Libraries/include/TinyTIFF)

#add_subdirectory(Libraries/glfw)

add_library(glad src/glad.c)
target_sources(glad PRIVATE )
target_include_directories(glad PUBLIC Libraries/include)

file(GLOB IMGUI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib/imgui/*.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include/imgui")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include")

find_package(OpenGL REQUIRED)
#find_package(glfw3 REQUIRED)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           CRENDER EXTENSIONS

file(GLOB_RECURSE EXTENSION_SOURCES CONFIGURE_DEPENDS "CRenderExtensions/*.cpp")
file(GLOB_RECURSE EXTENSION_HEADERS CONFIGURE_DEPENDS "CRenderExtensions/*.h")

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           HEADER FILES

set(HEADER_FILES
    Headers/Engine3D.h
    Headers/Camera.h
    Headers/Scene.h
    
    
    Headers/GeometryBasics.h
    Headers/Texture.h
    
    Headers/VBO.h
    Headers/EBO.h
    Headers/VAO.h
    Headers/shaderClass.h
    
    Headers/Window.h
    Headers/ComputeShader.h
    Headers/Action.h
    Headers/EngineConfig.h

    Headers/Mutex.h
    Headers/Bindings.h

    Headers/GUI.h
    Headers/Export.h
)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           SOURCE FILES

add_executable(${PROJECT_NAME} src/Main.cpp
    src/Engine3D.cpp src/Camera.cpp
    src/Scene.cpp
    src/GeometryBasics.cpp
    src/Texture.cpp
    src/VBO.cpp src/EBO.cpp src/VAO.cpp
    src/shaderClass.cpp
    
    src/Window.cpp
    src/ComputeShader.cpp

    ${HEADER_FILES}
    ${EXTENSION_HEADERS}
    ${EXTENSION_SOURCES}
    
    src/Bindings.cpp
    src/GUI.cpp
    src/Export.cpp
)


# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           CONFIGURE PYBIND11

pybind11_add_module(py_engine3d src/Bindings.cpp)

set_target_properties(py_engine3d PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Link Python Module
target_include_directories(py_engine3d PRIVATE src/ Headers/ Libraries/include CRenderExtensions)
target_link_libraries(py_engine3d PRIVATE glad ${GLFW_LIB} OpenGL::GL Python3::Module)
target_compile_definitions(py_engine3d PRIVATE Py_GIL_USED=1)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           TARGET INCLUDE DIRECTORIES

target_include_directories(${PROJECT_NAME} PRIVATE CRenderExtensions)
target_include_directories(${PROJECT_NAME} PRIVATE Headers)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE Libraries/include)
target_include_directories(${PROJECT_NAME} PRIVATE Libraries/include/TinyTIFF)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/Libraries/include/TinyTIFF)
target_include_directories(${PROJECT_NAME} PRIVATE Libraries/include/imgui)


target_include_directories(${PROJECT_NAME} PRIVATE 
    "CRenderExtensions/Lighting/Headers"
    "CRenderExtensions/ImageService/Headers"
    "CRenderExtensions/ShadowAnalysis/Headers"
    "CRenderExtensions/PythonWorker/Headers"
    "Headers"
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${Python3_INCLUDE_DIRS})

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           TARGET LINK DIRECTORIES

#target_link_directories(${PROJECT_NAME} PRIVATE "${Python3_ROOT_DIR}/libs")
#target_link_directories(py_engine3d PRIVATE "${Python3_ROOT_DIR}/libs")

#target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_LIB} ${EXTRA_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glad
    OpenGL::GL
    TinyTIFF
    imgui
    ${CMAKE_DL_LIBS}
    Python3::Python
    pybind11::headers
    nlohmann_json::nlohmann_json
    #pybind11::embed
)

#target_link_libraries(py_engine3d PRIVATE ${Python3_LIBRARIES})

target_compile_definitions(${PROJECT_NAME} PRIVATE Py_GIL_USED=1)
target_compile_definitions(py_engine3d PRIVATE Py_GIL_USED=1)

# ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#           RESOURCES + SHADERS

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Python DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/CRenderExtensions DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/CRenderExtensions/PythonWorker/path_manage.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Python)

#           CMAKE SYMBOLIC LINK TO TEST_IMAGES & OUTPUT_IMAGES DIRECTORY

set(SRC_TEST_IMAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test_images")
set(BIN_TEST_IMAGES_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_images")

set(SRC_OUT_IMAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output_images")
set(BIN_OUT_IMAGES_DIR "${CMAKE_CURRENT_BINARY_DIR}/output_images")

# Symlink to test_images
if(EXISTS "${SRC_TEST_IMAGES_DIR}")
    if(NOT EXISTS "${BIN_TEST_IMAGES_DIR}")
        if(WIN32)
            file(TO_NATIVE_PATH "${SRC_TEST_IMAGES_DIR}" SRC_TEST_NATIVE)
            file(TO_NATIVE_PATH "${BIN_TEST_IMAGES_DIR}" BIN_TEST_NATIVE)
            execute_process(
                COMMAND cmd /c mklink /j "${BIN_TEST_NATIVE}" "${SRC_TEST_NATIVE}"
                RESULT_VARIABLE TEST_LINK_RESULT
                OUTPUT_QUIET ERROR_QUIET
            )
        else()
            file(CREATE_LINK "${SRC_TEST_IMAGES_DIR}" "${BIN_TEST_IMAGES_DIR}" SYMBOLIC)
        endif()
    endif()
else()
    message(WARNING "[CMake Warning] Directory test_images does not exist at: ${SRC_TEST_IMAGES_DIR}")
endif()

# Symlink to output_images
if(NOT EXISTS "${SRC_OUT_IMAGES_DIR}")
    file(MAKE_DIRECTORY "${SRC_OUT_IMAGES_DIR}")
endif()

if(NOT EXISTS "${BIN_OUT_IMAGES_DIR}")
    if(WIN32)
        file(TO_NATIVE_PATH "${SRC_OUT_IMAGES_DIR}" SRC_OUT_NATIVE)
        file(TO_NATIVE_PATH "${BIN_OUT_IMAGES_DIR}" BIN_OUT_NATIVE)
        execute_process(
            COMMAND cmd /c mklink /j "${BIN_OUT_NATIVE}" "${SRC_OUT_NATIVE}"
            RESULT_VARIABLE OUT_LINK_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
    else()
        file(CREATE_LINK "${SRC_OUT_IMAGES_DIR}" "${BIN_OUT_IMAGES_DIR}" SYMBOLIC)
    endif()
endif()

# Python DLLs for Windows...
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:Python3::Python>"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

#GITHUB ACTIONS

if(GITHUB_ACTIONS)
    message("NOTE: GITHUB_ACTIONS defined")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GITHUB_ACTIONS)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)