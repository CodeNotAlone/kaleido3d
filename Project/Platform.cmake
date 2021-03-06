if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "-fno-objc-arc -x objective-c++ ${CMAKE_CXX_FLAGS}")
    if(IOS)
        add_definitions(-DK3DPLATFORM_OS_IOS=1)
        set(K3D_TARGET_SYSTEM "iOS")
    elseif(ANDROID)
        add_definitions(-DK3DPLATFORM_OS_ANDROID=1)
        set(K3D_TARGET_SYSTEM "Android")
    else()
        set(MACOS TRUE)
        add_definitions(-DK3DPLATFORM_OS_MAC=1)
        set(K3D_TARGET_SYSTEM "MacOS")
		set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}")
		set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tsinstudio.kaleido3d")
		set(MACOSX_BUNDLE_COPYRIGHT "Copyright Tsin Studio 2016. All Rights Reserved.")
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore") # UWP Windows
    set(K3D_TARGET_SYSTEM "WindowUWP")
    set(WINUWP TRUE)
    add_definitions(-DK3DPLATFORM_OS_WINUWP=1)
    add_definitions(/ZW)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows") # Win32
    if(NOT ANDROID)
        add_definitions(-DK3DPLATFORM_OS_WIN=1)
        set(K3D_TARGET_SYSTEM "Window")
    else()
        add_definitions(-DK3DPLATFORM_OS_ANDROID=1)
        set(K3D_TARGET_SYSTEM "Android")
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(NOT ANDROID)
        set(LINUX TRUE)
        add_definitions(-DK3DPLATFORM_OS_LINUX=1)
        set(K3D_TARGET_SYSTEM "Linux")
    else()
        add_definitions(-DK3DPLATFORM_OS_ANDROID=1)
        set(K3D_TARGET_SYSTEM "Android")
    endif()
endif()

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
	set(TARGET_ARCH "64")
else()
	set(TARGET_ARCH "32")
endif()

message(STATUS "Host System = ${CMAKE_SYSTEM_NAME}, Target System = ${K3D_TARGET_SYSTEM}${TARGET_ARCH}")
