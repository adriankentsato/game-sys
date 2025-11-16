# SDL3 Libraries

# Find SDL3
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.26)
endif()

# Find SDL3_image
find_package(SDL3_image QUIET)
if(NOT SDL3_image_FOUND)
    include_dependency(SDL3_image https://github.com/libsdl-org/SDL_image.git release-3.2.4)
endif()

# Find SDL3_ttf
find_package(SDL3_ttf QUIET)
if(NOT SDL3_ttf_FOUND)
    include_dependency(SDL3_ttf https://github.com/libsdl-org/SDL_ttf.git release-3.2.2)
endif()
