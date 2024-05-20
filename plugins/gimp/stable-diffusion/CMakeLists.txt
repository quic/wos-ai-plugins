# Enable Hot Reload for MSVC compilers if supported.
cmake_minimum_required(VERSION 3.28)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("gimp_sd_wos_plugin")

set (APP "gimp_sd_wos_plugin")
 option(BUILD_WITH_VCRUNTIME "Build the gimp_sd_wos_plugin with static vcruntime libraries." ON)
 message("Build gimp_sd_wos_plugin with vcruntime: ${BUILD_WITH_VCRUNTIME}")

if(MSVC)
  set(ADDITIONAL_CXX_FLAGS "/wd4013 /wd4018 /wd4028 /wd4047 /wd4068 /wd4090 /wd4101 /wd4113 /wd4133 /wd4190 /wd4244 /wd4267 /wd4305 /wd4477 /wd4996 /wd4819 /fp:fast")
  set(ADDITIONAL_C_FLAGS "/wd4013 /wd4018 /wd4028 /wd4047 /wd4068 /wd4090 /wd4101 /wd4113 /wd4133 /wd4190 /wd4244 /wd4267 /wd4305 /wd4477 /wd4996 /wd4819 /fp:fast")
  set(CMAKE_CXX_FLAGS "${ADDITIONAL_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")
  set(CMAKE_C_FLAGS "${ADDITIONAL_C_FLAGS} ${CMAKE_C_FLAGS}")
  string(REGEX REPLACE "/O2" "/Ox" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  string(REGEX REPLACE "/O2" "/Ox" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif()

find_package(flatbuffers REQUIRED)
find_package(dlfcn-win32 REQUIRED)
find_package(OpenCV REQUIRED)

# Add source to this project's executable.

include(GenerateExportHeader)

set (QNN_ROOT $ENV{QNN_SDK_ROOT})
set (QNN_INCLUDE_DIR "${QNN_ROOT}/include/QNN")
set (QNN_LIB_PREFIX "${QNN_ROOT}/lib")
set (_dtuple_POSTFIX "windows-msvc")
message("Linking with ARM64 QNN")
get_filename_component(QNN_IMPLIB_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnHtp.lib" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_HTP_DLL_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnHtp.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_HTP_PREPARE_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnHtpPrepare.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_STUB_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnHtpV73Stub.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_SKEL_PATH "${QNN_LIB_PREFIX}/hexagon-v73/unsigned/libQnnHtpV73Skel.so" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_HTP_NETRUN_EXTENSIONS_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnHtpNetRunExtensions.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(QNN_SYSTEM_DLL_PATH "${QNN_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/QnnSystem.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if( OpenCV_FOUND )
  # Additional Include Directories
  include_directories( ${OpenCV_INCLUDE_DIRS} )

  # Additional Library Directories
  link_directories( ${OpenCV_LIB_DIR} )

endif()

find_path(DIRENT_INCLUDE_DIRS "dirent.h")

message(Project=${PROJECT_SOURCE_DIR})

set(QNN_SDK_ROOT $ENV{QNN_SDK_ROOT})
set(SRC_PATH ${PROJECT_SOURCE_DIR}/src)

set(SDLIB_SOURCES
    ${SRC_PATH}/helpers/RuntimeApiHelpers.cpp
    ${SRC_PATH}/helpers/QnnApiHelpers.cpp
    ${SRC_PATH}/helpers/UiHelper.cpp
    ${SRC_PATH}/helpers/Helpers.cpp
    ${SRC_PATH}/helpers/GetOpt.cpp
    ${SRC_PATH}/helpers/Client.cpp
    ${SRC_PATH}/qnn/QnnApi.cpp
    ${SRC_PATH}/qnn/QnnApiUtils.cpp
    ${SRC_PATH}/qnn/BackendExtensions.cpp
    ${SRC_PATH}/qnn/ClientBuffer.cpp
    ${SRC_PATH}/qnn/IOTensor.cpp
    ${SRC_PATH}/qnn/RpcMem.cpp
    ${SRC_PATH}/data_loader/src/DataLoader.cpp
    ${SRC_PATH}/scheduler/src/Scheduler.cpp
    ${SRC_PATH}/stable_diffusion/src/StableDiffusionHelper.cpp
)

message(SDLIB_SOURCES ${SDLIB_SOURCES})
message(OPENCV_LIBS ${OpenCV_LIBS})
message(echo ${DIRENT_INCLUDE_DIRS})

# Set up the include directories
set(INCLUDE_DIRS
    ${DIRENT_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    ${QNN_SDK_ROOT}/include/QNN
    ${DIRENT_INCLUDE_DIRS}
    ${SRC_PATH}/qnn/include
    ${SRC_PATH}/data_loader/include
    ${SRC_PATH}/scheduler/include
    ${SRC_PATH}/helpers
    ${SRC_PATH}/stable_diffusion/include
)

# Add in the include directories for building this project.
include_directories(${INCLUDE_DIRS})
link_directories(${SRC_PATH}/tokenizer/target/aarch64-pc-windows-msvc/release)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQNN_ENABLE_API_2x -DQNN_ENABLE_API_2x_P3 -DQNN_ENABLE_API_2x_P2")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


add_library(QNN SHARED IMPORTED)
set_target_properties(QNN PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${QNN_INCLUDE_DIR}
	IMPORTED_IMPLIB ${QNN_IMPLIB_PATH}
	IMPORTED_LOCATION ${QNN_HTP_DLL_PATH}
)

add_executable (${APP} "src/main.cpp" ${SDLIB_SOURCES})

target_include_directories(${APP} PRIVATE ${DIRENT_INCLUDE_DIRS} INCLUDE_DIRS)
target_compile_definitions(${APP} PRIVATE DEBUG_ENABLE=1)
target_link_libraries(${APP} PRIVATE ${OpenCV_LIBS} dlfcn-win32::dl wsock32 ws2_32 flatbuffers::flatbuffers tokenizer.dll.lib)
target_link_libraries (${APP} PUBLIC QNN)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${APP} PROPERTY CXX_STANDARD 20)
endif()
