CC = cc
CFLAGS = -Wall -Wextra
objects = src/main.o src/system.o src/auth.o src/db.o

atm : $(objects)
	$(CC) $(CFLAGS) -o atm $(objects) -lsqlite3

%.o : %.c src/header.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	rm -f $(objects) atm
