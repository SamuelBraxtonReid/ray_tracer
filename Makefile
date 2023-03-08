SOURCES = statistics.c scene.c colors.c timer.c matrix.c parse.c search.c image.c main.c
EXCLUDE = -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
OPT = -O3

all:
	clang -std=c99 -Wall -Wextra -Wpedantic -Werror $(EXCLUDE) $(OPT) $(SOURCES)

clang:
	clang -std=c99 -Wall -Wextra -Wpedantic -Werror $(EXCLUDE) $(OPT) $(SOURCES) -o clang

gcc:
	gcc-11 -std=c99 -Wall -Wextra -Wpedantic -Werror $(EXCLUDE) $(OPT) $(SOURCES) -o gcc

clean:
	rm a.out clang gcc
       
