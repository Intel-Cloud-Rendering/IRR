HOME_DIR:=${PWD}
SRC:=${HOME_DIR}/src
BIN:=${HOME_DIR}/bin
OBJ:=${HOME_DIR}/obj
INC:=${HOME_DIR}/inc
TRANSLATOR_INC:=../android/android-emugl/host/libs/Translator/include
INTER_SRC:=${SRC}/intermediates
EMUGEN_SRC:=${HOME_DIR}/../android/android-emugl/host/tools/emugen
EMUGEN_LOCAL_PATH:=${SRC}/tools/emugen
SPEC_SRC:=${HOME_DIR}/../android/android-emugl/host/libs
COMM_PATH:=${HOME_DIR}/../android/android-emu
BASE_COMM_SRC:=${addprefix ${COMM_PATH}/android/base/, StringView.cpp system/System.cpp Log.cpp files/PathUtils.cpp threads/Thread_pthread.cpp threads/ThreadStore.cpp threads/FunctorThread.cpp StringFormat.cpp synchronization/MessageChannel.cpp memory/LazyInstance.cpp}
EMUGL_COMM_PATH:=${HOME_DIR}/../android/android-emugl/shared
EMUGL_COMM_SRC:=${addprefix ${EMUGL_COMM_PATH}/emugl/common/, stringparsing.cpp shared_library.cpp misc.cpp}
OGL_CODEC_COMM_PATH:=${HOME_DIR}/../android/android-emugl/shared/OpenglCodecCommon
OGL_CODEC_COMM_SRC:=${addprefix ${OGL_CODEC_COMM_PATH}/, glUtils.cpp ChecksumCalculator.cpp ChecksumCalculatorThreadInfo.cpp}
OGLRD_PATH:=${HOME_DIR}/../android/android-emugl/host/libs/libOpenglRender
OGLRD_SRC:=${addprefix ${OGLRD_PATH}/, RenderWindow.cpp RendererImpl.cpp YUVConverter.cpp NativeSubWindow_x11.cpp ColorBuffer.cpp TextureResize.cpp FbConfig.cpp RenderControl.cpp TextureDraw.cpp WindowSurface.cpp RenderContext.cpp RenderThreadInfo.cpp RenderLibImpl.cpp FrameBuffer.cpp GLESVersionDetector.cpp}
OGLRD_INC:=${HOME_DIR}/../android/android-emugl/host/include
TEST_SRC:=${SRC}/test
STREAM_SRC:=${HOME_DIR}/../android/android-emu/android/streaming

EGL_INC:=

FFMPEG_PATH:=${HOME_DIR}/../../../prebuilts/android-emulator-build/common/ffmpeg_qsv_331/linux-x86_64
FFMPEG_INC:=-I${FFMPEG_PATH}/include/
FFMPEG_LIB_PATH:=-L${FFMPEG_PATH}/lib/
FFMPEG_LIBS:=-lavdevice -lavfilter -lavformat -lavcodec -lavutil -lswresample -lswscale -lva -lva-drm -lva-x11 -lz

LIBTRANS_PATH:=${HOME_DIR}/../../../prebuilts/android-emulator-build/common/libtrans/linux-x86_64
LIBTRANS_INC:=-I${LIBTRANS_PATH}/include
LIBTRANS_LIB_PATH:=-L${LIBTRANS_PATH}/lib
LIBTRANS_LIBS:=-ltrans

MFX_PATH:=${HOME_DIR}/../../../prebuilts/android-emulator-build/common/mfx/linux-x86_64
MFX_INC:=-I${MFX_PATH}/include
MFX_LIB_PATH:=-L${MFX_PATH}/lib
MFX_LIBS:=-lmfx

OGL_DISPATCH_PATH:=${HOME_DIR}/../android/android-emugl/host/libs/libOpenGLESDispatch
OGL_DISPATCH_SRC:=${addprefix ${OGL_DISPATCH_PATH}/, GLESv2Dispatch.cpp GLESv1Dispatch.cpp}

