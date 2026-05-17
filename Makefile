NAME = libmem.a
CFLAGS = -Wall -Wextra -Werror -Iinclude

SRC = lib/memory.c

OBJ = build/memory.o

default: $(NAME)

main: $(NAME)
	@cc $(CFLAGS) -o main main.c $(NAME) -lssl -lcrypto
	@echo "Main executable created successfully."

$(NAME):
	@cc -c $(CFLAGS) $(SRC)
	@mv *.o build/
	@ar rcs $(NAME) $(OBJ)
	@echo "Library $(NAME) created successfully."

clean:
	@rm -f main
	@rm -f $(OBJ)
	@echo "Cleaned up successfully."

clean-all: clean
	@rm -f $(NAME)
	@echo "All cleaned up successfully."

run: clean-all main
	@./main

valgrind: clean-all main
	@valgrind --leak-check=full --show-leak-kinds=all ./main

re: clean default

re-run: clean run

.PHONY: default main clean run valgrind re re-run