cmake_minimum_required(VERSION 3.15)

project(oop)

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

#Additional Cmakelists

add_subdirectory(Libraries/include/glm)

# Libraries
add_library(glad)
target_sources(glad PRIVATE src/glad.c)
target_include_directories(glad PUBLIC Libraries/include)

find_package(OpenGL REQUIRED)
#find_package(glfw3 REQUIRED)

# Main executable
set(HEADER_FILES Headers/Engine3D.h Headers/Camera.h Headers/GeometryOrganizer.h Headers/GeometryLoader.h
    Headers/GeometryBasics.h Headers/Texture.h Headers/MultiArray.h 
    Headers/VBO.h Headers/EBO.h Headers/VAO.h Headers/shaderClass.h
)
add_executable(${PROJECT_NAME} src/Main.cpp src/Engine3D.cpp src/Camera.cpp src/GeometryOrganizer.cpp
    src/GeometryLoader.cpp src/GeometryBasics.cpp src/Texture.cpp src/VBO.cpp src/EBO.cpp src/VAO.cpp src/shaderClass.cpp
    ${HEADER_FILES}
)
target_include_directories(${PROJECT_NAME} PRIVATE Headers Libraries/include)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glad 
    glfw3
    OpenGL::GL 
    ${CMAKE_DL_LIBS}
)
# Resources + Shaders

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


#GITHUB ACTIONS

if(GITHUB_ACTIONS)
    message("NOTE: GITHUB_ACTIONS defined")
    target_compile_definitions(${PROJECT_NAME} PRIVATE GITHUB_ACTIONS)
endif()

# Compiler flags
message("Compiler: ${CMAKE_CXX_COMPILER_ID} version ${CMAKE_CXX_COMPILER_VERSION}")
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /wd4244 /wd4267 /wd4996 /external:anglebrackets /external:W0)
    if(WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    if(WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

# Sanitizers Configuration
set(is_debug "$<CONFIG:Debug>")
set(is_rel_with_deb "$<CONFIG:RelWithDebInfo>")
set(debug_mode "$<OR:${is_debug},${is_rel_with_deb}>")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:/fsanitize=address>")
elseif(APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
    target_link_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
elseif(UNIX)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        # We leave GCC "clean" so Valgrind can run without shadow memory conflicts
        message("GCC detected: Sanitizers disabled to allow Valgrind usage.")
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        # Enable ASan for Clang 14 and all newer versions (15, 16, 17, 18, etc.)
        if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "14.0")
            target_compile_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
            target_link_options(${PROJECT_NAME} PRIVATE "$<${debug_mode}:-fsanitize=address,undefined>")
        else()
            message("Legacy Clang detected: Sanitizers skipped.")
        endif()
    endif()
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)