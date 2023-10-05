file(GLOB V2_1SOURCES ${PROJECT_SOURCE_DIR}/Version2_1/*.cpp)

add_executable(Version2_1 ${V2_1SOURCES})
add_compile_options(/O3)

target_link_libraries(Version2_1
PRIVATE
$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
