CC=g++
FILE=main.cpp
STD=-std=c++2a
FLAGS=-g --all-warnings -lgit2 -lfmt
EMPTY=

main: $(FILE:.cpp=.o)
	@mkdir -p bin
	$(CC) main.o ${STD} ${FLAGS} -o bin/main

$(FILE:.cpp=.o): ${FILE}
	$(CC) -c $(FILE) ${STD} ${FLAGS} -o $(FILE:.cpp=.o)

run: bin/main
	@echo '========================================'
	@./bin/main

clean:
	@touch main.o bin/main
	@rm main.o bin/main
	@echo cleaning done
