"""
This script gets the orders from the client and sends it to the zolertia C script.
It also gets messages from the network.

authors : MAHRAZ Anasset and ROBYNS Jonathan
"""

import time
import serial
import random
from datetime import datetime
import pytz
import json
import subprocess  # for lora transmit C code
from threading import Thread  # for lora receive thread

#dictionnaire des adresses réseaux
NETWORK= {"1":False, "2":False, "3":False}  # possibilité de découvrir le réseau pour s'assigner un ID unique
dict_request = {"ID":None, "T":None, "O":None, "NETD":None, "NETS":None}
dict_reqback = {"ID":None, "ACK":None, "R":None, "NETD":None, "NETS":None}

ser = serial.Serial(
    port = '/dev/ttyUSB0',
    baudrate = 115200,
    timeout = 1
)

class Receive(Thread):
    def __init__(self, loop = 1):
        Thread.__init__(self)
        self.loraReceived = False
        self.loop = loop

    def run(self):
        try:
            print("thread start", self.loraReceived)
            proc = subprocess.Popen(["../Rasp/Receive", myNet, str(self.loop)], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            #print(proc)
            stdout, stderr = proc.communicate(timeout=self.loop*2)
            print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
            if len(stdout.decode('utf-8').split("T")) > 1:
                stdout = stdout.decode('utf-8').split("T")[len(stdout.decode('utf-8').split("T")) - 1].strip("\n").split(",")
                print(stdout)
                if len(stdout) == 5:
                    dict_request["ID"] = int(stdout[0])
                    dict_request["T"] = int(stdout[1])
                    dict_request["O"] = int(stdout[2])
                    dict_request["NETD"] = int(stdout[3])
                    dict_request["NETS"] = int(stdout[4])
                    self.loraReceived = "T"
            if len(stdout.decode('utf-8').split("A")) > 1:
                stdout = stdout.decode('utf-8').split("A")[len(stdout.decode('utf-8').split("A")) - 1].strip("\n").split(",")
                print(stdout)
                if len(stdout) == 5:
                    dict_reqback["ID"] = int(stdout[0])
                    dict_reqback["ACK"] = int(stdout[1])
                    dict_reqback["R"] = int(stdout[2])
                    dict_reqback["NETD"] = int(stdout[3])
                    dict_request["NETS"] = int(stdout[4])
                    self.loraReceived = "A"
            print("thread end", self.loraReceived)
            return
        except Exception as e:
            print(e)
            #self.loraReceived = False
            return

def my_debug_message(msg):
    """
    msg (string) : debug message to write in infor_debug.txt

    This fonction has no return value. It simply writes msg in "info_debug.txt"
    """
    with open("info_debug.txt",'a') as debug_file:
        debug_file.write(msg)
        debug_file.write("\n")

myNet = raw_input('Entrez le numéro de réseau : ')
if myNet == "1":
    NETWORK["1"] = True
elif myNet == "2":
    NETWORK["2"] = True
elif myNet == "3":
    NETWORK["3"] = True
else:
    NETWORK["1"] = True
"""myNet = ""
for key in NETWORK.keys():
    if NETWORK[key] == True:
        myNet = key"""

print("Init LoRa Module")
proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
print(proc)
stdout, stderr = proc.communicate(timeout=10)
print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

thread_1 = Receive()  # loop = 10 by default
threadInitiated = True
thread_1.start()
timeTZigbee = 0

while True:
    if thread_1.loraReceived:
        print("Lora received")
        if thread_1.loraReceived == "T":
            dump = json.dumps(dict_request).replace(" ","")+"\n"
        else
            dump = json.dumps(dict_reqback).replace(" ","")+"\n"
        print(dump, bytes(dump, "utf-8"))
        ser.write(bytes(dump, "utf-8"))
        timeTZigbee = time.time()
    #    ser.write((json.dumps(dict_lora).replace(" ","")+"\n").encode())

    if threadInitiated and not thread_1.is_alive():
        print("waiting end of thread")
        thread_1.join()
        print("thread ended")
        threadInitiated = False

    if not threadInitiated:
        thread_1 = Receive()
        threadInitiated = True
        thread_1.start()

    #print("Listening to the serial port.")
    try:
        zolertia_info=""
        zolertia_info=str(ser.readline().decode("utf-8"))
        if zolertia_info != "":
            print("zolertia info = "+zolertia_info+"\n")
            if zolertia_info[0] == "{":
                elapsed = time.time() - timeTZigbee
                print(f'Temps de transmission Zigbee: {elapsed:.2}ms')
                zolertiadicback=json.loads(zolertia_info)  # convertion into a dictionnary
                if ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])]==True ) :
                    print("je le publie dans mon server ")
                elif  ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])]==False ):
                    print("j'envoie à mon Module LoRa")
                    TimeTLora = time.time()
                    if zolertiadicback.has_key("ACK"):  # traitement du message de retour
                        if str(zolertiadicback["R"]) == "led_on" or zolertiadicback["R"] == 1:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "1"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                        elif str(zolertiadicback["R"]) == "led_off" or zolertiadicback["R"] == 0:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                        else:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), "0", "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    else:
                        proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NETD"]), myNet, str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    print(proc)
                    stdout, stderr = proc.communicate(timeout=15)
                    elapsed = time.time() - TimeTLora
                    print(f'Temps de transmission Lora: {elapsed:.2}ms')
                    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

            else : # debug messages
                now = datetime.now()
                now=now.replace(tzinfo=pytz.utc)
                now=now.astimezone(pytz.timezone("Europe/Paris"))
                current_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                my_debug_message(current_time + zolertia_info)

    except Exception as e:
        print(e)
        thread_1.join()
        break