#CPP_FILES:=${wildcard ${SRC}/*.cpp ${INTER_SRC}/*.cpp ${COMM_SRC}/*.cpp ${OGLRD_SRC}/*.cpp ${OGL_CODEC_COMM_SRC}/*.cpp}
CPP_FILES:=${wildcard ${SRC}/override/*.cpp ${SRC}/*.cpp ${STREAM_SRC}/*.cpp ${OGLRD_SRC} ${OGL_DISPATCH_SRC} ${INTER_SRC}/*.cpp ${BASE_COMM_SRC} ${EMUGL_COMM_SRC} ${OGL_CODEC_COMM_SRC}}
#CPP_FILES:=$(shell find ${SRC} -type d \( -path ${SPEC_SRC} -prune -o -path ${TEST_SRC} \) -prune -o -name "*.cpp" -print)
OBJ_FILES:=${addprefix ${OBJ}/, ${notdir ${CPP_FILES:.cpp=.cpp.o}}}
CC:=g++
DEFS:=-DIRR2
CPPFLAGS:=-std=c++11 -g ${DEFS}
INC_PATH:=-I${STREAM_SRC} -I${EGL_INC} -I${TRANSLATOR_INC} -I${INC} -I${OGLRD_PATH} -I${COMM_PATH} -I${EMUGL_COMM_PATH} -I${INTER_SRC} -I${OGLRD_INC} -I${SRC} -I${OGL_CODEC_COMM_PATH} -I${OGL_DISPATCH_PATH} -I${EGL_INC} -I${SPEC_SRC}/GLESv1_dec -I${SPEC_SRC}/GLESv2_dec -I${SPEC_SRC}/renderControl_dec
LIB_PATH:=${MFX_LIB_PATH} ${FFMPEG_LIB_PATH} ${LIBTRANS_LIB_PATH} -L${HOME_DIR}/boost_1_65_1/stage/lib -L${HOME_DIR}/lib64
LIBS:=${LIBTRANS_LIBS} ${FFMPEG_LIBS} ${MFX_LIBS} -lboost_system -lboost_thread -lpthread -ldl -lX11 -lboost_timer -lboost_chrono -lrt

.PHONY: all
all: dec bin_folder ${BIN}/RenderServer clean_intermediate

.PHONY: dec
dec: gles1_dec gles2_dec renderControl_dec

.PHONY: gles1_dec
gles1_dec: inter_folder emugen
	${BIN}/emugen -i ${SPEC_SRC}/GLESv1_dec -D ${INTER_SRC} gles1
	cp ${SPEC_SRC}/GLESv1_dec/GLESv1Decoder.h ${INTER_SRC}
	cp ${SPEC_SRC}/GLESv1_dec/GLESv1Decoder.cpp ${INTER_SRC}

.PHONY: gles2_dec
gles2_dec: inter_folder emugen
	${BIN}/emugen -i ${SPEC_SRC}/GLESv2_dec -D ${INTER_SRC} gles2
	cp ${SPEC_SRC}/GLESv2_dec/GLESv2Decoder.h ${INTER_SRC}
	cp ${SPEC_SRC}/GLESv2_dec/GLESv2Decoder.cpp ${INTER_SRC}

.PHONY: renderControl_dec
renderControl_dec: inter_folder emugen
	${BIN}/emugen -i ${SPEC_SRC}/renderControl_dec -D ${INTER_SRC} renderControl

.PHONY: emugen
emugen: obj_folder bin_folder
	cd ${EMUGEN_LOCAL_PATH} && make

.PHONY: test
test: ${BIN}/BufQueTest ${BIN}/RdChanTest

${BIN}/BufQueTest: obj_folder bin_folder ${OBJ}/BufQueTest.cpp.o
	@${CC} ${OBJ}/BufQueTest.cpp.o -o $@ ${LIB_PATH} ${LIBS}

${BIN}/RdChanTest: obj_folder bin_folder ${OBJ}/RdChanTest.cpp.o ${OBJ}/RenderChannel.cpp.o ${OBJ}/ExtendableSmallBuffer.cpp.o
	@${CC} ${OBJ}/RdChanTest.cpp.o ${OBJ}/RenderChannel.cpp.o ${OBJ}/ExtendableSmallBuffer.cpp.o -o $@ ${LIB_PATH} ${LIBS}

${BIN}/RenderServer: obj_folder bin_folder ${OBJ_FILES}
	@${CC} ${OBJ_FILES} -o $@ ${LIB_PATH} ${LIBS}

${OBJ}/%.cpp.o: ${SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${STREAM_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${FFMPEG_INC} ${LIBTRANS_INC} ${MFX_INC} ${INC_PATH}

${OBJ}/%.cpp.o:${SRC}/override/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${INTER_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${OGL_CODEC_COMM_PATH}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${OGLRD_PATH}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${OGL_DISPATCH_PATH}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${EMUGL_COMM_PATH}/emugl/common/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/threads/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/memory/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/synchronization/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/system/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_PATH}/android/base/files/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${TEST_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

.PHONY: clean_intermediate
clean_intermediate:
	@rm -r ${OBJ} | true
	@rm -r ${INTER_SRC} | true

.PHONY: clean
clean: clean_intermediate
	@rm -r ${BIN} | true

.PHONY: obj_folder
obj_folder: | ${OBJ}

.PHONY: bin_folder
bin_folder: | ${BIN}

.PHONY: inter_folder
inter_folder: | ${INTER_SRC}

${OBJ}:
	@mkdir -p ${OBJ}

${BIN}:
	@mkdir -p ${BIN}

${INTER_SRC}:
	@mkdir -p ${INTER_SRC}
