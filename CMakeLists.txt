cmake_minimum_required(VERSION 3.12)
project(SnapKey)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}) --- unnecessary
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
set(BUILD_SHARED_LIBS OFF)

add_executable(ThockTap ThockTap.cpp resources.rc)

if (WIN32)
    # Define UNICODE macros to ensure the 'W' (Wide) version of Windows API calls are used.
    target_compile_definitions(ThockTap PRIVATE UNICODE _UNICODE)
    
    # Optional: Force MSVC (Visual Studio Compiler) to treat source files as UTF-8
    # This is often needed if you don't use the L"" prefix or your VS Code is older.
    target_compile_options(ThockTap PRIVATE /utf-8)
    set_target_properties(ThockTap PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()