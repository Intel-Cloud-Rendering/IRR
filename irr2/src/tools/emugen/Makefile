HOME_DIR:=${PWD}/../../..
SRC:=${HOME_DIR}/../android/android-emugl/host/tools/emugen
BIN:=${HOME_DIR}/bin
OBJ:=${HOME_DIR}/obj/emugen
#CPP_FILES:=${wildcard ${SRC}/*.cpp}
CPP_FILES:=${SRC}/ApiGen.cpp ${SRC}/EntryPoint.cpp ${SRC}/main.cpp ${SRC}/Parser.cpp ${SRC}/strUtils.cpp ${SRC}/TypeFactory.cpp 
OBJ_FILES:=${addprefix ${OBJ}/, ${notdir ${CPP_FILES:.cpp=.cpp.o}}}
CC:=g++
DEFS:=-DIRR2
CPPFLAGS:=-std=c++11 ${DEFS}
INC_PATH:=-I${HOME_DIR}/src/tools/emugen/
LIB_PATH:=
LIBS:=

.PHONY: all
all: bin_folder ${BIN}/emugen clean_intermediate

${BIN}/emugen: obj_folder bin_folder ${OBJ_FILES}
	@${CC} ${OBJ_FILES} -o $@ ${LIB_PATH} ${LIBS}

${OBJ}/%.cpp.o: ${SRC}/%.cpp
	@${CC} ${CPPFLAGS} -c $^ -o $@ ${INC_PATH}


.PHONY: clean_intermediate
clean_intermediate:
	@rm -r ${OBJ} | true

.PHONY: clean
clean: clean_intermediate
	@rm -r ${BIN} | true

.PHONY: obj_folder
obj_folder: | ${OBJ}

.PHONY: bin_folder
bin_folder: | ${BIN}

${OBJ}:
	@mkdir -p ${OBJ}

${BIN}:
	@mkdir -p ${BIN}
