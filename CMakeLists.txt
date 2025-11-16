cmake_minimum_required(VERSION 3.22)
project(GameApp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_subdirectory(vendors)
add_subdirectory(src)
