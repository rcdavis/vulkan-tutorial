project(App)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(spdlog)

add_executable(App
    src/Utils/DebugUtils.cpp
    src/Utils/Log.cpp
    src/Application.cpp
    src/Main.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(App PUBLIC
        DEBUG
        LOGGING_ENABLED
        ASSERTS_ENABLED
    )
endif()

target_include_directories(App PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Vulkan REQUIRED)

target_link_libraries(App
    Vulkan::Vulkan
    glm::glm
    glfw
    spdlog::spdlog_header_only
)
