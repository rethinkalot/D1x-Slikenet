add_library(xmodel STATIC
aseread.cpp
submodel.cpp
tga.cpp
xmodel.cpp
	)
if(DXX_USE_BUNDLED_DEPS)
    target_link_libraries(xmodel PUBLIC ${DXX_GLEW_TARGET} ${DXX_SDL_TARGET})
else()
    find_package(GLEW REQUIRED)
    target_include_directories(xmodel PUBLIC ${GLEW_INCLUDE_DIR})
endif()
