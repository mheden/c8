CC = gcc
CC_FLAGS = -Wall -Werror -Wextra -std=c99 -pedantic -O2
CC_INCLUDE = -Iinclude

AR = ar
AR_FLAGS = rcs

#------------------------------------------------------------------------------#

LIB_BIN = libc8.a
LIB_SRC = src/c8.c
LIB_OBJ = $(LIB_SRC:.c=.o)
LIB_DEP = $(LIB_OBJ:.o=.d)

APP_BIN = c8emu
APP_SRC = c8emu.c
APP_OBJ = $(APP_SRC:.c=.o)
APP_DEP = $(APP_OBJ:.o=.d)

TEST_BIN = test_c8
TEST_SRC = $(wildcard test/*.c) $(wildcard test/unity/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_DEP = $(TEST_OBJ:.o=.d)

#------------------------------------------------------------------------------#

all: $(LIB_BIN) $(TEST_BIN) $(APP_BIN)

lib: $(LIB_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(LIB_BIN): $(LIB_OBJ)
	$(AR) $(AR_FLAGS) $@ $^

$(APP_BIN): $(APP_OBJ) $(LIB_BIN)
	$(CC) -o $@ $^ $(LIB_BIN)

$(TEST_BIN): $(TEST_OBJ)
	$(CC) -o $@ $^

%.d: %.c
	$(CC) $(CC_FLAGS) $(CC_INCLUDE) $< -MM -MT $(@:.d=.o) > $@

%.o: %.c
	$(CC) -c $(CC_FLAGS) $(CC_INCLUDE) $< -o $@

-include $(LIB_DEP)
-include $(APP_DEP)
-include $(TEST_DEP)

.PHONY: clean
clean:
	rm -f $(LIB_BIN) $(LIB_OBJ) $(LIB_DEP)
	rm -f $(APP_BIN) $(APP_OBJ) $(APP_DEP)
	rm -f $(TEST_BIN) $(TEST_OBJ) $(TEST_DEP)
