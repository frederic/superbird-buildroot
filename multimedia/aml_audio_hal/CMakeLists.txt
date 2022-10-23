cmake_minimum_required(VERSION 3.8)

project(aml_audio_hal
	VERSION 1.0)

include(GNUInstallDirs)

add_subdirectory("libms12v2")
add_subdirectory("audio_hal/libaudioprocessing")

add_library(spdifenc STATIC
	utils/SPDIFEncoderAD.cpp
	utils/spdifenc_wrap.cpp
)

target_include_directories(spdifenc PRIVATE
	./audio_hal
	./include
	./include/utils)

target_compile_options(spdifenc PRIVATE -Wno-deprecated-declarations)

option(USE_DTV "Compile with DTV option" ON)

if(USE_DTV)
    list(APPEND DTV_FILES audio_hal/audio_dtv_ad.c)
    list(APPEND DTV_FILES audio_hal/audio_hw_dtv.c)
    list(APPEND DTV_INC $ENV{AML_AUDIO_HAL_STAGING_DIR}/usr/include/aml_dvb/am_adp)
    list(APPEND DTV_LIBS -L$ENV{AML_AUDIO_HAL_STAGING_DIR}/usr/lib/aml_libs_nf -lamadec -lam_adp )
endif()

#############################################
# libaudio_hal
#############################################
add_library(audio_hal SHARED
	audio_hal/alsa_config_parameters.c
	audio_hal/alsa_manager.c
	audio_hal/aml_ac3_parser.c
	audio_hal/aml_audio_delay.c
	audio_hal/aml_audio_ease.c
	audio_hal/amlAudioMixer.c
	audio_hal/aml_audio_resample_manager.c
	audio_hal/aml_audio_stream.c
	audio_hal/aml_audio_timer.c
	audio_hal/aml_avsync_tuning.c
	audio_hal/aml_dca_dec_api.c
	audio_hal/aml_dcv_dec_api.c
	audio_hal/aml_hw_mixer.c
	audio_hal/aml_resample_wrap.cpp
	audio_hal/audio_android_resample_api.c
	audio_hal/audio_data_process.c
	audio_hal/audio_format_parse.c
	audio_hal/audio_hw.c
	audio_hal/audio_hw_ms12.c
	audio_hal/audio_hw_profile.c
	audio_hal/audio_hwsync.c
	audio_hal/audio_hw_utils.c
	audio_hal/audio_port.c
	audio_hal/audio_post_process.c
	audio_hal/audio_route.c
	audio_hal/audio_simple_resample_api.c
	audio_hal/audio_virtual_buf.c
	audio_hal/dolby_lib_api.c
	audio_hal/hw_avsync.c
	audio_hal/hw_avsync_callbacks.c
	audio_hal/spdif_encoder_api.c
	audio_hal/sub_mixing_factory.c
	amlogic_AQ_tools/audio_eq_drc_compensation.c
	amlogic_AQ_tools/audio_eq_drc_parser.c
	amlogic_AQ_tools/ini/dictionary.c
	amlogic_AQ_tools/ini/iniparser.c
	utils/ac3_parser_utils.c
	utils/alsa_device_parser.c
	utils/aml_alsa_mixer.c
	utils/aml_android_utils.c
	utils/aml_audio_mixer.c
	utils/aml_audio_resampler.c
	utils/aml_buffer_provider.c
	utils/aml_data_utils.c
	utils/aml_dump_debug.c
	utils/aml_ringbuffer.c
	utils/aml_volume_utils.c
	utils/aml_configs/aml_conf_loader.c
	utils/aml_configs/aml_conf_parser.c
    ${DTV_FILES}
)

add_definitions(-DREPLACE_OUTPUT_BUFFER_WITH_CALLBACK)

set(PREBUILD_STATIC_LIBS
	${CMAKE_SOURCE_DIR}/amlogic_AQ_tools/lib_aml_ng.a
	${CMAKE_SOURCE_DIR}/amlogic_AQ_tools/Amlogic_DRC_Param_Generator.a
	${CMAKE_SOURCE_DIR}/amlogic_AQ_tools/Amlogic_EQ_Param_Generator.a)

set_target_properties(audio_hal PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1)

target_include_directories(audio_hal PRIVATE
    ${DTV_INC}
	./include
	./include/utils
	./include/amlogic_AQ_tools
	./amlogic_AQ_tools
	./amlogic_AQ_tools/ini
	./libms12v2/include)

target_compile_options(audio_hal PRIVATE -D_GNU_SOURCE -DTV_AUDIO_OUTPUT -Wno-deprecated-declarations)

target_link_libraries(audio_hal
#	audioprocessing spdifenc ms12v2 ${PREBUILD_STATIC_LIBS}
	audioprocessing spdifenc ms12v2
    ${DTV_LIBS} -lcutils -lamaudioutils -ltinyalsa -lamavutils -llog -lexpat -lpthread -ldl -lstdc++ -Wl,-z,defs)

install(TARGETS audio_hal
	LIBRARY DESTINATION "lib")

install(FILES audio_hal/mixer_paths.xml
	DESTINATION /etc)

