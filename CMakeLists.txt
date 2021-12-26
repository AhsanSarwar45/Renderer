cmake_minimum_required(VERSION 3.16)

project(Renderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("Vendor/tinyobjloader")
add_subdirectory("Vendor/GLM")
add_subdirectory("Vendor/GLFW")

set(UtilitySourceFiles 
"Source/Utilities/tgaimage.cpp"
#"Source/Utilities/geometry.cpp"
"Source/Utilities/model.cpp"

)

add_executable(Lesson4
"Source/Lesson4/lesson4.cpp"
${UtilitySourceFiles}
)

target_include_directories(Lesson4 PUBLIC 
"Source"
"Vendor")


target_link_libraries(Lesson4 PUBLIC tinyobjloader)
target_link_libraries(Lesson4 PUBLIC glm::glm)
target_link_libraries(Lesson4 PUBLIC glfw)
target_link_libraries(Lesson4 PUBLIC opengl32)



