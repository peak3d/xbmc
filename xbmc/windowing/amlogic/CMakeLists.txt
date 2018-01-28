set(SOURCES GLContextEGL.cpp
            WinSystemAmlogic.cpp
            VideoSyncAML.cpp
            VideoBufferION.cpp)

set(HEADERS GLContextEGL.h
            WinSystemAmlogic.h
            VideoSyncAML.h
            VideoBufferION.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemAmlogicGLESContext.cpp)
  list(APPEND HEADERS WinSystemAmlogicGLESContext.h)
endif()

core_add_library(windowing_Amlogic)
