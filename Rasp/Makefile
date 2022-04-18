CC=gcc
CFLAGS=-Wall -Wextra
EXEC=TestLora
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
BASE= SX1272.o gpio_util.o RF_LoRa_868_SO.o sendRecept.o

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	make clean
#	$(CC) -o $@ SX1272.o gpio_util.o RF_LoRa_868_SO.o filecsv.o sendRecept.o TestLora.o $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

Init: $(OBJ)
#	rm -vrf Receive.o Transmit.o
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)
	make clean

Receive: $(OBJ)
#	rm -vrf Init.o Transmit.o
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)
	make clean

Transmit: $(OBJ)
#	rm -vrf Init.o Receive.o
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)
	make clean

clean:
	rm -vrf *.o

cleanall: clean
	rm -vrf $(EXEC) Init Receive Transmit

#CFLAGS=-W -Wall -ansi -pedantic -std=c99 -g
#INC=-I ./
# SRC=./
# $(EXEC) : $(SRC)TestLora.c $(SRC)operation.o 
#     $(CC) $(INC) -o $(SRC)$@ $^ $(CFLAGS) 
 
# $(SRC)%.o : $(SRC)%.c
#     $(CC) $(INC) -o $@ -c $< $(CFLAGS) 

# all:
# 	rm calculette.exe calculette.lex.cpp calculette.bison.cpp calculette.bison.h
# 	win_bison -d calculette.y -o calculette.bison.cpp -Wcounterexamples
# 	win_flex -o calculette.lex.cpp calculette.l
# 	g++ calculette.lex.cpp calculette.bison.cpp -o calculette
# 	./calculette test2.txt