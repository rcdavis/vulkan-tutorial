project(App)

add_executable(App
    src/Utils/Log.cpp
    src/Utils/VkUtils.cpp
    src/Application.cpp
    src/Main.cpp
    src/VmaImpl.cpp
    src/VolkImpl.cpp
    src/VulkanContext.cpp
)

target_compile_definitions(App PUBLIC
    GLFW_INCLUDE_NONE
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(App PUBLIC
        DEBUG
        LOGGING_ENABLED
        ASSERTS_ENABLED
    )
else()
    target_compile_definitions(App PUBLIC
        NDEBUG
    )
endif()

target_include_directories(App PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Vulkan REQUIRED)

target_link_libraries(App PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
    volk
    VulkanMemoryAllocator
    glm::glm
    spdlog::spdlog_header_only
)
