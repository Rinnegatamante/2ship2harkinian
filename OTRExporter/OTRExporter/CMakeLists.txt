set(PROJECT_NAME OTRExporter)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "AnimationExporter.h"
    "ArrayExporter.h"
    "AudioExporter.h"
    "BackgroundExporter.h"
    "BlobExporter.h"
	"CKeyFrameExporter.h"
    "CollisionExporter.h"
    "command_macros_base.h"
    "CutsceneExporter.h"
    "DisplayListExporter.h"
    "Exporter.h"
    "ExporterArchive.h"
    "ExporterArchiveO2R.h"
    "ExporterArchiveOTR.h"
    "Main.h"
    "MtxExporter.h"
    "PathExporter.h"
    "PlayerAnimationExporter.h"
    "RoomExporter.h"
    "SkeletonExporter.h"
    "SkeletonLimbExporter.h"
    "TextExporter.h"
    "TextMMExporter.h"
    "TextureExporter.h"
    "TextureAnimationExporter.h"
    "VersionInfo.h"
    "VtxExporter.h"
    "z64cutscene.h"
    "z64cutscene_commands.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "AnimationExporter.cpp"
    "ArrayExporter.cpp"
    "AudioExporter.cpp"
    "BackgroundExporter.cpp"
    "BlobExporter.cpp"
    "CollisionExporter.cpp"
	"CKeyFrameExporter.cpp"
    "CutsceneExporter.cpp"
    "DisplayListExporter.cpp"
    "Exporter.cpp"
    "ExporterArchive.cpp"
    "ExporterArchiveO2R.cpp"
    "ExporterArchiveOTR.cpp"
    "Main.cpp"
    "MtxExporter.cpp"
    "PathExporter.cpp"
    "PlayerAnimationExporter.cpp"
    "RoomExporter.cpp"
    "SkeletonExporter.cpp"
    "SkeletonLimbExporter.cpp"
    "TextExporter.cpp"
    "TextMMExporter.cpp"
    "TextureExporter.cpp"
    "TextureAnimationExporter.cpp"
    "VersionInfo.cpp"
    "VtxExporter.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
endif()

set(ROOT_NAMESPACE OTRExporter)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(${PROJECT_NAME} PROPERTIES
    	INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# MSVC runtime library
################################################################################
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
        string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        	$<$<CONFIG:Debug>:
        		MultiThreadedDebug
        	>
        	$<$<CONFIG:Release>:
        		MultiThreaded
        	>
        	$<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
        )
	set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})
endif()
################################################################################
# Compile definitions
################################################################################
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_definitions(${PROJECT_NAME} PRIVATE
        	"$<$<CONFIG:Debug>:"
        		"_DEBUG"
        	">"
        	"$<$<CONFIG:Release>:"
        		"NDEBUG"
        	">"
        	"_CONSOLE;"
        	"_CRT_SECURE_NO_WARNINGS;"
        	STORMLIB_NO_AUTO_LINK
        )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang|AppleClang")
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:Debug>:"
			"_DEBUG"
		">"
		"$<$<CONFIG:Release>:"
			"NDEBUG"
		">"
		"_CONSOLE;"
		"_CRT_SECURE_NO_WARNINGS;"
	)
endif()
################################################################################
# Compile and link options
################################################################################

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../ZAPDTR/ZAPD/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship/include
    # TODO: these should no longer be necessary if we were to link against LUS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libultraship/src/resource
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mm/2s2h
	.
)

find_package(nlohmann_json REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

FetchContent_Declare(
    StormLib
    GIT_REPOSITORY https://github.com/ladislav-zezula/StormLib.git
    GIT_TAG v9.25
)
FetchContent_MakeAvailable(StormLib)
target_include_directories(${PROJECT_NAME} PRIVATE ${stormlib_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Oi-
    	>
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
	    /O2
        >
        /permissive-;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
            /INCREMENTAL:NO
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU|Clang|AppleClang")
	target_compile_options(${PROJECT_NAME} PRIVATE
	-Wall -Wextra -Wno-error
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-unused-variable
	-Wno-missing-field-initializers
        -Wno-parentheses
        -Wno-narrowing
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-enum-enum-conversion>
	)
endif()
################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    libultraship
)

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

