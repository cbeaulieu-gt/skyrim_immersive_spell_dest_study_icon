cmake_minimum_required(VERSION 3.21)
project(DIIIDestImmSpellLearningIcon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
    add_compile_options(/permissive- /W4 /EHsc /utf-8 /Zc:preprocessor)
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(WARNING "CMAKE_TOOLCHAIN_FILE was not set. Configure with vcpkg toolchain for dependency resolution.")
endif()

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/PapyrusBridge.cpp
    include/PapyrusBridge.h
    external/diii/DIII_API.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    CommonLibSSE::CommonLibSSE
    JsonCpp::JsonCpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        external/diii
)

# Optional convenience copy if SKYRIM_MODS_PATH points to a mod output folder.
if(DEFINED ENV{SKYRIM_MODS_PATH})
    set(SKSE_PLUGIN_OUT "$ENV{SKYRIM_MODS_PATH}/SKSE/Plugins")
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SKSE_PLUGIN_OUT}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${SKSE_PLUGIN_OUT}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION SKSE/Plugins
)
