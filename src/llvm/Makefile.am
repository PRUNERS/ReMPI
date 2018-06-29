AM_CFLAGS = -fPIC -g -Wall -pthread -std=c++0x -rdynamic
AM_CXXFLAGS = $(AM_CFLAGS)


lib_LTLIBRARIES = libreompir.la libreomp.la

reompir_SOURCES =  \
		reomp_ir_pass.cpp \
		reomp_drace.cpp \
		mutil.cpp \
		reomp_util.cpp \
		reomp_config.cpp 

libreompir_la_SOURCES  = $(reompir_SOURCES)
libreompir_la_LIBADD =  $(libz_a_LIB)
#libreompir_la_CFLAGS = -lstdc++ `llvm-config --cxxflags`
libreompir_la_CFLAGS = -lstdc++ `llvm-config --cxxflags` -Wno-deprecated
libreompir_la_CXXFLAGS = -lz -lstdc++ `llvm-config --cxxflags`  -Wno-deprecated
libreompir_la_CPPFLAGS = `llvm-config --cppflags`  -Wno-deprecated
libreompir_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -L/opt/ibm/xlC/13.1.5/lib `llvm-config --ldflags`
libreompir_la_LDLIBS  = `llvm-config --ldlibs`


reomp_SOURCES =  \
		reomp_omp_wrapper.cpp \
		reomp_profile.cpp \
		reomp_gate.cpp \
		reomp_gate_tid.cpp \
		reomp_gate_clock.cpp \
		reomp_mem.cpp \
		reomp_mon.cpp \
		reomp.cpp \
		mutil.cpp \
		reomp_util.cpp \
		reomp_config.cpp 


libreomp_la_SOURCES  = $(reomp_SOURCES)
libreomp_la_LIBADD =  $(libz_a_LIB)
#libreomp_la_CFLAGS = -lstdc++ `llvm-config --cxxflags`
libreomp_la_CFLAGS = -lstdc++ `llvm-config --cxxflags` -Wno-deprecated -rdynamic
libreomp_la_CXXFLAGS = -lz -lstdc++ `llvm-config --cxxflags`  -Wno-deprecated -lbfd -rdynamic
libreomp_la_CPPFLAGS = `llvm-config --cppflags`  -Wno-deprecated 
libreomp_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@ -L/opt/ibm/xlC/13.1.5/lib `llvm-config --ldflags` -L/g/g90/sato5/opt/opt-catalyst/lib -rdynamic
libreomp_la_LDLIBS  = `llvm-config --ldlibs`
