objects = src/main.o src/system.o src/auth.o src/db.o

atm : $(objects)
	cc -o atm $(objects) -lsqlite3

src/main.o : src/header.h
src/system.o : src/header.h
src/auth.o : src/header.h
src/db.o : src/header.h
clean :
	rm -f $(objects) atm
