CC = gcc
CFLAGS = -Wall -Wextra
OBJ = principal.o voter.o
DEPS = voter.h -lrt -lpthread

all: practica2

# Regla para compilar el ejecutable
practica2: $(OBJ)
	$(CC) $(CFLAGS) -o voting $(OBJ)

# Regla para compilar archivos .c a .o
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpia archivos objeto y el ejecutable
clean:
	rm -f $(OBJ) voting