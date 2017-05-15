<<<<<<< HEAD
AM_CFLAGS = -fPIC -g -Wall -pthread -std=c++0x 
AM_CXXFLAGS = $(AM_CFLAGS)




lib_LTLIBRARIES = libreomp.la libreomprr.la

reomp_SOURCES =  \
		reomp_IR_pass.cpp \
		mutil.cpp \
		reomp_util.cpp \
		reomp_config.cpp 
libreomp_la_SOURCES  = $(reomp_SOURCES)
libreomp_la_LIBADD =  $(libz_a_LIB)
libreomp_la_CFLAGS = -lstdc++ `llvm-config --cxxflags`
libreomp_la_CXXFLAGS = -lz -lstdc++ `llvm-config --cxxflags`
libreomp_la_CPPFLAGS = `llvm-config --cppflags`
libreomp_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -L/opt/ibm/xlC/13.1.5/lib `llvm-config --ldflags`
libreomp_la_LDLIBS  = `llvm-config --ldlibs`


reomprr_SOURCES =  \
		reomp_mem.cpp \
		reomp_rr.cpp \
		mutil.cpp \
		reomp_util.cpp \
		reomp_config.cpp 
libreomprr_la_SOURCES  = $(reomprr_SOURCES)
libreomprr_la_LIBADD =  $(libz_a_LIB)
libreomprr_la_CFLAGS = -lstdc++ `llvm-config --cxxflags`
libreomprr_la_CXXFLAGS = -lz -lstdc++ `llvm-config --cxxflags`
libreomprr_la_CPPFLAGS = `llvm-config --cppflags`
libreomprr_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -L/opt/ibm/xlC/13.1.5/lib `llvm-config --ldflags`
libreomprr_la_LDLIBS  = `llvm-config --ldlibs`

=======
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
>>>>>>> 1c5f25d99afd61625dd8c3a6ea85b688be4cbbb7

