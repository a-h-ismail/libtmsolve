cmake_minimum_required(VERSION 3.4.0)

project(tmsolve)

# Find source files
file(GLOB SOURCES src/*.c)

# Include header files
include_directories(include)

# Enable optimizations
add_compile_options(-O2)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Link to math library
target_link_libraries(${PROJECT_NAME} m)

# Build from the headers I got here
add_compile_definitions(LOCAL_BUILD)

# Install library
install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Install library headers
file(GLOB HEADERS include/*.h)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})