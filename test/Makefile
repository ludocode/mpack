# This Makefile currently just builds fuzz.c into a fuzzer for use with
# american fuzzy lop. Don't use this directly; use tools/afl.sh to fuzz MPack.

ifeq (Makefile, $(firstword $(MAKEFILE_LIST)))
$(error The current directory should be the root of the repository. Try "cd .." and then "make -f test/Makefile")
endif

CC=afl-gcc

CPPFLAGS := $(CPPFLAGS) \
	-include test/fuzz-config.h \
	-Isrc \
	-O0 -DDEBUG -g \
	-MMD -MP \

BUILD := build/fuzz
PROG := mpack-fuzz

SRCS := \
	$(shell find src/ -type f -name '*.c') \
	test/fuzz.c

OBJS := $(patsubst %, $(BUILD)/%.o, $(SRCS))

GLOBAL_DEPENDENCIES := test/Makefile

.PHONY: all
all: $(PROG)

-include $(patsubst %, $(BUILD)/%.d, $(SRCS))

.PHONY: $(PROG)
$(PROG): $(BUILD)/$(PROG)

$(OBJS): $(BUILD)/%.o: % $(GLOBAL_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/$(PROG): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)
