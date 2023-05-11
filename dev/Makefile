EXE=blum8
CC_LINUX=cc
LIBS_LINUX=-lm
FLAGS_LINUX=-I ./include/ -I /usr/include/GLFW/ -I /usr/include/AL/
CC_WINDOWS=x86_64-w64-mingw32-gcc
LIBS_WINDOWS=-lm -lgdi32 -lmingwex -lwinmm
FLAGS_WINDOWS=-I ./include/ -I /usr/include/GLFW/ -I /usr/include/AL/ -I /usr/x86_64-w64-mingw32/include -m64 -mwindows
CFILES=$(wildcard src/*.c)
OBJS_LINUX=$(patsubst src/%.c, bin/linux/%.o, $(CFILES))
OBJS_WINDOWS=$(patsubst src/%.c, bin/windows/%.o, $(CFILES))
DEPS_LINUX=$(patsubst src/%.c, bin/linux/%.d, $(CFILES))
DEPS_WINDOWS=$(patsubst src/%.c, bin/windows/%.d, $(CFILES))
MODE=DEVELOPMENT

ifeq ($(MODE), DEVELOPMENT)
	FLAGS_LINUX += -Wall -Wextra -Werror -g
	FLAGS_WINDOWS += -Wall -Wextra -Werror -g
else ifeq ($(MODE), RELEASE)
	FLAGS_LINUX += -Ofast
	FLAGS_WINDOWS += -Ofast
else
$(error unknown mode $(RELEASE).)
endif

.PHONY: all clean

all: bin/linux/$(EXE) bin/windows/$(EXE).exe

bin/linux/$(EXE): $(OBJS_LINUX)
	@echo linking object files into elf...
	@$(CC_LINUX) $(FLAGS_LINUX) -o bin/linux/$(EXE) $^ $(LIBS_LINUX)
	@echo updating linux resources...
	@cp data/* ../linux/data/
	@cp shaders/* ../linux/shaders/
	@cp libs/linux/* ../linux/libs/
	@cp bin/linux/$(EXE) ../linux/
	@echo $(EXE) was succesfully builded for linux!

bin/windows/$(EXE).exe: $(OBJS_WINDOWS)
	@echo linking object files into exe...
	@$(CC_WINDOWS) $(FLAGS_WINDOWS) -o bin/windows/$(EXE) $^ $(LIBS_WINDOWS)
	@echo updating windows resources...
	@cp data/* ../windows/data/
	@cp shaders/* ../windows/shaders/
	@cp libs/windows/* ../windows/libs/
	@cp bin/windows/$(EXE).exe ../windows/
	@echo $(EXE) was succesfully builded for windows!

res:
	@cp data/* ../windows/data/
	@cp shaders/* ../windows/shaders/
	@cp libs/windows/* ../windows/libs/
	@cp data/* ../linux/data/
	@cp shaders/* ../linux/shaders/
	@cp libs/linux/* ../linux/libs/
	@echo resources updated


-include $(DEPS_WINDOWS)
-include $(DEPS_LINUX)

bin/linux/%.o: src/%.c
	@echo compiling $< with gcc...
	@$(CC_LINUX) $(FLAGS_LINUX) -MMD -c -DLINUX $< -o $@

bin/windows/%.o: src/%.c
	@echo compiling $< with mingw...
	@$(CC_WINDOWS) $(FLAGS_WINDOWS) -MMD -c -DWINDOWS $< -o $@

clean:
	@rm bin/linux/*
	@rm bin/windows/*
	@echo cleaned up binaries!
	@rm ../windows/$(EXE).exe  ../windows/shaders/* ../windows/libs/* ../windows/data/* ../linux/$(EXE) ../linux/shaders/* ../linux/libs/* ../linux/data/*
	@echo cleaned up build!

