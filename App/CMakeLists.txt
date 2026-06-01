project(App)

add_executable(App
    src/Utils/Log.cpp
    src/Utils/VkUtils.cpp
    src/Application.cpp
    src/Main.cpp
    src/Mesh.cpp
    src/Platform.cpp
    src/ShaderData.cpp
    src/TextureData.cpp
    src/TinyObjImpl.cpp
    src/VmaImpl.cpp
    src/VulkanContext.cpp
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res
     DESTINATION ${CMAKE_BINARY_DIR}/App)

target_compile_definitions(App PRIVATE
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
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

if (PROJECT_PLATFORM STREQUAL "LINUX")
    target_compile_definitions(App PUBLIC
        PLATFORM_LINUX=1
    )
endif()

target_include_directories(App PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    "$ENV{VULKAN_SDK}/include"
)

target_compile_features(App PRIVATE cxx_std_20)

find_library(SLANG_LIBRARY
    NAMES slang
    HINTS
        "$ENV{VULKAN_SDK}/lib"
        "$ENV{VULKAN_SDK}/lib64"
)

find_package(Vulkan REQUIRED)

target_link_libraries(App PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
    volk
    VulkanMemoryAllocator
    glm::glm
    spdlog::spdlog_header_only
    tinyobjloader
    ktx
    ${SLANG_LIBRARY}
)
