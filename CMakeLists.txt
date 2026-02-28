cmake_minimum_required(VERSION 3.16)

# Disable compiler checks that might fail
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

project(InvoicePrinterGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt packages (tries Qt6 first, falls back to Qt5)
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if (Qt6_FOUND)
    message(STATUS "Found Qt6")
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    message(STATUS "Found Qt5")
    set(QT_VERSION_MAJOR 5)
endif()

# Source files
set(PROJECT_SOURCES
    InvoicePrinterGUI.cpp
    InvoicePrinter.cpp
    SetupWindow.cpp
)

set(PROJECT_HEADERS
    InvoicePrinter.h
    SetupWindow.h
)

# Create executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Link Qt libraries
if (Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core 
        Qt6::Gui 
        Qt6::Widgets
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core 
        Qt5::Gui 
        Qt5::Widgets
    )
endif()

# ===== COPY TRUETYPE FONTS FOR VIETNAMESE SUPPORT =====
# Copy Roboto font files to output directory
set(FONT_FILES
    "${CMAKE_SOURCE_DIR}/Roboto-Regular.ttf"
    "${CMAKE_SOURCE_DIR}/Roboto-Bold.ttf"
)

foreach(FONT ${FONT_FILES})
    if(EXISTS ${FONT})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FONT}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
            COMMENT "Copying font file: ${FONT}"
        )
    else()
        message(WARNING "Font file not found: ${FONT}")
        message(WARNING "Please download Roboto fonts from https://fonts.google.com/specimen/Roboto")
        message(WARNING "and place them in ${CMAKE_SOURCE_DIR}/")
    endif()
endforeach()

if(WIN32)
    # Set the path to your manually downloaded Poppler directory
    set(POPPLER_DIR "C:/poppler-25.07.0")  # Change this to your actual path
    
    # Define all the DLLs needed from Library/bin
    set(POPPLER_DLLS
        "${POPPLER_DIR}/Library/bin/cairo.dll"
        "${POPPLER_DIR}/Library/bin/charset.dll"
        "${POPPLER_DIR}/Library/bin/deflate.dll"
        "${POPPLER_DIR}/Library/bin/expat.dll"
        "${POPPLER_DIR}/Library/bin/fontconfig-1.dll"
        "${POPPLER_DIR}/Library/bin/freetype.dll"
        "${POPPLER_DIR}/Library/bin/iconv.dll"
        "${POPPLER_DIR}/Library/bin/jpeg8.dll"
        "${POPPLER_DIR}/Library/bin/lcms2.dll"
        "${POPPLER_DIR}/Library/bin/Lerc.dll"
        "${POPPLER_DIR}/Library/bin/libcrypto-3-x64.dll"
        "${POPPLER_DIR}/Library/bin/libcurl.dll"
        "${POPPLER_DIR}/Library/bin/libexpat.dll"
        "${POPPLER_DIR}/Library/bin/liblzma.dll"
        "${POPPLER_DIR}/Library/bin/libpng16.dll"
        "${POPPLER_DIR}/Library/bin/libssh2.dll"
        "${POPPLER_DIR}/Library/bin/libtiff.dll"
        "${POPPLER_DIR}/Library/bin/libzstd.dll"
        "${POPPLER_DIR}/Library/bin/openjp2.dll"
        "${POPPLER_DIR}/Library/bin/pixman-1-0.dll"
        "${POPPLER_DIR}/Library/bin/poppler-cpp.dll"
        "${POPPLER_DIR}/Library/bin/poppler-glib.dll"
        "${POPPLER_DIR}/Library/bin/poppler.dll"
        "${POPPLER_DIR}/Library/bin/tiff.dll"
        "${POPPLER_DIR}/Library/bin/zlib.dll"
        "${POPPLER_DIR}/Library/bin/zstd.dll"
    )
    
    # Copy all DLLs to output directory
    foreach(DLL ${POPPLER_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying ${DLL}"
        )
    endforeach()
    
    # Copy pdftotext.exe
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${POPPLER_DIR}/Library/bin/pdftotext.exe"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying pdftotext.exe"
    )
    
    # Copy the share/poppler data directory (needed for encoding data)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${POPPLER_DIR}/share/poppler"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/share/poppler"
        COMMENT "Copying poppler data files"
    )
endif()

# ===== LIBHARU (PDF Library) Configuration =====
# Option 1: If installed via vcpkg
find_package(libharu CONFIG QUIET)
if(libharu_FOUND)
    message(STATUS "Found libharu via vcpkg")
    target_link_libraries(${PROJECT_NAME} PRIVATE hpdf)
else()
    # Option 2: Manual search
    find_library(HPDF_LIBRARY 
        NAMES hpdf libhpdf hpdfs libhpdfs
        PATHS 
            "C:/Program Files/libharu/lib"
            "C:/libharu/lib"
            "C:/libharu/include"
            "${CMAKE_SOURCE_DIR}/lib"
            "${CMAKE_SOURCE_DIR}/external/libharu/lib"
    )
    
    find_path(HPDF_INCLUDE_DIR 
        NAMES hpdf.h
        PATHS
            "C:/Program Files/libharu/include"
            "C:/libharu/include"
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/external/libharu/include"
    )
    
    if(HPDF_LIBRARY AND HPDF_INCLUDE_DIR)
        message(STATUS "Found libharu at: ${HPDF_LIBRARY}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${HPDF_LIBRARY})
        target_include_directories(${PROJECT_NAME} PRIVATE ${HPDF_INCLUDE_DIR})
    else()
        message(WARNING "libharu NOT found. PDF export will not work.")
        message(WARNING "Please install libharu using one of these methods:")
        message(WARNING "  1. vcpkg install libharu")
        message(WARNING "  2. Download from http://libharu.org/")
        message(WARNING "  3. Place libharu in external/libharu/ folder")
    endif()
endif()

# Windows specific: Link additional libraries that libharu might need
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    # Set Windows subsystem to avoid console window
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy Qt DLLs to output directory (Windows only)
if(WIN32)
    # Get Qt binary directory
    if(Qt6_FOUND)
        get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    else()
        get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    endif()
    
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    
    # Add custom command to deploy Qt after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe" 
            --release
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt..."
    )
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install font files alongside executable
install(FILES ${FONT_FILES}
    DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Invoice Printer GUI - Configuration")
message(STATUS "========================================")
message(STATUS "Qt Version: ${QT_VERSION_MAJOR}")
if(Qt6_FOUND)
    message(STATUS "Qt Location: ${Qt6_DIR}")
else()
    message(STATUS "Qt Location: ${Qt5_DIR}")
endif()
if(HPDF_LIBRARY)
    message(STATUS "LibHaru: Found")
else()
    message(STATUS "LibHaru: NOT FOUND (PDF export disabled)")
endif()
# Check if fonts exist
set(FONTS_FOUND TRUE)
foreach(FONT ${FONT_FILES})
    if(NOT EXISTS ${FONT})
        set(FONTS_FOUND FALSE)
    endif()
endforeach()
if(FONTS_FOUND)
    message(STATUS "TrueType Fonts: Found (Vietnamese support enabled)")
else()
    message(WARNING "TrueType Fonts: NOT FOUND")
    message(WARNING "Download Roboto fonts from: https://fonts.google.com/specimen/Roboto")
    message(WARNING "Place Roboto-Regular.ttf and Roboto-Bold.ttf in: ${CMAKE_SOURCE_DIR}/")
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Dir: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "========================================")
message(STATUS "")