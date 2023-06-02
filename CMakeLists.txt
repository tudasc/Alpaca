cmake_minimum_required(VERSION 3.0)

#das ist der name des projekts
project(api_analysis)

set(CMAKE_CXX_STANDARD 17)

#wir benötigen clang als library
find_package(Clang REQUIRED)
#version sollte 14.0.X sein
message(STATUS "Found LLVM/Clang version " ${LLVM_VERSION})

find_package(Clang REQUIRED)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${LLVM_CMAKE_DIR}"
  )

include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

set(LLVM_LINK_COMPONENTS support)

#wir wollen eine ausführbare datei erellen, die APIAnalysis heißt
#und aus der Datei APIAnalysis.cpp gebaut wird
add_executable(APIAnalysis APIAnalysis.cpp HelperFunctions.cpp header/HelperFunctions.h FunctionAnalyser.cpp header/FunctionAnalyser.h header/CodeMatcher.h CodeMatcher.cpp header/JSONFile.h header/OutputHandler.h ConsoleOutputHandler.cpp JSONOutputHandler.cpp header/JSONDefinitions/InsertAction.h header/JSONDefinitions/ReplaceAction.h header/JSONDefinitions/RemoveAction.h header/JSONDefinitions/JSONFunction.h header/VariableAnalyser.h VariableAnalyser.cpp header/JSONDefinitions/JSONVariable.h)


#wir wollen zu der executable die bibliothek clangTooling linken
target_link_libraries(APIAnalysis
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )

#wir wollen das die ausführbare datei keine run time type information generiert
#das ist notwendig weil die bibliothek (clangTooling) auch keine rtti hat, 
#und wir die gleiche configuration haben müssen
target_compile_options(APIAnalysis PUBLIC -fno-rtti -frtti)
