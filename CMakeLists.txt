cmake_minimum_required(VERSION 3.16)
project(MotionVideoPlayer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON) # Enable even if not using .ui files for consistency
set(CMAKE_AUTORCC ON) # Enable for potential future resource files
set(CMAKE_INCLUDE_CURRENT_DIR ON) # Allows including headers without src/ prefix

# Find Qt 6 - Requires CMAKE_PREFIX_PATH to be set if using Qt Online Installer
# Example: cmake .. -DCMAKE_PREFIX_PATH=/path/to/your/Qt/6.x.y/gcc_64
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Find OpenCV 4
find_package(OpenCV REQUIRED)

# --- Project Sources ---
set(PROJECT_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/VideoProcessor.cpp
    src/VideoDisplayWidget.cpp
)

set(PROJECT_HEADERS
    src/MainWindow.h
    src/VideoProcessor.h
    src/VideoDisplayWidget.h
)

# Define the executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OpenCV_LIBS} # From find_package(OpenCV)
)

# Include directories (OpenCV headers)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS} # From find_package(OpenCV)
)

# --- Optional: Installation ---
# install(TARGETS ${PROJECT_NAME} DESTINATION bin) # Basic install rule