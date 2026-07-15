# =============================================================================
# libmem — pool · arena · slab allocators
# =============================================================================
#
#  Targets:
#
#    help      Show this message                             (default)
#    all       Build libmem.a  (release)
#    debug     Build libmem.a with -DLIBMEM_DEBUG + -g
#    examples  Build all programs in examples/
#    clean     Remove object files and example binaries
#    fclean    clean + remove libmem.a
#    re        fclean + all
#
# =============================================================================

CC      := cc
AR      := ar
ARFLAGS := rcs

NAME    := libmem.a
CFLAGS  := -Wall -Wextra -Werror -Iinclude

SRC :=  src/pool.c   \
        src/arena.c  \
        src/slab.c

OBJ       := $(SRC:src/%.c=build/%.o)
OBJ_DEBUG := $(SRC:src/%.c=build/debug_%.o)

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
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make all"      "Build $(NAME)  (release)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make debug"    "Build $(NAME) with -DLIBMEM_DEBUG -g"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make examples" "Build all programs in examples/"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make clean"    "Remove object files and example binaries"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make fclean"   "clean + remove $(NAME)"
	@printf "  $(CYAN)%-16s$(RESET) %s\n" "make re"       "fclean + all"
	@printf "\n"

# =============================================================================
# Release build
# =============================================================================

## all: Build libmem.a (release)
.PHONY: all
all: $(NAME)

build/%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p build

$(NAME): $(OBJ)
	@$(AR) $(ARFLAGS) $@ $^
	@printf "$(GREEN)$(BOLD)✓ $(NAME) (release)$(RESET)\n"

# =============================================================================
# Debug build  (-DLIBMEM_DEBUG enables stats and double-free detection)
# =============================================================================

## debug: Build libmem.a with debug flags
.PHONY: debug
debug: CFLAGS += -g -DLIBMEM_DEBUG
debug: $(OBJ_DEBUG)
	@$(AR) $(ARFLAGS) $(NAME) $^
	@printf "$(YELLOW)$(BOLD)✓ $(NAME) (debug — LIBMEM_DEBUG)$(RESET)\n"

build/debug_%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $< $(YELLOW)[debug]$(RESET)\n"
	@$(CC) $(CFLAGS) -g -DLIBMEM_DEBUG -c $< -o $@

# =============================================================================
# Examples
# =============================================================================

EXAMPLES_SRC := $(wildcard examples/*.c)
EXAMPLES_BIN := $(EXAMPLES_SRC:examples/%.c=examples/bin/%)

## examples: Build all programs in examples/
.PHONY: examples
examples: $(NAME) $(EXAMPLES_BIN)

examples/bin/%: examples/%.c $(NAME) | examples/bin
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -o $@ $< $(NAME)
	@printf "$(GREEN)✓ $@$(RESET)\n"

examples/bin:
	@mkdir -p examples/bin

# =============================================================================
# Cleanup
# =============================================================================

## clean: Remove object files and example binaries
.PHONY: clean
clean:
	@rm -rf examples/bin $(OBJ) $(OBJ_DEBUG)
	@printf "$(DIM)libmem: cleaned$(RESET)\n"

## fclean: clean + remove libmem.a
.PHONY: fclean
fclean: clean
	@rm -f $(NAME)

## re: fclean + all
.PHONY: re
re: fclean all