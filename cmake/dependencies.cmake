include(FetchContent)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.8
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
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
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG 1.4.350
)
FetchContent_MakeAvailable(volk)

set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "" FORCE)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)

# Optional settings
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG v4.4.2
)
FetchContent_MakeAvailable(ktx)
