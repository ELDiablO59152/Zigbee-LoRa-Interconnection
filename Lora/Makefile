CC:=gcc
CFLAGS:=-Wall -Wextra
SOURCE:=src
SOURCE_ASCON:=src/ascon
BUILD:=obj
SRC:=$(wildcard $(SOURCE)/*.c)
ASCON:=$(wildcard $(SOURCE_ASCON)/*.c)
OBJ:=$(addprefix $(BUILD)/, $(notdir $(SRC:.c=.o)))
OBJ_ASCON:=$(addprefix $(BUILD)/, $(notdir $(ASCON:.c=.o)))
EXEC:=TestLora
FINAL:=Init API Receive Transmit
BASE:=filecsv.o gpio_util.o RF_LoRa_868_SO.o sendRecept.o SX1272.o ascon_utils.o aead.o printstate.o
LOG:=../Rasp/data.csv ../Rasp/info_debug.txt

all: $(FINAL)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(BUILD)/%.o: $(SOURCE)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BUILD)/%.o: $(SOURCE_ASCON)/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

Init: $(OBJ) $(OBJ_ASCON)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

API: $(OBJ) $(OBJ_ASCON)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

Receive: $(OBJ) $(OBJ_ASCON)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

Transmit: $(OBJ) $(OBJ_ASCON)
	$(CC) -o $@ $(addprefix $(BUILD)/, $(BASE)) $(BUILD)/$@.o $(CFLAGS)

clean:
	rm -vrf $(BUILD)/*.o

cleanall: clean
	rm -vrf $(EXEC) $(FINAL) $(LOG)