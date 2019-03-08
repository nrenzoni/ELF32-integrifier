# gets src files without leading directory
# SRC_FILES := $(notdir $(wildcard src/*.c))

# all files in src directory without directory nor suffix
# FILES := $(basename $(SRC_FILES))

GCC_OPTIM_LEVEL := 3

buildHashShellCode : $(wildcard src/*)
	

src/%.c :
	./c_file_to_32_asm.sh $@ $(GCC_OPTIM_LEVEL)

