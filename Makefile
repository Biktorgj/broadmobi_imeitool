all: clean imeitool

imeitool:
	@$(CC) $(CFLAGS) -Wall imeitool.c -o imeitool
	@chmod +x imeitool

clean:
	@rm -rf imeitool
