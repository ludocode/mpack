# This builds MPack and its test suite with avr-gcc for AVR (e.g. Arduino.)
# It doesn't actually work yet; in fact it doesn't link because the resulting
# code is way too big. But it does actually compile all the source files at
# least. Eventually it would be nice to trim the unit test suite down into
# something that could run on an Arduino. It would also be nice to set up some
# wrapper scripts to make it run under simavr so we can run it on CI builds.

# Requires avr-gcc and avr-libc (even though it builds with MPACK_STDLIB
# disabled) since avr-libc has stdint.h and friends.

ifeq (Makefile, $(firstword $(MAKEFILE_LIST)))
$(error The current directory should be the root of the repository. Try "cd ../.." and then "make -f test/avr-gcc/Makefile")
endif

CC=avr-gcc
BUILD := test/.build/avr
PROG := mpack-avr

CPPFLAGS := -Isrc -Itest -DMPACK_HAS_CONFIG=1
CFLAGS := \
		-Os -DNDEBUG \
		-Wall -Wextra -Wpedantic -Werror \
		-MMD -MP

SRCS := \
	$(shell find src/ -type f -name '*.c') \
	$(wildcard test/test*.c)

OBJS := $(patsubst %, $(BUILD)/%.o, $(SRCS))

GLOBAL_DEPENDENCIES := test/avr/Makefile

.PHONY: all
all: $(PROG)

.PHONY: clean
clean:
	rm -rf $(BUILD)

-include $(patsubst %, $(BUILD)/%.d, $(SRCS))

.PHONY: $(PROG)
$(PROG): $(BUILD)/$(PROG)

$(OBJS): $(BUILD)/%.o: % $(GLOBAL_DEPENDENCIES)
	@mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/$(PROG): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)
