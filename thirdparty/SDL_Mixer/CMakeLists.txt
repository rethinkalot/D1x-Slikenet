cmake_minimum_required(VERSION 3.16)
project(SDL_mixer-1.2-cmake C)

add_library(SDL_mixer-1.2-cmake SHARED
	dynamic_flac.c
	dynamic_fluidsynth.c 
	dynamic_mod.c
	dynamic_mp3.c
	dynamic_ogg.c
	effect_position.c
	effects_internal.c
	effect_stereoreverse.c
	load_aiff.c
	load_flac.c
	load_ogg.c
	load_voc.c
	mixer.c
	mp3utils.c
	music.c
	music_cmd.c
	music_flac.c
	music_fluidsynth.c
	music_mad.c
	music_mod.c
	music_modplug.c
	music_mpg.c
	music_ogg.c
	wavestream.c
	SDL_mixer.h
)

add_library(SDL_mixer-1.2-cmake::SDL_mixer-1.2-cmake ALIAS SDL_mixer-1.2-cmake)

find_package(SDL REQUIRED)

find_package(PkgConfig REQUIRED) 
pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
pkg_check_modules(VORBISFILE REQUIRED IMPORTED_TARGET vorbisfile)
pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
pkg_check_modules(LIBMPG123 REQUIRED IMPORTED_TARGET libmpg123)
pkg_check_modules(FLUIDSYNTH REQUIRED IMPORTED_TARGET fluidsynth)

target_compile_definitions(SDL_mixer-1.2-cmake PRIVATE
	BUILD_SDL_MIXER
	WAV_MUSIC OGG_MUSIC MP3_MUSIC FLAC_MUSIC
	MID_MUSIC USE_FLUIDSYNTH_MIDI)

target_include_directories(SDL_mixer-1.2-cmake PRIVATE ${SDL_INCLUDE_DIR})
target_link_libraries(SDL_mixer-1.2-cmake PUBLIC
	${SDL_LIBRARY}
	PkgConfig::OGG
	PkgConfig::VORBIS
	PkgConfig::VORBISFILE
	PkgConfig::FLAC
	PkgConfig::LIBMPG123
	PkgConfig::FLUIDSYNTH)

target_include_directories(SDL_mixer-1.2-cmake PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

set_target_properties(SDL_mixer-1.2-cmake PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
