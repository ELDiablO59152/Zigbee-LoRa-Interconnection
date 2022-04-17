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
NETWORK= {"1":True ,"2":False,"3":False}

ser = serial.Serial(
        port='/dev/ttyUSB0',
        baudrate = 115200,
)

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
                network=json.loads(message.payload.decode("utf8"))#transformer le message reçu en dicttionnaire 
                if ( (str(network["NET"]) in NETWORK ) and NETWORK[str(network["NET"])]==True ):
                    print("j'envoie à mon zolertia")
                    ser.write(bytes(message.payload.decode("utf-8")+"\n",'utf-8'))

                elif  ( (str(network["NET"]) in NETWORK ) and  NETWORK[str(network["NET"])]==False ):
                    print("j'envoie à mon Module LoRa")

                    proc = subprocess.Popen(["../Rasp/Transmit ", str(network["NET"]), str(network["ID"]), "LED_ON"], shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    print(proc)

                    stdout, stderr = proc.communicate(timeout=15)
                    print("Output:\n", stdout)

                else :
                        print("le réseaux selectionné n'existe pas ")
                
        except Exception as e:
                print(e)

mqttc = mqtt.Client()
mqttc.on_message = my_on_message
mqttc.connect("test.mosquitto.org", 1883, 60)
mqttc.subscribe("/EBalanceplus/order",2)

mqttc.loop_start()
while True:
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
