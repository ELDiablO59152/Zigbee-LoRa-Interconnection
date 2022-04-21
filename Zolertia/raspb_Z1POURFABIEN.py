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

#dictionnaire des adresses réseaux
NETWORK= {"1":False, "2":True, "3":False}  # possibilité de découvrir le réseau pour s'assigner un ID unique
dict_request = {"ID":None, "T":None, "O":None, "NET":None}
dict_reqback = {"ID":None, "ACK":None, "R":None, "NET":None}

ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate = 115200,
    timeout=1
)

class Receive(Thread):
    def __init__(self, loop = 10):
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
                stdout = stdout.decode('utf-8').split("T")[1].strip("\n").split(",")
                print(stdout)
                dict_request["ID"] = int(stdout[0])
                dict_request["T"] = int(stdout[1])
                dict_request["O"] = int(stdout[2])
                dict_request["NET"] = int(stdout[3])
                self.loraReceived = True
            if len(stdout.decode('utf-8').split("A")) > 1:
                stdout = stdout.decode('utf-8').split("A")[1].strip("\n").split(",")
                print(stdout)
                dict_reqback["ID"] = int(stdout[0])
                dict_reqback["ACK"] = int(stdout[1])
                dict_reqback["R"] = int(stdout[2])
                dict_reqback["NET"] = int(stdout[3])
                self.loraReceived = True
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
            print("j'envoie à mon zolertia")
            ser.write(bytes(message.payload.decode("utf-8")+"\n",'utf-8'))

        elif  ( (str(network["NET"]) in NETWORK ) and NETWORK[str(network["NET"])]==False ):
            start = time.time()
            proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NET"]), myNet, str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            print(proc)
            stdout, stderr = proc.communicate(timeout=15)
            elapsed = time.time() - start
            print(f'Temps d\'exécution : {elapsed:.2}ms')
            print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

        else :
            print("le réseaux selectionné n'existe pas ")

    except Exception as e:
        print(e)

mqttc = mqtt.Client()
mqttc.on_message = my_on_message
mqttc.connect("test.mosquitto.org", 1883, 60)
mqttc.subscribe("/EBalanceplus/order",2)

mqttc.loop_start()

print("Init LoRa Module")
proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
print(proc)
stdout, stderr = proc.communicate(timeout=10)
print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

myNet = ""
for key in NETWORK.keys():
    if NETWORK[key] == True:
        myNet = key

thread_1 = Receive()  # loop = 10 by default
threadInitiated = True
thread_1.start()

while True:
    if thread_1.loraReceived:
        print("Lora received")
        dump = json.dumps(dict_request).replace(" ","")+"\n"
        print(dump, bytes(dump, "utf-8"))
        ser.write(bytes(dump, "utf-8"))
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
                zolertiadicback=json.loads(zolertia_info) # convertion into a dictionnary
                if ( (str(zolertiadicback["NET"]) in NETWORK ) and NETWORK[str(zolertiadicback["NET"])]==True ) :
                    print("je le publie dans mon server ")
                    s=mqttc.publish("/EBalanceplus/order_back",zolertia_info)
                elif  ( (str(zolertiadicback["NET"]) in NETWORK ) and NETWORK[str(zolertiadicback["NET"])]==False ):
                    print("j'envoie à mon Module LoRa")
                    start = time.time()
                    if zolertiadicback.has_key("ACK"):
                        if str(zolertiadicback["R"]) == "led_on" or zolertiadicback["R"] == 1:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NET"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "1"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                        elif str(zolertiadicback["R"]) == "led_off" or zolertiadicback["R"] == 0:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NET"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                        else:
                            proc = subprocess.Popen(["../Rasp/Transmit", "A", str(zolertiadicback["NET"]), myNet, str(zolertiadicback["ID"]), "0", "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                        
                    else:
                        proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NET"]), myNet, str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    print(proc)
                    stdout, stderr = proc.communicate(timeout=15)
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
