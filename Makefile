all: build

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

copy_help_files:
	@echo -n "Copying help files..."
	@if [ ! -d "$(HOME)/.local/share/vine/" ]; then mkdir $(HOME)/.local/share/vine/; fi
	@cp help.txt $(HOME)/.local/share/vine/help.txt
	@echo "OK."

build: copy_help_files
	@echo -n "Compiling..."
	@$(CC) $(CFLAGS) vine.c -o ./vine
	@echo "OK."
	@echo "Installation completed."

install: copy_help_files
	@echo -n "Compiling..."
	@sudo $(CC) $(CFLAGS) vine.c -o /usr/local/bin/vine
	@echo "OK."
	@echo "Installation completed."

clean:
	@if [ -d "$(HOME)/.local/share/vine/" ]; then rm -rf $(HOME)/.local/share/vine/; fi
	@if [ -f "/usr/local/bin/vine" ]; then sudo rm /usr/local/bin/vine; fi
	@if [ -f "./vine" ]; then rm ./vine; fi
