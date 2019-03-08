_FILES := $(shell ls src)
FILES := $(basename $(_FILES))

INCLUDE_PARENT_DIR := ./include
INCLUDE_DIRS := $(shell find $(INCLUDE_PARENT_DIR) -type d)
#INCLUDE_FILES := $(shell find $(INCLUDE_PARENT_DIR) -type f)
INCLUDE_PARAMS := $(INCLUDE_DIRS:%=-I%)

SRC_FILES := $(FILES:%=%.c)
SRC_DIR := ./src
SRC := $(patsubst %, $(SRC_DIR)/%, $(SRC_FILES))

OBJ_FILES := $(FILES:%=%.o)
OBJ_DIR := ./obj
OBJ := $(patsubst %, $(OBJ_DIR)/%, $(OBJ_FILES))

DIRS := $(SRC_DIR) $(OBJ_DIR) $(INCLUDE_DIR)

LIBS := capstone
# LIBS_PARAM := $(foreach l, $(LIBS), -l$l)
LIBS_PARAM := $(LIBS:%=-l%)

CFLAGS := -Wall -g

RELEASE_NAME := elf_integrifier

$(shell mkdir -p $(DIRS))

release : $(OBJ)
	$(CC) -o $(RELEASE_NAME) $(CFLAGS) $^ $(LIBS_PARAM)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(INCLUDE_FILES)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE_PARAMS) $(LIBS_PARAM)

.PHONY : clean

clean :
	rm $(OBJ)
