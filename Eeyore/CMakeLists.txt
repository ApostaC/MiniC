include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(EeyoreParser Eeyore.y ${CMAKE_CURRENT_BINARY_DIR}/Eeyore.tab.cpp)
FLEX_TARGET(EeyoreLexer Eeyore.l ${CMAKE_CURRENT_BINARY_DIR}/Eeyore.yy.cpp)
ADD_FLEX_BISON_DEPENDENCY(EeyoreLexer EeyoreParser)

add_compile_options(-std=c++14 -O2)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

set(SOURCES Eeyore.cpp common.h Symbols.h Symbols.cpp Statement.h Statement.cpp Express.cpp Express.h)
ADD_EXECUTABLE(Eeyore ${SOURCES} ${BISON_EeyoreParser_OUTPUT_SOURCE} ${FLEX_EeyoreLexer_OUTPUTS})
