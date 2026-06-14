cmake_minimum_required(VERSION 3.16)
project(RPGClient C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "RPGClient supports Windows PC only")
endif()

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/3Party)
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/Common)

# --- Lua 5.4 ---
set(LUA_DIR ${THIRD_PARTY_DIR}/lua)
file(GLOB LUA_SOURCES ${LUA_DIR}/*.c)
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*/(lua|luac|ltests|onelua)\\.c$")
add_library(lua54 STATIC ${LUA_SOURCES})
target_include_directories(lua54 PUBLIC ${LUA_DIR})
if(MSVC)
    target_compile_definitions(lua54 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# --- SFML prebuilt ---
set(SFML_ROOT ${THIRD_PARTY_DIR}/sfml)
if(NOT EXISTS ${SFML_ROOT}/lib/sfml-graphics.lib)
    message(FATAL_ERROR "SFML not found. Run Client\\3Party\\download_and_build.ps1")
endif()
set(SFML_INCLUDE ${SFML_ROOT}/include)
set(SFML_MODULES graphics window system network audio)

# --- RPGClient ---
file(GLOB_RECURSE SDK_SRC CONFIGURE_DEPENDS sdk/*.cpp)

set(APP_DIRS app game lua net ui util)
set(APP_SRC ${CMAKE_SOURCE_DIR}/main.cpp)
foreach(dir ${APP_DIRS})
    file(GLOB_RECURSE _dir_src CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${dir}/*.cpp)
    list(APPEND APP_SRC ${_dir_src})
endforeach()

add_executable(RPGClient ${SDK_SRC} ${APP_SRC})

file(GLOB SDK_INCLUDE_DIRS LIST_DIRECTORIES true ${CMAKE_SOURCE_DIR}/sdk/*)

target_include_directories(RPGClient PRIVATE
    ${COMMON_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sdk
    ${SDK_INCLUDE_DIRS}
    ${SFML_INCLUDE}
)

target_link_directories(RPGClient PRIVATE ${SFML_ROOT}/lib)

target_link_libraries(RPGClient PRIVATE
    $<$<CONFIG:Debug>:sfml-graphics-d>
    $<$<CONFIG:Debug>:sfml-window-d>
    $<$<CONFIG:Debug>:sfml-system-d>
    $<$<NOT:$<CONFIG:Debug>>:sfml-graphics>
    $<$<NOT:$<CONFIG:Debug>>:sfml-window>
    $<$<NOT:$<CONFIG:Debug>>:sfml-system>
    opengl32 winmm gdi32
    lua54
    ws2_32
)

if(MSVC)
    target_compile_options(RPGClient PRIVATE /utf-8)
    set_target_properties(RPGClient PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(RPGClient PRIVATE "/ENTRY:mainCRTStartup")
endif()

set_target_properties(RPGClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:RPGClient>"
)

foreach(dir config script database basefile map assets)
    if(EXISTS ${CMAKE_SOURCE_DIR}/${dir})
        add_custom_command(TARGET RPGClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/${dir}
                $<TARGET_FILE_DIR:RPGClient>/${dir}
        )
    endif()
endforeach()

foreach(module ${SFML_MODULES})
    add_custom_command(TARGET RPGClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${SFML_ROOT}/bin/sfml-${module}-d-2.dll,${SFML_ROOT}/bin/sfml-${module}-2.dll>"
            "$<TARGET_FILE_DIR:RPGClient>"
    )
endforeach()
add_custom_command(TARGET RPGClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/openal32.dll"
        "$<TARGET_FILE_DIR:RPGClient>"
)
