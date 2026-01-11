CC = gcc
CFLAGS = -Wall -Wextra -pthread
LIBS = -lglut -lGLU -lGL -lm

OBJS = main.o maze.o multithreading.o father.o female.o graphics.o
TARGET = program

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

maze.o: maze.c
	$(CC) $(CFLAGS) -c maze.c

multithreading.o: multithreading.c
	$(CC) $(CFLAGS) -c multithreading.c

father.o: father.c
	$(CC) $(CFLAGS) -c father.c

female.o: female.c
	$(CC) $(CFLAGS) -c female.c

graphics.o: graphics.c
	$(CC) $(CFLAGS) -c graphics.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
