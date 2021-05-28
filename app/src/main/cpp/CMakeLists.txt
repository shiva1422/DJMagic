cmake_minimum_required(VERSION 3.6.0)
add_library(glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
find_library(log-lib log)
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(main SHARED main.cpp AndroidEvents.cpp Context.cpp JavaCall.cpp Graphics.cpp)


##FFMpeg
set(FFMPEG_DIR E:/Android/mainprojects/FFMpeg/ffmpegandroid/build/${CMAKE_ANDROID_ARCH_ABI})
target_include_directories(main PUBLIC ${FFMPEG_DIR}/include)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavformat.so")

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavutil.so")

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavcodec.so")

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libswresample.so")

###FFMPEG


target_link_libraries(main avformat avutil avcodec swresample android  glue GLESv3 EGL ${log-lib})
