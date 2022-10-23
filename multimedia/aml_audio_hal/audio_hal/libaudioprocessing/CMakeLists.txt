#############################################
# libaudioprocessing.a
#############################################
add_library(audioprocessing STATIC
	AudioResampler.cpp
	AudioResamplerSinc.cpp
	AudioResamplerCubic.cpp
	AudioResamplerDyn.cpp
)

target_include_directories(audioprocessing PRIVATE
	${PROJECT_SOURCE_DIR}/audio_hal
        ${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/utils)
