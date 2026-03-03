include(FetchContent)

find_package(Vulkan REQUIRED)

find_package(SDL2 QUIET CONFIG)
if(NOT SDL2_FOUND)
  if(NOT HAI_FETCH_DEPS)
    message(FATAL_ERROR "SDL2 was not found and HAI_FETCH_DEPS=OFF. Install SDL2 or enable dependency fetching.")
  endif()

  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.32.8
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL2)
endif()

if(NOT TARGET SDL2::SDL2)
  if(TARGET SDL2::SDL2-static)
    add_library(SDL2::SDL2 ALIAS SDL2::SDL2-static)
  elseif(TARGET SDL2::SDL2-shared)
    add_library(SDL2::SDL2 ALIAS SDL2::SDL2-shared)
  else()
    message(FATAL_ERROR "Could not determine SDL2 imported target name.")
  endif()
endif()

if(NOT HAI_FETCH_DEPS)
  message(FATAL_ERROR "ImGui setup in this project requires HAI_FETCH_DEPS=ON.")
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.8
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_MakeAvailable(imgui)
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_executable(home_automation_interface
  main.cpp
  ${IMGUI_SOURCES}
)

target_include_directories(home_automation_interface PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(home_automation_interface PRIVATE
  SDL2::SDL2
  Vulkan::Vulkan
)

target_compile_definitions(home_automation_interface PRIVATE
  SDL_MAIN_HANDLED
)
