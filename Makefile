# =============================================================================
# libmem — pool · arena · slab allocators
# =============================================================================
#
#  Targets:
#
#    help      Show this message                      (default)
#    all       Build libmem.a
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

OBJ := $(SRC:src/%.c=build/%.o)

RESET  := \033[0m
BOLD   := \033[1m
DIM    := \033[2m
GREEN  := \033[32m
CYAN   := \033[36m

# =============================================================================

.DEFAULT_GOAL := help

.PHONY: help
help:
	@printf "\n$(BOLD)libmem$(RESET) — available targets\n\n"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make all"      "Build $(NAME)"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make examples" "Build all programs in examples/"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make clean"    "Remove object files and example binaries"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make fclean"   "clean + remove $(NAME)"
	@printf "  $(CYAN)%-14s$(RESET) %s\n" "make re"       "fclean + all"
	@printf "\n"

## all: Build libmem.a
.PHONY: all
all: $(NAME)

build/%.o: src/%.c | build
	@printf "  $(DIM)CC$(RESET)  $<\n"
	@$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p build

$(NAME): $(OBJ)
	@$(AR) $(ARFLAGS) $@ $^
	@printf "$(GREEN)$(BOLD)✓ $(NAME)$(RESET)\n"

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
	@rm -rf examples/bin $(OBJ)
	@printf "$(DIM)libmem: cleaned$(RESET)\n"

## fclean: clean + remove libmem.a
.PHONY: fclean
fclean: clean
	@rm -f $(NAME)

## re: fclean + all
.PHONY: re
re: fclean all