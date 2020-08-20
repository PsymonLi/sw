# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

.DEFAULT_GOAL := all
EVENT_GEN_DIR = ${BLD_GEN_DIR}/event/

all:
	mkdir -p ${EVENT_GEN_DIR}
	./event_compiler.py ${EVENT_GEN_DIR} common/* cloud/*
