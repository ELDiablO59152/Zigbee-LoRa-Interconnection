CC:=gcc
CFLAGS:=-Wall -Wextra
SOURCE:=src
BUILD:=obj
SRC:=$(wildcard $(SOURCE)/*.c)
OBJ:=$(addprefix $(BUILD)/, $(notdir $(SRC:.c=.o)))
EXEC:=TestLora
FINAL:=Init Receive Transmit
BASE:=filecsv.o gpio_util.o RF_LoRa_868_SO.o sendRecept.o SX1272.o

all: $(FINAL)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(BUILD)/%.o: $(SOURCE)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

Init: $(OBJ)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

Receive: $(OBJ)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

Transmit: $(OBJ)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

clean:
	rm -vrf $(BUILD)/*.o

cleanall: clean
	rm -vrf $(EXEC) $(FINAL) data.csv info_debug.txt