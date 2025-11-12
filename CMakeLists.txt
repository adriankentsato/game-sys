cmake_minimum_required(VERSION 3.30)
project(GameApp C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include the command that downloads libraries
include(FetchContent)

# Function to download and make available a library
function(include_dependency libName gitURL gitTag)
    # Download the library
    message(STATUS "Downloading ${libName}...")
    FetchContent_Declare(
        ${libName}
        GIT_REPOSITORY ${gitURL}
        GIT_TAG ${gitTag}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )

    # Make the library available
    FetchContent_MakeAvailable(${libName})
endfunction()

# Find SDL3
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.26)
else()
    message(STATUS "SDL3 found: ${SDL3_INCLUDE_DIRS}")
endif()

# Find SDL3_image
find_package(SDL3_image QUIET)
if(NOT SDL3_image_FOUND)
    include_dependency(SDL3_image https://github.com/libsdl-org/SDL_image.git release-3.2.4)
else()
    message(STATUS "SDL3_image found: ${SDL3_image_INCLUDE_DIRS}")
endif()

# Find SDL3_ttf
find_package(SDL3_ttf QUIET)
if(NOT SDL3_ttf_FOUND)
    include_dependency(SDL3_ttf https://github.com/libsdl-org/SDL_ttf.git release-3.2.2)
else()
    message(STATUS "SDL3_ttf found: ${SDL3_ttf_INCLUDE_DIRS}")
endif()

add_executable(GameApp main.c)

# Include SDL3 headers
target_include_directories(
    GameApp
    PUBLIC
    ${SDL3_INCLUDE_DIRS}
    ${SDL3_image_INCLUDE_DIRS}
    ${SDL3_ttf_INCLUDE_DIRS}
)

# Link SDL3 libraries
target_link_libraries(
    GameApp
    PUBLIC
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)


if (WIN32)
    add_custom_command(
        TARGET GameApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:GameApp>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3_image>" "$<TARGET_FILE_DIR:GameApp>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3_ttf>" "$<TARGET_FILE_DIR:GameApp>"
        VERBATIM
    )
endif()
