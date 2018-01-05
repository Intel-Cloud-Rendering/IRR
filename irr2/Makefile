HOME_DIR:=${PWD}
SRC:=${HOME_DIR}/src
BIN:=${HOME_DIR}/bin
OBJ:=${HOME_DIR}/obj
INC:=${HOME_DIR}/inc
INTER_SRC:=${SRC}/intermediates
EMUGEN_SRC:=${SRC}/tools/emugen
SPEC_SRC:=${SRC}/specs
COMM_SRC:=${SRC}/common
OGL_CODEC_COMM_SRC:=${SRC}/common/OpenglCodecCommon
OGLRD_SRC:=${SRC}/openglRender
TEST_SRC:=${SRC}/test
#CPP_FILES:=${wildcard ${SRC}/*.cpp ${INTER_SRC}/*.cpp ${COMM_SRC}/*.cpp ${OGLRD_SRC}/*.cpp ${OGL_CODEC_COMM_SRC}/*.cpp}
CPP_FILES:=${wildcard ${SRC}/*.cpp ${SRC}/openglRender/*.cpp ${SRC}/openglesDispatch/*.cpp ${INTER_SRC}/*.cpp ${COMM_SRC}/*/*.cpp ${COMM_SRC}/*/*/*.cpp ${COMM_SRC}/*/*/*/*.cpp}
OBJ_FILES:=${addprefix ${OBJ}/, ${notdir ${CPP_FILES:.cpp=.cpp.o}}}
CC:=g++
CPPFLAGS:=-std=c++11 -g
INC_PATH:=-I${INC} -I${INC}/OpenglRender -I${COMM_SRC} -I${INTER_SRC} -I${OGLRD_SRC} -I${SRC} -I${OGL_CODEC_COMM_SRC} -I${SPEC_SRC}/GLESv1_dec -I${SPEC_SRC}/GLESv2_dec -I${SPEC_SRC}/renderControl_dec
LIB_PATH:=-L${HOME_DIR}/boost_1_65_1/stage/lib -L${HOME_DIR}/lib64
LIBS:=-lboost_system -lboost_thread -lpthread -ldl -lX11 -lboost_timer -lboost_chrono -lrt

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
	cd ${EMUGEN_SRC} && make

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

${OBJ}/%.cpp.o: ${INTER_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${OGLRD_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${OGL_CODEC_COMM_SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/openglRender/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${SRC}/openglesDispatch/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/emugl/common/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/threads/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/memory/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/synchronization/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/system/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}

${OBJ}/%.cpp.o: ${COMM_SRC}/android/base/files/%.cpp
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
