cmake_minimum_required (VERSION 3.16)

# ============================================================================
# Merged Plugin - Combined ComponentInfo and IFCPropertyReader
# ============================================================================

# Find Python 3
find_package(Python3 REQUIRED COMPONENTS Interpreter)
message(STATUS "Using Python: ${Python3_EXECUTABLE} (version ${Python3_VERSION})")

function (SetCompilerOptions target)
	target_compile_features (${target} PUBLIC cxx_std_20)
	target_compile_options (${target} PUBLIC "$<$<CONFIG:Debug>:-DDEBUG>")
	if (WIN32)
		target_compile_options (${target} PUBLIC /W3 /WX /wd4996 /Zc:wchar_t-)
	else ()
		target_compile_options (${target} PUBLIC -Wall -Werror -fvisibility=hidden
			-Wno-multichar
			-Wno-ctor-dtor-privacy
			-Wno-invalid-offsetof
			-Wno-ignored-qualifiers
			-Wno-reorder
			-Wno-overloaded-virtual
			-Wno-unused-parameter
			-Wno-missing-field-initializers
			-Wno-unknown-pragmas
			-Wno-missing-braces
			-Wno-unused-private-field
			-Wno-return-std-move
			-Wno-unused-value
			-Wno-switch
			-Wno-deprecated
			-Wno-shorten-64-to-32)
	endif ()
endfunction ()

function (AddGSModuleIncludeFolder target folder)
	message (STATUS "AddGSModuleIncludeFolder ${folder}")
	target_include_directories (${target} PUBLIC "${folder}")
endfunction ()

function (AddGSModuleLinkLibrary target library)
	if (WIN32)
		file (GLOB libs "${library}/Win/*.lib")
		foreach (lib ${libs})
			message (STATUS "AddGSModuleLinkLibrary ${lib}")
			target_link_libraries (${target} ${lib})
		endforeach ()
	else ()
		message (STATUS "AddGSModuleLinkLibrary ${library}")
		target_link_libraries (${target} ${library})
	endif ()
