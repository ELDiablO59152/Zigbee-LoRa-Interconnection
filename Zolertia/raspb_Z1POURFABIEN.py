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
import subprocess # for lora transmit C code
from threading import Thread # for lora receive thread



#dictionnaire des adresses réseaux
NETWORK= {"1":False, "2":True, "3":False}
dict_lora = {"ID":None,"T":None,"O":None,"NET":None}

ser = serial.Serial(
        port='/dev/ttyUSB0',
        baudrate = 115200,
)

class Receive(Thread):
    def __init__(self, loop=10):
        Thread.__init__(self)
        self.loop = str(loop)

    def run(self):
        proc = subprocess.Popen(["../Rasp/Receive", self.loop], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        print(proc)
        stdout, stderr = proc.communicate(timeout=self.loop*2)
        print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

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
                        print("j'envoie à mon Module LoRa")

                        start = time.time()
                        proc = subprocess.Popen(["../Rasp/Transmit", "T", str(network["NET"]), str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
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

proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
print(proc)

stdout, stderr = proc.communicate(timeout=10)
print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

while True:
        '''thread_1 = Receive()
        if thread_1.is_alive() == False:
                 thread_1.start()
                 thread_1.join()'''
        proc = subprocess.Popen(["../Rasp/Receive", "2"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        print(proc) # A mettre dans un thread

        stdout, stderr = proc.communicate(timeout=60)
        print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
        
        if len(stdout.decode('utf-8').split("J")) > 1:
            stdout = stdout.decode('utf-8').split("J")[1].strip("\n").split(",")
            print(stdout)
            dict_lora["ID"] = int(stdout[0])
            dict_lora["T"] = int(stdout[1])
            dict_lora["O"] = int(stdout[2])
            dict_lora["NET"] = int(stdout[3])
            dump = json.dumps(dict_lora).replace(" ","")+"\n"
            print(dump, bytes(dump, "utf-8"), len(dump))
            ser.write(bytes(dump, "utf-8"))
#            ser.write((json.dumps(dict_lora)+"\n").encode())

        print("Listening to the serial port.")
        zolertia_info=str(ser.readline().decode("utf-8"))
        print("zolertia info = "+zolertia_info+"\n")
        if zolertia_info[0] == "{":
                s=mqttc.publish("/EBalanceplus/order_back",zolertia_info)

        else : # debug messagess
                now = datetime.now()
                now=now.replace(tzinfo=pytz.utc)
                now=now.astimezone(pytz.timezone("Europe/Paris"))
                current_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                my_debug_message(current_time + zolertia_info)

