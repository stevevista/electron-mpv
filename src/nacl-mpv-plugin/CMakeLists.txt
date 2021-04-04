
# cmake -G "Visual Studio 15" -A x64 .. -DNAC_SDK_DIR=D:\dev\nacl_sdk
# cmake --build . --config Release -- /p:CharacterSet=Unicode

cmake_minimum_required(VERSION 3.9)

cmake_policy (SET CMP0091 NEW)

# Every project needs a name.  We call this the "examples" project.
project(pepper-ffplay)

# -DTHIRD_PARTY_DIR
# -DFFMPEG_PREBUILD_DIR
# -DNAC_SDK_DIR
# -DPEPPER_VERSION


if (NOT NAC_SDK_DIR)
  find_path(NAC_SDK_DIR 
    pepper_${PEPPER_VERSION}/include/ppapi/cpp/core.h
    C:/nacl_sdk
    D:/nacl_sdk
    E:/nacl_sdk
    F:/nacl_sdk
    C:/dev/nacl_sdk
    D:/dev/nacl_sdk
    E:/dev/nacl_sdk
    F:/dev/nacl_sdk)
endif()

set(THIRD_PARTY_PREBUILT ${THIRD_PARTY_DIR}/prebuilt)

if (NOT FFMPEG_PREBUILD_DIR)
  set(FFMPEG_PREBUILD_DIR ${THIRD_PARTY_PREBUILT})
endif()

set(FFMPEG_INCLUDE_PATH ${FFMPEG_PREBUILD_DIR}/include)
set(FFMPEG_LIB_PATH ${FFMPEG_PREBUILD_DIR}/lib)

set(THIRD_INC_PATH ${THIRD_PARTY_PREBUILT}/include)
if (CMAKE_CL_64)
  set(THIRD_LIB_PATH ${THIRD_PARTY_PREBUILT}/lib/x64)
else()
  set(THIRD_LIB_PATH ${THIRD_PARTY_PREBUILT}/lib/x86)
endif()

set (ALL_FFMPEG_LIBS
    avcodec
    avformat
    avfilter
    avutil
    swresample
    swscale)

set (ALL_FFMPEG_LIBS_DBG
    avcodec$<$<CONFIG:Debug>:d>
    avformat$<$<CONFIG:Debug>:d>
    avfilter$<$<CONFIG:Debug>:d>
    avutil$<$<CONFIG:Debug>:d>
    swresample$<$<CONFIG:Debug>:d>
    swscale$<$<CONFIG:Debug>:d>)

set(_PEPPER_VERSION_NAME pepper_${PEPPER_VERSION})

if (WIN32)
  if (CMAKE_CL_64)
    set(ARCHSUFFIX "win32-x64")
  else()
    set(ARCHSUFFIX "win32-x86")
  endif()
else()
  set(ARCHSUFFIX "linux-x64")
endif()

set(NACL_INCLUDE_PATH ${NAC_SDK_DIR}/${_PEPPER_VERSION_NAME}/include)
set(NACL_LIB_PATH ${NAC_SDK_DIR}/${_PEPPER_VERSION_NAME}/lib/win_x86_64_host/Release)

macro(disable_msvc_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W3 /WX- 
      /wd"4005"
      /wd"4018"
      /wd"4047"
      /wd"4090"
      /wd"4101"
      /wd"4146"
      /wd"4229"
      /wd"4244"
      /wd"4267"
      /wd"4305"
      /wd"4334"
      /wd"4819"
      /wd"4996")
  endif()
endmacro()

macro(mpv_compile_definitions target)
  disable_msvc_warnings(${target})
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE MPV_CUSTOMIZE_MSVC_WORKAROUND MPV_CUSTOMIZE_USE_HOT_DETECTION)
  target_compile_definitions(${target} PRIVATE WIN32 _WINDOWS _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NDEBUG)
  target_compile_definitions(${target} PRIVATE _WIN32_WINNT=0x0602 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS COBJMACROS)
  target_compile_definitions(${target} PRIVATE __thread=__declspec\(thread\) restrict=__restrict WINBOOL=BOOL HAVE_TA_LEAK_REPORT=0)
  target_compile_definitions(${target} PRIVATE __MINGW32__)
  target_compile_definitions(${target} PRIVATE strcasecmp=_stricmp strncasecmp=_strnicmp)
