"""
This script gets the orders from the client and sends it to the zolertia C script.
It also gets messages from the network.

authors : MAHRAZ Anasset and ROBYNS Jonathan
"""

import paho.mqtt.client as mqtt
import time
import serial
import random
from datetime import datetime
import pytz
import json
import subprocess  # for lora transmit C code
from threading import Thread  # for lora receive thread

DEBUG = 1

#dictionnaire des adresses réseaux
NETWORK= {"1":False, "2":True, "3":False}
dict_lora = {"ID":None, "T":None, "O":None, "NET":None}

ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate = 115200,
    timeout=1
)

class Receive(Thread):
    def __init__(self, loraReceived, loop = 10):
        Thread.__init__(self)
        self.loraReceived = loraReceived 
        self.loop = loop

    def run(self):
        try:
            proc = subprocess.Popen(["../Rasp/Receive", myNet], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            if DEBUG:
                print(proc)
            stdout, stderr = proc.communicate(timeout=self.loop*2)
            if DEBUG:
                print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
            stdout = stdout.decode('utf-8').split("J")[1].strip("\n").split(",")
            if DEBUG:
                print(stdout)
            dict_lora["ID"] = int(stdout[0])
            dict_lora["T"] = int(stdout[1])
            dict_lora["O"] = int(stdout[2])
            dict_lora["NET"] = int(stdout[3])
            self.loraReceived = True
        
        except Exception as e:
            print(e)
            self.loraReceived = False
            return

def my_debug_message(msg):
    """
    msg (string) : debug message to write in infor_debug.txt

    This fonction has no return value. It simply writes msg in "info_debug.txt"
    """
    with open("info_debug.txt",'a') as debug_file:
        debug_file.write(msg)
        debug_file.write("\n")

def my_on_message(client,userdata,message):
    """
    message (string) : the message we get

    This function trigers each time we get a mqtt message
    """
    try:
        print("Received message '" + message.payload.decode("utf-8")
        + "' on topic '" + message.topic
        + "' with QoS " + str(message.qos)+"\n")
        network=json.loads(message.payload.decode("utf8"))#transformer le message reçu en dictionnaire 
        if ( (str(network["NET"]) in NETWORK ) and NETWORK[str(network["NET"])]==True ):
            if DEBUG:
                print("j'envoie à mon zolertia")
            ser.write(bytes(message.payload.decode("utf-8")+"\n",'utf-8'))

        elif  ( (str(network["NET"]) in NETWORK ) and NETWORK[str(network["NET"])]==False ):
            if DEBUG:
                print("j'envoie à mon Module LoRa")
                start = time.time()

            proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NET"]), myNet, str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            if DEBUG:
                print(proc)
            stdout, stderr = proc.communicate(timeout=15)

            if DEBUG:
                elapsed = time.time() - start
                print(f'Temps d\'exécution : {elapsed:.2}ms')
                print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

        else :
            if DEBUG:
                print("le réseaux selectionné n'existe pas ")

    except Exception as e:
        print(e)

mqttc = mqtt.Client()
mqttc.on_message = my_on_message
mqttc.connect("test.mosquitto.org", 1883, 60)
mqttc.subscribe("/EBalanceplus/order",2)

mqttc.loop_start()

if DEBUG:
    print("Init LoRa Module")
proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
if DEBUG:
    print(proc)
stdout, stderr = proc.communicate(timeout=10)
if DEBUG:
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

myNet = ""
threadInitiated = False
loraReceived = False

for key in NETWORK.keys():
    if NETWORK[key] == True:
        myNet = key

thread_1 = Receive(loraReceived)  # loop = 10 by default

while True:
    if threadInitiated and not thread_1.is_alive():
        if DEBUG:
            print("wainting end of thread")
        thread_1.join()
        threadInitiated = False
        if DEBUG:
            print("thread ended")
    if not threadInitiated:
        thread_1 = Receive(loraReceived)
        threadInitiated = True
        #if not thread_1.is_alive():
        thread_1.start()
        if DEBUG:
            print("thread started")
    #proc = subprocess.Popen(["../Rasp/Receive", myNet, 2], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    #print(proc) # A mettre dans un thread
    #stdout, stderr = proc.communicate(timeout=60)
    #print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
    #if len(stdout.decode('utf-8').split("J")) > 1:
    #    stdout = stdout.decode('utf-8').split("J")[1].strip("\n").split(",")
    #    print(stdout)
    #    dict_lora["ID"] = int(stdout[0])
    #    dict_lora["T"] = int(stdout[1])
    #    dict_lora["O"] = int(stdout[2])
    #    dict_lora["NET"] = int(stdout[3])

    if loraReceived:
        if DEBUG:
            print("Lora received")
        if len(stdout.decode('utf-8').split("J")) > 1:
            dump = json.dumps(dict_lora).replace(" ","")+"\n"
            if DEBUG:
                print(dump, bytes(dump, "utf-8"))
            ser.write(bytes(dump, "utf-8"))
    #        ser.write((json.dumps(dict_lora).replace(" ","")+"\n").encode())
    if DEBUG:
        #print("Listening to the serial port.")
    try:
        zolertia_info=""
        zolertia_info=str(ser.readline().decode("utf-8"))
        if zolertia_info != "":
            if DEBUG:
                print("zolertia info = "+zolertia_info+"\n")
            if zolertia_info[0] == "{":
                zolertiadicback=json.loads(zolertia_info) # convertion into a dictionnary
                if ( (str(zolertiadicback["NET"]) in NETWORK ) and NETWORK[str(zolertiadicback["NET"])]==True ) :
                    if DEBUG:
                        print("je le publie dans mon server ")
                    s=mqttc.publish("/EBalanceplus/order_back",zolertia_info)
                elif  ( (str(zolertiadicback["NET"]) in NETWORK ) and NETWORK[str(zolertiadicback["NET"])]==False ):
                    if DEBUG:
                        print("j'envoie à mon Module LoRa")
                        start = time.time()

                    proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NET"]), myNet, str(network["ID"]), str(network["ACK"]), str(network["R"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    if DEBUG:
                        print(proc)
                    stdout, stderr = proc.communicate(timeout=15)

                    if DEBUG:
                        elapsed = time.time() - start
                        print(f'Temps de transmission : {elapsed:.2}ms')
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
