cmake_minimum_required(VERSION 3.10.2)
add_library(glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
find_library(log-lib log)
set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

##Oboe
set(OBOE E:/androidlibs/oboe)
add_subdirectory(${OBOE} ./oboe)





##FFMpeg
set(FFMPEG_DIR E:/Android/mainprojects/FFMpeg/ffmpegandroid/build/${CMAKE_ANDROID_ARCH_ABI})

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavformat.so")

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavutil.so")

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libavcodec.so")

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION "${FFMPEG_DIR}/lib/libswresample.so")

###FFMPEG





add_library(main SHARED main.cpp AndroidEvents.cpp Context.cpp JavaCall.cpp Graphics.cpp Commons.h FileManager)
target_include_directories(main PUBLIC ${FFMPEG_DIR}/include ${OBOE}/include)
add_library(ui STATIC View.cpp ImageView.cpp VideoView.cpp ViewGroup.cpp)
target_include_directories(ui PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue ${OBOE}/include ${FFMPEG_DIR}/include ${OBOE}/include)

add_library(audio STATIC AudioTrack.cpp MediaCodec.cpp MediaPlayer.cpp)
target_include_directories(audio PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue ${OBOE}/include ${FFMPEG_DIR}/include)





target_link_libraries(main ui  audio oboe  avformat avutil avcodec swresample android  glue jnigraphics GLESv3 EGL ${log-lib})