endmacro()

#-----------------------------------------------------------------------------------------
# yolo4-tiny-detection
#-----------------------------------------------------------------------------------------
add_library(yolo4-tiny-detection SHARED
    src/detection/gemm.cc
    src/detection/tiny-network.c
    src/detection/detection-service.cc
    src/yolo4-tiny-detection.def
    )

target_compile_options(yolo4-tiny-detection PRIVATE /openmp)

target_compile_definitions(yolo4-tiny-detection PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(yolo4-tiny-detection PRIVATE /W3 /WX- 
      /wd"4244"
      /wd"4819"
      /wd"4267")

target_include_directories(yolo4-tiny-detection PRIVATE
  ${FFMPEG_INCLUDE_PATH})

target_link_directories(yolo4-tiny-detection PRIVATE
    ${FFMPEG_LIB_PATH})

target_link_libraries(yolo4-tiny-detection PRIVATE
    avutil
    swscale
    avcodec)

#-----------------------------------------------------------------------------------------
#  main target
#-----------------------------------------------------------------------------------------
set(_TARGET_MPV "mpv-${ARCHSUFFIX}-${_PEPPER_VERSION_NAME}")

set(MPV_INCLUDE_DIRS
  src/mpv-patched
  src/mpv/osdep/win32/include
  src/mpv
  src
  )

set(MPV_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/mpv")

#----------------------------------------------------------------------------------------------------------------------
# mpv sources
#----------------------------------------------------------------------------------------------------------------------
set(MPV_BASIC_SOURCES
  ${MPV_SRC_PATH}/osdep/win32/pthread.c
  src/mpv/osdep/timer.c
  src/mpv/osdep/timer-win2.c
  src/mpv/osdep/glob-win.c
  src/mpv/osdep/path-win.c
  src/mpv/osdep/windows_utils.c
  src/mpv/osdep/threads.c
  src/mpv/osdep/io.c
  src/mpv/osdep/w32_keyboard.c
  src/mpv/osdep/terminal-win.c
  src/mpv/osdep/subprocess.c
  src/mpv/osdep/subprocess-win.c
  src/mpv/ta/ta.c
  src/mpv/ta/ta_talloc.c
  src/mpv/ta/ta_utils.c
  src/mpv/misc/bstr.c
  src/mpv/misc/charset_conv.c
  src/mpv/misc/dispatch.c
  src/mpv/misc/rendezvous.c
  src/mpv/misc/json.c
  src/mpv/misc/node.c
  src/mpv/misc/thread_pool.c
  src/mpv/misc/thread_tools.c
  src/mpv/misc/natural_sort.c
  src/mpv/common/av_common.c
  src/mpv/common/av_log.c
  src/mpv/common/codecs.c
  src/mpv/common/common.c
  src/mpv/common/version.c
  src/mpv/common/msg.c
  src/mpv/common/playlist.c
  src/mpv/common/tags.c
  src/mpv/common/stats.c
  src/mpv/common/encode_lavc.c
  src/mpv/common/recorder.c
  src/mpv/input/cmd.c
  src/mpv/input/keycodes.c
  src/mpv/input/input.c
  src/mpv/input/ipc.c
  src/mpv/input/ipc-win.c
  src/mpv/input/event.c
  src/mpv/options/m_config_core.c
  src/mpv/options/m_config_frontend.c
  src/mpv/options/m_option.c
  src/mpv/options/m_property.c
  src/mpv/options/parse_commandline.c
  src/mpv/options/parse_configfile.c
  src/mpv/options/path.c
  src/mpv/stream/cookies.c
  src/mpv/stream/stream.c
  src/mpv/stream/stream_avdevice.c
  src/mpv/stream/stream_file.c
  src/mpv/stream/stream_memory.c
  src/mpv/stream/stream_mf.c
  src/mpv/stream/stream_null.c
  src/mpv/stream/stream_concat.c
  src/mpv/stream/stream_cb.c
  src/mpv/stream/stream_slice.c
  src/mpv/stream/stream_edl.c
  src/mpv/options/options.c
  src/mpv/demux/codec_tags.c
  src/mpv/demux/cache.c
  src/mpv/demux/ebml.c
  src/mpv/demux/cue.c
  src/mpv/demux/timeline.c
  src/mpv/demux/packet.c
  src/mpv/demux/demux.c
  src/mpv/demux/demux_disc.c
  src/mpv/demux/demux_mf.c
  src/mpv/demux/demux_cue.c
  src/mpv/demux/demux_edl.c
  src/mpv/demux/demux_playlist.c
  src/mpv/demux/demux_mkv.c
  src/mpv/demux/demux_mkv_timeline.c
  src/mpv/demux/demux_null.c
  src/mpv/demux/demux_raw.c
  src/mpv/demux/demux_timeline.c
  src/mpv/demux/demux_lavf.c
  src/mpv/filters/filter.c
  src/mpv/filters/f_lavfi.c
  src/mpv/filters/f_demux_in.c
  src/mpv/filters/f_swscale.c
  src/mpv/filters/f_swresample.c
  src/mpv/filters/f_auto_filters.c
  src/mpv/filters/f_async_queue.c
  src/mpv/filters/f_hwtransfer.c
  src/mpv/filters/f_output_chain.c
  src/mpv/filters/f_autoconvert.c
  src/mpv/filters/f_utils.c
  src/mpv/filters/frame.c
  src/mpv/filters/user_filters.c
  src/mpv/sub/ass_mp.c
  src/mpv/sub/osd.c
  src/mpv/sub/osd_libass.c
  src/mpv/sub/sd_ass.c
  src/mpv/sub/dec_sub.c
  src/mpv/sub/img_convert.c
  src/mpv/sub/sd_lavc.c
  src/mpv/sub/lavc_conv.c
  src/mpv/sub/filter_sdh.c
  src/mpv/sub/draw_bmp.c
  src/mpv/audio/aframe.c
  src/mpv/audio/chmap.c
  src/mpv/audio/chmap_sel.c
  src/mpv/audio/fmt-conversion.c
  src/mpv/audio/format.c
  src/mpv/audio/decode/ad_lavc.c
  src/mpv/audio/decode/ad_spdif.c
  src/mpv/audio/filter/af_drop.c
  src/mpv/audio/filter/af_format.c
  src/mpv/audio/filter/af_scaletempo.c
  src/mpv/audio/filter/af_scaletempo2.c
  src/mpv/audio/filter/af_scaletempo2_internals.c
  src/mpv/audio/filter/af_lavcac3enc.c
  src/mpv/audio/out/ao.c
  src/mpv/audio/out/ao_null.c
  src/mpv/audio/out/ao_wasapi.c
  src/mpv/audio/out/ao_wasapi_changenotify.c
  src/mpv/audio/out/ao_wasapi_utils.c
  src/mpv/audio/out/ao_lavc.c
  src/mpv/audio/out/ao_pcm.c
  src/mpv/audio/out/buffer.c
  src/mpv/video/csputils.c
  src/mpv/video/repack.c
  src/mpv/video/d3d.c
  src/mpv/video/cuda.c
  src/mpv/video/mp_image.c
  src/mpv/video/mp_image_pool.c
  src/mpv/video/fmt-conversion.c
  src/mpv/video/hwdec.c
  src/mpv/video/image_loader.c
  src/mpv/video/image_writer.c
  src/mpv/video/sws_utils.c
  src/mpv/video/img_format.c
  src/mpv/video/decode/vd_lavc.c
  src/mpv/video/filter/refqueue.c
  src/mpv/video/filter/vf_format.c
  src/mpv/video/filter/vf_sub.c
  src/mpv/video/filter/vf_d3d11vpp.c
  src/mpv/video/out/aspect.c
  src/mpv/video/out/filter_kernels.c
  src/mpv/video/out/bitmap_packer.c
  src/mpv/video/out/dr_helper.c
  src/mpv/video/out/dither.c
  src/mpv/video/out/vo.c
  src/mpv/video/out/vo_null.c
  src/mpv/video/out/vo_image.c
  src/mpv/video/out/vo_lavc.c
  src/mpv/video/out/vo_tct.c
  src/mpv/video/out/vo_libmpv.c
  src/mpv/video/out/libmpv_sw.c
  src/mpv/video/out/win_state.c
  src/mpv/video/out/w32_common.c
  src/mpv/video/out/win32/droptarget.c
  src/mpv/video/out/win32/displayconfig.c
  src/mpv/video/out/gpu/ra.c
  src/mpv/video/out/gpu/utils.c
  src/mpv/video/out/gpu/video.c
  src/mpv/video/out/gpu/osd.c
  src/mpv/video/out/gpu/error_diffusion.c
  src/mpv/video/out/gpu/user_shaders.c
  src/mpv/video/out/gpu/libmpv_gpu.c
  src/mpv/video/out/opengl/libmpv_gl.c
  src/mpv/video/out/opengl/context.c
  src/mpv/player/audio.c
  src/mpv/player/video.c
  src/mpv/player/client.c
  src/mpv/player/command.c
  src/mpv/player/main.c
  src/mpv/player/misc.c
  src/mpv/player/osd.c
  src/mpv/player/sub.c
  src/mpv/player/playloop.c
  src/mpv/player/screenshot.c
  src/mpv/player/configfiles.c
  src/mpv/player/external_files.c
  src/mpv/player/scripting.c
  src/sd_ass_ai.c
  src/mpv-patched/stream/stream_lavf.c
  src/mpv-patched/filters/f_decoder_wrapper.c
  src/mpv-patched/player/loadfile.c)

set_source_files_properties(src/mpv/sub/sd_ass.c PROPERTIES COMPILE_FLAGS /Dsd_ass=orgin_sd_ass)

add_library(mpv_common_objs OBJECT ${MPV_BASIC_SOURCES})
mpv_compile_definitions(mpv_common_objs)

target_include_directories(mpv_common_objs PRIVATE
  ${MPV_INCLUDE_DIRS}
  ${FFMPEG_INCLUDE_PATH}
  ${THIRD_INC_PATH}
  build)

set(MPV_SOURCES
  src/mpv/video/out/gpu/video_shaders.c
  src/mpv/video/out/gpu/shader_cache.c
  src/mpv/video/out/gpu/hwdec.c
  src/mpv/video/out/gpu/lcms.c
  src/mpv/video/out/gpu/spirv.c
  src/mpv/video/out/opengl/formats.c
  src/mpv/video/out/opengl/common.c
  src/mpv/video/out/opengl/utils.c
  src/mpv/video/out/opengl/ra_gl.c
  src/mpv/video/out/opengl/hwdec_dxva2egl.c
  src/mpv/video/out/opengl/angle_dynamic.c
  src/mpv/video/out/opengl/hwdec_d3d11egl.c
  src/stubs.c
  )

#
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/etc")
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/sub")

execute_process(COMMAND python "${MPV_SRC_PATH}/TOOLS/file2string.py" "${MPV_SRC_PATH}/etc/builtin.conf" OUTPUT_FILE "${PROJECT_BINARY_DIR}/generated/etc/builtin.conf.inc")
execute_process(COMMAND python "${MPV_SRC_PATH}/TOOLS/file2string.py" "${MPV_SRC_PATH}/etc/input.conf" OUTPUT_FILE "${PROJECT_BINARY_DIR}/generated/etc/input.conf.inc")
execute_process(COMMAND python "${MPV_SRC_PATH}/TOOLS/file2string.py" "${MPV_SRC_PATH}/sub/osd_font.otf" OUTPUT_FILE "${PROJECT_BINARY_DIR}/generated/sub/osd_font.otf.inc")

add_library(mpv-core SHARED 
  $<TARGET_OBJECTS:mpv_common_objs>
  ${MPV_SOURCES}
  src/detection/ai_render.c
  src/mpv-core.def)

disable_msvc_warnings(mpv-core)

mpv_compile_definitions(mpv-core)
target_compile_definitions(mpv-core PRIVATE BUILD_PEPPER HAVE_GL_WIN32=0 HAVE_GL_DXINTEROP=0 HAVE_GL_DXINTEROP_D3D9=0)

target_compile_definitions(mpv-core PRIVATE WIN32 _WINDOWS _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NDEBUG)
target_compile_definitions(mpv-core PRIVATE _WIN32_WINNT=0x0602 PLAYER_CORE_DLL COBJMACROS)

target_include_directories(mpv-core PRIVATE
    ${MPV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_PATH}
    ${THIRD_INC_PATH}
    build)

target_link_directories(mpv-core PRIVATE
    ${FFMPEG_LIB_PATH}
    ${THIRD_LIB_PATH})

target_link_libraries(mpv-core PRIVATE
    yolo4-tiny-detection
    ${ALL_FFMPEG_LIBS}
    ass$<$<CONFIG:Debug>:d>
    zlib
    dwmapi.lib
    winmm.lib
    ksuser.lib
    avrt.lib
    OpenGL32.lib
    Dxguid.lib
    )

add_library(${_TARGET_MPV} SHARED 
  src/nacl-pepper.cc)

disable_msvc_warnings(${_TARGET_MPV})
target_compile_definitions(${_TARGET_MPV} PRIVATE WIN32 _WINDOWS _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NDEBUG)
target_compile_definitions(${_TARGET_MPV} PRIVATE _WIN32_WINNT=0x0602 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS COBJMACROS)

target_include_directories(${_TARGET_MPV} PRIVATE
    ${NACL_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    ${THIRD_INC_PATH}
    src)

add_executable(mpv_player 
  $<TARGET_OBJECTS:mpv_common_objs>
  ${MPV_SOURCES}
  src/mpv/video/out/vo_gpu.c
  src/mpv/video/out/gpu/context.c
  src/mpv/video/out/opengl/context_win.c
  src/mpv/video/out/opengl/context_dxinterop.c
  src/mpv/video/out/opengl/hwdec_dxva2gldx.c
  src/mpv/osdep/main-fn-win.c
  src/detection/ai_render.c)

target_include_directories(mpv_player PRIVATE
    ${MPV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_PATH}
    ${THIRD_INC_PATH}
    build)

mpv_compile_definitions(mpv_player)
target_compile_definitions(mpv_player PRIVATE __thread=__declspec\(thread\) restrict=__restrict WINBOOL=BOOL HAVE_TA_LEAK_REPORT=0)
target_compile_definitions(mpv_player PRIVATE HAVE_GL_WIN32=1 HAVE_GL_DXINTEROP=1 HAVE_GL_DXINTEROP_D3D9=1)

disable_msvc_warnings(mpv_player)
target_compile_definitions(mpv_player PRIVATE WIN32 _WINDOWS _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NDEBUG)
target_compile_definitions(mpv_player PRIVATE _WIN32_WINNT=0x0602 COBJMACROS)

target_link_directories(mpv_player PRIVATE
    ${FFMPEG_LIB_PATH}
    ${THIRD_LIB_PATH})

if (MSVC)
  set_property(TARGET ${_TARGET_MPV} PROPERTY
               MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_link_directories(${_TARGET_MPV} PRIVATE
    ${NACL_LIB_PATH}
    ${FFMPEG_LIB_PATH}
    ${THIRD_LIB_PATH})

target_link_libraries(${_TARGET_MPV} PRIVATE
    mpv-core
    ppapi_cpp
    ppapi_gles2
    ${ALL_FFMPEG_LIBS}
    ass$<$<CONFIG:Debug>:d>
    dwmapi.lib
    winmm.lib
    ksuser.lib
    avrt.lib
    OpenGL32.lib
    Dxguid.lib
    )

target_link_libraries(mpv_player PRIVATE
    yolo4-tiny-detection
    ${ALL_FFMPEG_LIBS}
    ass$<$<CONFIG:Debug>:d>
    zlib
    dwmapi.lib
    winmm.lib
    ksuser.lib
    avrt.lib
    OpenGL32.lib
    Dxguid.lib
    dwmapi.lib
    )

add_custom_target(CopyRuntimeFiles ALL 
    VERBATIM 
    COMMAND_EXPAND_LISTS 
    COMMAND ${CMAKE_COMMAND} -E 
      copy_if_different
                $<TARGET_FILE:yolo4-tiny-detection>
                $<TARGET_FILE:mpv-core>
                $<TARGET_FILE:${_TARGET_MPV}>
                "${ELECTRON_APP_ROOT_PATH}"   
)