endfunction ()

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component (CMAKE_CURRENT_FOLDER_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
set (CMAKE_SUPPRESS_REGENERATION 1)
set (CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)
if (WIN32)
	set (CMAKE_GENERATOR_TOOLSET v143)
	set (CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
else ()
	set (CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
	if (CMAKE_GENERATOR STREQUAL Xcode)
		set (CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
	endif ()
endif ()

# API DevKit：默认指向 HBIM2 下的 API.Development.Kit，可用 -DAC_API_DEVKIT_DIR= 覆盖
set (AC_API_DEVKIT_DIR "${CMAKE_CURRENT_LIST_DIR}/../API.Development.Kit.MAC.29.3100" CACHE PATH "API DevKit directory.")
set (AC_ADDON_NAME "${CMAKE_CURRENT_FOLDER_NAME}" CACHE STRING "Add-On name.")
set (AC_ADDON_LANGUAGE "INT" CACHE STRING "Add-On language code.")

message (STATUS "APIDevKit directory: ${AC_API_DEVKIT_DIR}")
set (ACAPINC_FILE_LOCATION ${AC_API_DEVKIT_DIR}/Support/Inc/ACAPinc.h)
if (EXISTS ${ACAPINC_FILE_LOCATION})
	file (READ ${ACAPINC_FILE_LOCATION} ACAPIncContent)
	string (REGEX MATCHALL "#define[ \t]+ServerMainVers_([0-9][0-9])" VersionList ${ACAPIncContent})
	set (ARCHICAD_VERSION ${CMAKE_MATCH_1})
	message (STATUS "Archicad Version: ${ARCHICAD_VERSION}")
else ()
	message (FATAL_ERROR "Failed to detect Archicad version, please check the value of the AC_API_DEVKIT_DIR variable.")
endif ()

if (WIN32)
	add_definitions (-DUNICODE -D_UNICODE -D_ITERATOR_DEBUG_LEVEL=0 -DGS_WIN)
else ()
	add_definitions (-Dmacintosh=1 -DGS_MAC)
endif ()
add_definitions (-DACExtension)

project (${AC_ADDON_NAME})

# 版本信息 - 自动日期格式版本号 (格式: 0.月.日.构建次数)
# 版本号可以通过CMake命令行参数传递，例如: -DAC_ADDON_VERSION_AUTO="0.2.1.1"
# 如果未提供，使用默认值 "0.0.0.0" 并在构建时通过构建脚本更新

# 默认版本（如果未从外部传入）
if (NOT DEFINED AC_ADDON_VERSION_AUTO)
    set (AC_ADDON_VERSION_AUTO "0.0.0.0" CACHE STRING "自动生成的日期格式版本号 (0.月.日.构建次数)")
endif()

# 解析版本字符串为各个组件
string(REPLACE "." ";" VERSION_PARTS ${AC_ADDON_VERSION_AUTO})
list(LENGTH VERSION_PARTS VERSION_PARTS_COUNT)

if (VERSION_PARTS_COUNT EQUAL 4)
    list(GET VERSION_PARTS 0 AC_ADDON_VERSION_MAJOR)
    list(GET VERSION_PARTS 1 AC_ADDON_VERSION_MINOR)
    list(GET VERSION_PARTS 2 AC_ADDON_VERSION_PATCH)
    list(GET VERSION_PARTS 3 AC_ADDON_VERSION_BUILD)
    
    # 保持向后兼容性的版本字符串 (仅前三个部分)
    set (AC_ADDON_VERSION "${AC_ADDON_VERSION_MAJOR}.${AC_ADDON_VERSION_MINOR}.${AC_ADDON_VERSION_PATCH}")
    
    # 完整的日期格式版本
    set (AC_ADDON_VERSION_FULL "${AC_ADDON_VERSION_AUTO}")
else()
    # 版本格式不正确，使用默认值
    message(WARNING "版本格式不正确: ${AC_ADDON_VERSION_AUTO}，应使用 0.月.日.构建次数 格式")
    set (AC_ADDON_VERSION_MAJOR 0)
    set (AC_ADDON_VERSION_MINOR 0)
    set (AC_ADDON_VERSION_PATCH 0)
    set (AC_ADDON_VERSION_BUILD 0)
    set (AC_ADDON_VERSION "0.0.0")
    set (AC_ADDON_VERSION_FULL "0.0.0.0")
endif()

message(STATUS "插件版本: ${AC_ADDON_VERSION_FULL} (格式: 0.月.日.构建次数)")
message(STATUS "  - 月: ${AC_ADDON_VERSION_MINOR}, 日: ${AC_ADDON_VERSION_PATCH}, 构建次数: ${AC_ADDON_VERSION_BUILD}")

# 将版本信息传递给编译器
add_definitions (-DAC_ADDON_VERSION_MAJOR=${AC_ADDON_VERSION_MAJOR})
add_definitions (-DAC_ADDON_VERSION_MINOR=${AC_ADDON_VERSION_MINOR})
add_definitions (-DAC_ADDON_VERSION_PATCH=${AC_ADDON_VERSION_PATCH})
add_definitions (-DAC_ADDON_VERSION_BUILD=${AC_ADDON_VERSION_BUILD})
add_definitions (-DAC_ADDON_VERSION_STRING="${AC_ADDON_VERSION_FULL}")

set (AddOnSourcesFolder ./Src)
set (AddOnResourcesFolder .)

# AddOnResources

set (ResourceObjectsDir ${CMAKE_BINARY_DIR}/ResourceObjects)

file (GLOB AddOnImageFiles
	${AddOnResourcesFolder}/RFIX/Images/*.svg
)
if (WIN32)
	file (GLOB AddOnResourceFiles
		${AddOnResourcesFolder}/R${AC_ADDON_LANGUAGE}/*.grc
		${AddOnResourcesFolder}/RFIX/*.grc
		${AddOnResourcesFolder}/RFIX.win/*.rc2
	)
else ()
	file (GLOB AddOnResourceFiles
		${AddOnResourcesFolder}/R${AC_ADDON_LANGUAGE}/*.grc
		${AddOnResourcesFolder}/RFIX/*.grc
		${AddOnResourcesFolder}/RFIX.mac/*.plist
	)
	# MergedPlugin 只使用 MergedPluginFix.grc 的 DATA 100（Selection_Test.html）。
	# IFCPropertyReaderFix.grc 也定义了 DATA 100（MinimalTest.html），会覆盖前者，导致面板只显示最小测试页。排除之。
	list (FILTER AddOnResourceFiles EXCLUDE REGEX "IFCPropertyReaderFix\\.grc$")
endif ()

source_group ("Images" FILES ${AddOnImageFiles})
source_group ("Resources" FILES ${AddOnResourceFiles})
add_custom_target (
	AddOnResources ALL
	DEPENDS "${ResourceObjectsDir}/AddOnResources.stamp"
	SOURCES ${AddOnResourceFiles} ${AddOnImageFiles}
)

get_filename_component (AddOnSourcesFolderAbsolute "${CMAKE_CURRENT_LIST_DIR}/${AddOnSourcesFolder}" ABSOLUTE)
get_filename_component (AddOnResourcesFolderAbsolute "${CMAKE_CURRENT_LIST_DIR}/${AddOnResourcesFolder}" ABSOLUTE)
get_filename_component (APIDevKitToolsFolderAbsolute "${AC_API_DEVKIT_DIR}/Support/Tools" ABSOLUTE)
if (WIN32)
	add_custom_command (
		OUTPUT "${ResourceObjectsDir}/AddOnResources.stamp"
		DEPENDS ${AddOnResourceFiles} ${AddOnImageFiles}
		COMMENT "Compiling resources..."
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ResourceObjectsDir}"
		COMMAND ${Python3_EXECUTABLE} "${APIDevKitToolsFolderAbsolute}/CompileResources.py" "${AC_ADDON_LANGUAGE}" "${AC_API_DEVKIT_DIR}" "${AddOnSourcesFolderAbsolute}" "${AddOnResourcesFolderAbsolute}" "${ResourceObjectsDir}" "${ResourceObjectsDir}/${AC_ADDON_NAME}.res"
		COMMAND ${CMAKE_COMMAND} -E touch "${ResourceObjectsDir}/AddOnResources.stamp"
	)
else ()
	# 确保仅 make 也能产出完整 bundle：先创建目录并复制 Info.plist，再编译资源
	add_custom_command (
		OUTPUT "${ResourceObjectsDir}/AddOnResources.stamp"
		DEPENDS ${AddOnResourceFiles} ${AddOnImageFiles}
		COMMENT "Compiling resources..."
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ResourceObjectsDir}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/$<CONFIG>/${AC_ADDON_NAME}.bundle/Contents/MacOS"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/$<CONFIG>/${AC_ADDON_NAME}.bundle/Contents/Resources"
		COMMAND ${CMAKE_COMMAND} -E copy "${AddOnResourcesFolderAbsolute}/RFIX.mac/Info.plist" "${CMAKE_BINARY_DIR}/$<CONFIG>/${AC_ADDON_NAME}.bundle/Contents/Info.plist"
		COMMAND ${Python3_EXECUTABLE} "${APIDevKitToolsFolderAbsolute}/CompileResources.py" "${AC_ADDON_LANGUAGE}" "${AC_API_DEVKIT_DIR}" "${AddOnSourcesFolderAbsolute}" "${AddOnResourcesFolderAbsolute}" "${ResourceObjectsDir}" "${CMAKE_BINARY_DIR}/$<CONFIG>/${AC_ADDON_NAME}.bundle/Contents/Resources"
		COMMAND ${CMAKE_COMMAND} -E copy "${AC_API_DEVKIT_DIR}/Support/Inc/PkgInfo" "${CMAKE_BINARY_DIR}/$<CONFIG>/${AC_ADDON_NAME}.bundle/Contents/PkgInfo"
		COMMAND ${CMAKE_COMMAND} -E touch "${ResourceObjectsDir}/AddOnResources.stamp"
	)
endif ()

# AddOn

file (GLOB AddOnHeaderFiles
	${AddOnSourcesFolder}/*.h
	${AddOnSourcesFolder}/*.hpp
)
file (GLOB AddOnSourceFiles
	${AddOnSourcesFolder}/*.c
	${AddOnSourcesFolder}/*.cpp
)
file (GLOB AllCFiles
	${AddOnSourcesFolder}/*.c
)
set_source_files_properties(${AllCFiles} PROPERTIES LANGUAGE CXX)
set (
	AddOnFiles
	${AddOnHeaderFiles}
	${AddOnSourceFiles}
)
source_group ("Sources" FILES ${AddOnFiles})
if (WIN32)
	add_library (AddOn SHARED ${AddOnFiles})
else ()
	add_library (AddOn MODULE ${AddOnFiles})
endif ()

set_target_properties (AddOn PROPERTIES OUTPUT_NAME ${AC_ADDON_NAME})
if (WIN32)
	set_target_properties (AddOn PROPERTIES SUFFIX ".apx")
else ()
	set_target_properties (AddOn PROPERTIES BUNDLE TRUE)
	set_target_properties (AddOn PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/${AddOnResourcesFolder}/RFIX.mac/Info.plist")
	set_target_properties (AddOn PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif ()

if (WIN32)
	target_link_options (AddOn PUBLIC "${ResourceObjectsDir}/${AC_ADDON_NAME}.res")
	target_link_options (AddOn PUBLIC /export:GetExportedFuncAddrs,@1 /export:SetImportedFuncAddrs,@2)
else ()
	target_link_options (AddOn PUBLIC -Wl,-u,_CheckEnvironment -Wl,-u,_RegisterInterface -Wl,-u,_Initialize -Wl,-u,_FreeData)
endif ()

target_include_directories (AddOn PUBLIC
	${AddOnSourcesFolder}
	${AC_API_DEVKIT_DIR}/Support/Inc
)

if (WIN32)
	target_link_libraries (AddOn
		"${AC_API_DEVKIT_DIR}/Support/Lib/ACAP_STAT.lib"
	)
else ()
	find_library (CocoaFramework Cocoa)
	target_link_libraries (AddOn
		"${AC_API_DEVKIT_DIR}/Support/Lib/libACAP_STAT.a"
		${CocoaFramework}
	)
endif ()

SetCompilerOptions (AddOn)
add_dependencies (AddOn AddOnResources)

# macOS: 链接前确保 bundle 目录存在，避免 ld: open() failed；构建后强制校验 bundle 完整性，不完整则构建失败
if (NOT WIN32)
	add_custom_command (TARGET AddOn PRE_LINK COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AddOn>" COMMENT "确保 bundle Contents/MacOS 存在")
	add_custom_command (TARGET AddOn POST_BUILD
		COMMAND "${CMAKE_SOURCE_DIR}/check_bundle.sh" "$<TARGET_FILE_DIR:AddOn>/../.."
		COMMENT "校验 bundle 完整性，缺失则构建失败"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
endif ()

get_filename_component (APIDevKitModulesDir "${AC_API_DEVKIT_DIR}/Support/Modules" ABSOLUTE)

file (GLOB GSIncludeFolders
	${APIDevKitModulesDir}/*
)
foreach (includeFolder ${GSIncludeFolders})
	get_filename_component(folderName ${includeFolder} NAME)
	if (NOT folderName STREQUAL ".DS_Store")
		AddGSModuleIncludeFolder (AddOn ${includeFolder})
	endif ()
endforeach ()

if (WIN32)
	set (APIDevKitLinkLibDir "${APIDevKitModulesDir}")
else ()
	get_filename_component (APIDevKitLinkLibDir "${AC_API_DEVKIT_DIR}/Support/Frameworks" ABSOLUTE)
endif ()

file (GLOB GSLinkLibFolders
	${APIDevKitLinkLibDir}/*
)
foreach (linkLibFolder ${GSLinkLibFolders})
	get_filename_component(libName ${linkLibFolder} NAME)
	if (NOT libName STREQUAL ".DS_Store")
		AddGSModuleLinkLibrary (AddOn ${linkLibFolder})
	endif ()
endforeach ()

get_filename_component(new_hpp "${APIDevKitModulesDir}/GSRoot/GSNew.hpp" REALPATH)
get_filename_component(malloc_hpp "${APIDevKitModulesDir}/GSRoot/GSMalloc.hpp" REALPATH)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(
        AddOn PRIVATE
        "SHELL:/FI \"${new_hpp}\""
        "SHELL:/FI \"${malloc_hpp}\""
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang\$")
    target_compile_options(
        AddOn PRIVATE
        "SHELL:-include \"${new_hpp}\""
        "SHELL:-include \"${malloc_hpp}\""
    )
else()
    message(FATAL_ERROR "Unknown compiler ID.")
endif()