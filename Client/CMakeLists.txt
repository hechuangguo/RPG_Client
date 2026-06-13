cmake_minimum_required(VERSION 3.16)
project(RPGClient C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "RPGClient supports Windows PC only")
endif()

set(REPO_ROOT ${CMAKE_SOURCE_DIR}/..)
set(THIRD_PARTY_DIR ${REPO_ROOT}/3Party)

# --- tinyxml2 ---
set(TINYXML2_DIR ${THIRD_PARTY_DIR}/_build/tinyxml2-src)
if(NOT EXISTS ${TINYXML2_DIR}/tinyxml2.cpp)
    include(FetchContent)
    FetchContent_Declare(tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG        10.0.0
        SOURCE_DIR     ${TINYXML2_DIR}
        BINARY_DIR     ${THIRD_PARTY_DIR}/_build/tinyxml2-build
    )
    set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(tinyxml2)
endif()
if(NOT TARGET tinyxml2)
    add_library(tinyxml2 STATIC ${TINYXML2_DIR}/tinyxml2.cpp)
    target_include_directories(tinyxml2 PUBLIC ${TINYXML2_DIR})
endif()

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
    message(FATAL_ERROR "SFML not found. Run 3Party/download_and_build.ps1")
endif()
set(SFML_INCLUDE ${SFML_ROOT}/include)
set(SFML_LIBS sfml-graphics sfml-window sfml-system opengl32 winmm gdi32)

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
    ${REPO_ROOT}/Common
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sdk
    ${SDK_INCLUDE_DIRS}
    ${SFML_INCLUDE}
)

target_link_directories(RPGClient PRIVATE ${SFML_ROOT}/lib)

target_link_libraries(RPGClient PRIVATE
    ${SFML_LIBS}
    lua54 tinyxml2
    ws2_32
)

if(MSVC)
    target_compile_options(RPGClient PRIVATE /utf-8)
endif()

set_target_properties(RPGClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
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

# Copy SFML DLLs
file(GLOB SFML_DLLS ${SFML_ROOT}/bin/*.dll)
foreach(dll ${SFML_DLLS})
    add_custom_command(TARGET RPGClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dll} $<TARGET_FILE_DIR:RPGClient>
    )
endforeach()
