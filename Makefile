# =============================================================================
# libmem — pool · arena · slab allocators
# =============================================================================
#
#  Targets:
#
#    help        Show this message                             (default)
#    all         Build libmem.a  (release)
#    debug       Build libmem_debug.a with -DLIBMEM_DEBUG -g
#    examples    Build all programs in examples/
#    test        Build and run the test suite (release)
#    test-debug  Build and run the test suite (debug)
#    clean       Remove object files, example and test binaries
#    fclean      clean + remove libmem.a and libmem_debug.a
#    re          fclean + all
#
# =============================================================================

CC      := cc
AR      := ar
ARFLAGS := rcs

NAME       := libmem.a
DEBUG_NAME := libmem_debug.a
CFLAGS     := -Wall -Wextra -Werror -Iinclude

SRC :=  src/pool.c   \
        src/arena.c  \
        src/slab.c

# Release objects
OBJ       := $(SRC:src/%.c=build/%.o)
# Debug objects — separate prefix so they never mix with release objects
OBJ_DEBUG := $(SRC:src/%.c=build/debug_%.o)

# Tests
TEST_SRC := tests/test_libmem.c
TEST_BIN := tests/bin/test_libmem
TEST_DBG := tests/bin/test_libmem_debug

RESET  := \033[0m
BOLD   := \033[1m
DIM    := \033[2m
GREEN  := \033[32m
YELLOW := \033[33m
CYAN   := \033[36m

# =============================================================================

.DEFAULT_GOAL := help

.PHONY: help
help:
	@printf "\n$(BOLD)libmem$(RESET) — available targets\n\n"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make all"        "Build $(NAME)  (release)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make debug"      "Build $(DEBUG_NAME) with -DLIBMEM_DEBUG -g"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make examples"   "Build all programs in examples/"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make test"       "Build and run test suite (release)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make test-debug" "Build and run test suite (debug)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make clean"      "Remove object files and binaries"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make fclean"     "clean + remove $(NAME) and $(DEBUG_NAME)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make re"         "fclean + all"
	@printf "\n"

# =============================================================================
# Release build → libmem.a
# =============================================================================

.PHONY: all
all: $(NAME)

build/%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p build

$(NAME): $(OBJ)
	@rm -f $@
	@$(AR) $(ARFLAGS) $@ $^
	@printf "$(GREEN)$(BOLD)✓ $(NAME) (release)$(RESET)\n"

# =============================================================================
# Debug build → libmem_debug.a   (-DLIBMEM_DEBUG + -g)
#
# Uses a separate archive file so that release and debug object files
# never coexist in the same archive.  Mixing them would cause duplicate
# symbols and potential ABI mismatches (struct sizes differ under
# LIBMEM_DEBUG).
# =============================================================================

.PHONY: debug
debug: $(DEBUG_NAME)

build/debug_%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $< $(YELLOW)[debug]$(RESET)\n"
	@$(CC) $(CFLAGS) -g -DLIBMEM_DEBUG -c $< -o $@

$(DEBUG_NAME): $(OBJ_DEBUG)
	@rm -f $@
	@$(AR) $(ARFLAGS) $@ $^
	@printf "$(YELLOW)$(BOLD)✓ $(DEBUG_NAME) (debug — LIBMEM_DEBUG)$(RESET)\n"

# =============================================================================
# Examples
# =============================================================================

EXAMPLES_SRC := $(wildcard examples/*.c)
EXAMPLES_BIN := $(EXAMPLES_SRC:examples/%.c=examples/bin/%)

.PHONY: examples
examples: $(NAME) $(EXAMPLES_BIN)

examples/bin/%: examples/%.c $(NAME) | examples/bin
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -o $@ $< $(NAME)
	@printf "$(GREEN)✓ $@$(RESET)\n"

examples/bin:
	@mkdir -p examples/bin

# =============================================================================
# Test suite
# =============================================================================

## test: Build and run the test suite against the release library
.PHONY: test
test: $(NAME) $(TEST_BIN)
	@printf "\n$(BOLD)Running tests (release)…$(RESET)\n"
	@$(TEST_BIN)

## test-debug: Build and run the test suite against the debug library
.PHONY: test-debug
test-debug: $(DEBUG_NAME) | tests/bin
	@printf "  $(DIM)CC$(RESET)  $(TEST_SRC) $(YELLOW)[debug]$(RESET)\n"
	@$(CC) $(CFLAGS) -g -DLIBMEM_DEBUG -o $(TEST_DBG) $(TEST_SRC) $(DEBUG_NAME)
	@printf "$(GREEN)✓ $(TEST_DBG)$(RESET)\n"
	@printf "\n$(BOLD)Running tests (debug)…$(RESET)\n"
	@$(TEST_DBG)

$(TEST_BIN): $(TEST_SRC) $(NAME) | tests/bin
	@printf "  $(DIM)CC$(RESET)  $(TEST_SRC)\n"
	@$(CC) $(CFLAGS) -o $@ $< $(NAME)
	@printf "$(GREEN)✓ $(TEST_BIN)$(RESET)\n"

tests/bin:
	@mkdir -p tests/bin

# =============================================================================
# Cleanup
# =============================================================================

.PHONY: clean
clean:
	@rm -rf examples/bin tests/bin $(OBJ) $(OBJ_DEBUG)
	@printf "$(DIM)libmem: cleaned$(RESET)\n"

.PHONY: fclean
fclean: clean
	@rm -f $(NAME) $(DEBUG_NAME)

.PHONY: re
re: fclean all