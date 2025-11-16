set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3::SDL3
    imgui
#    ${OpenGL_gl_LIBRARIES}
)

if (WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3_image>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3_ttf>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()
