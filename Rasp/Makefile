CC=gcc
CFLAGS=-Wall -Wextra
EXEC=TestLora
SRC= $(wildcard *.c)
OBJ= $(SRC:.c=.o)
FINAL=Init Receive Transmit
BASE=SX1272.o gpio_util.o RF_LoRa_868_SO.o filecsv.o sendRecept.o

all: $(FINAL)
	make clean
	cd ../Zolertia

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
#	$(CC) -o $@ SX1272.o gpio_util.o RF_LoRa_868_SO.o filecsv.o sendRecept.o TestLora.o $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

Init: $(OBJ)
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)

Receive: $(OBJ)
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)

Transmit: $(OBJ)
	$(CC) -o $@ $(BASE) $@.o $(CFLAGS)

clean:
	rm -vrf *.o

cleanall: clean
	rm -vrf $(EXEC) $(FINAL)

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