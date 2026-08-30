cmake_minimum_required(VERSION 3.16)

project(liona-local-ai LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Concurrent Widgets)
pkg_check_modules(Tesseract REQUIRED IMPORTED_TARGET tesseract)

add_executable(liona-local-ai
    include/ScreenSelector.h
    main.cpp
    src/MainApplication.cpp
    src/MainWindow.cpp
    src/OllamaConnection.cpp
    src/ScreenSelector.cpp
    src/TextRecognizer.cpp
)

target_compile_features(liona-local-ai PRIVATE cxx_std_17)
target_include_directories(liona-local-ai PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party"
)
target_link_libraries(liona-local-ai PRIVATE
    CURL::libcurl
    PkgConfig::Tesseract
    Qt6::Concurrent
    Qt6::Widgets
)
