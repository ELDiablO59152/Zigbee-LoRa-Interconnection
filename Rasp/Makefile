CC=gcc
CFLAGS=-Wall -Wextra
EXEC=TestLora
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)
FINAL=Init Receive Transmit
BASE=SX1272.o gpio_util.o RF_LoRa_868_SO.o filecsv.o sendRecept.o

all: $(FINAL)
#	make clean

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

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
	rm -vrf $(EXEC) $(FINAL) data.csv info_debug.txt