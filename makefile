CC = g++	
DEBUG = -g	
CFLAGS = -Wall -std=c++0x -c $(DEBUG) -O0 -pedantic-errors 	
LFLAGS = -Wall $(DEBUG)	

test: test.o
	$(CC) $(LFLAGS) -o test test.o
test.o: test.cpp
	$(CC) $(CFLAGS) test.cpp
test2: test2.o
	$(CC) $(LFLAGS) -o test2 test2.o
test2.o: test2.cpp
	$(CC) $(CFLAGS) test2.cpp
lab13: main.o	
	$(CC) $(LFLAGS) -o lab13 main.o
main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp	
run:	lab13	
	./lab13	 		
clean:	
	rm -rf lab13
	rm -rf test2
	rm -rf test	 		
	rm -rf *.o
