AM_CFLAGS = -fPIC -g -Wall -pthread -std=c++0x
AM_CXXFLAGS = $(AM_CFLAGS)

reomp_SOURCES =  reomp.cpp
reomp_SOURCES += reomp_err.cpp
reomp_SOURCES += reomp_util.cpp
reomp_SOURCES += reomp_config.cpp

lib_LTLIBRARIES = libreomp.la
libreomp_la_SOURCES  = $(reomp_SOURCES)
libreomp_la_LIBADD =  $(libz_a_LIB)
libreomp_la_CFLAGS = -lz -lstdc++ -lintlc
libreomp_la_CPPFLAGS = -DREOMP_LITE
libreomp_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -L/opt/ibm/xlC/13.1.5/lib 

