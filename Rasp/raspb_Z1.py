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
import subprocess  # for lora transmit C call
from threading import Thread  # for lora receive thread

# dictionnary of network adresses
NETWORK= {"1":False, "2":False, "3":False}  # possibility to discover the network to assign unique ID
dict_request = {"ID":None, "T":None, "O":None, "NETD":None, "NETS":None}
dict_reqback = {"ID":None, "ACK":None, "R":None, "NETD":None, "NETS":None}
reachableNet = [[0]*2 for i in range(3)]  # matrix of reachable network, 1 line/net contains the active flag and RSSI

ser = serial.Serial(
    port = '/dev/ttyUSB0',
    baudrate = 115200,
    timeout = 1
)

class Receive(Thread):
    def __init__(self, loop = 1):
        Thread.__init__(self)
        self.loraReceived = False  # the thread initialise the flag of lora reception
        self.loop = loop  # numper of loop in reception mode, time for a loop is determined by SF and BW

    def run(self):
        try:
            print("thread start", self.loraReceived)
            proc = subprocess.Popen(["../Lora/Receive", myNet, str(self.loop)], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE) # this function allows to start the sub-process
            #print(proc)
            stdout, stderr = proc.communicate(timeout=300)
            print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
            if len(stdout.decode('utf-8').split("T")) > 1:  # lora payload contain a transmission packet ?
                jsonLora = stdout.decode('utf-8').split("T")[len(stdout.decode('utf-8').split("T")) - 1].strip("\n").split(",")
                if len(jsonLora) == 5:
                    print(jsonLora)
                    dict_request["ID"] = int(jsonLora[0])
                    dict_request["T"] = int(jsonLora[1])
                    dict_request["O"] = int(jsonLora[2])
                    dict_request["NETD"] = int(jsonLora[3])
                    dict_request["NETS"] = int(jsonLora[4])
                    self.loraReceived = "T"
            if len(stdout.decode('utf-8').split("A")) > 1:  # lora payload contain an acknowledge packet ?
                jsonLora = stdout.decode('utf-8').split("A")[len(stdout.decode('utf-8').split("A")) - 1].strip("\n").split(",")
                if len(jsonLora) == 5:
                    print(jsonLora)
                    dict_reqback["ID"] = int(jsonLora[0])
                    dict_reqback["ACK"] = int(jsonLora[1])
                    dict_reqback["R"] = int(jsonLora[2])
                    dict_reqback["NETD"] = int(jsonLora[3])
                    dict_reqback["NETS"] = int(jsonLora[4])
                    self.loraReceived = "A"
            if len(stdout.decode('utf-8').split("D")) > 1:  # lora payload contain a discover packet ?
                jsonLora = stdout.decode('utf-8').split("D")[len(stdout.decode('utf-8').split("D")) - 1].strip("\n").split(",")
                if len(jsonLora) == 2:
                    print(jsonLora)
                    reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
                    reachableNet[int(jsonLora[0]) - 1][1] = int(jsonLora[1])  # save the RSSI
                    self.loraReceived = "D"
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

myNet = str(input('Enter the network number : '))
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
proc = subprocess.Popen(["../Lora/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
stdout, stderr = proc.communicate(timeout=10)
proc = subprocess.Popen(["../Lora/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE) # bug at startup with the GPIO
print(proc)
stdout, stderr = proc.communicate(timeout=10)  # 
print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

# genéraration de clé

# phase de discover

for i in NETWORK.keys():
    if NETWORK.get(i) == False:
        proc = subprocess.Popen(["../Lora/Transmit", "D", str(i), myNet], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        print(proc)


thread_1 = Receive()  # loop = 10 by default
threadInitiated = True
thread_1.start()  # start thread
timeTZigbee = 0  # recording the time of transfer

while True:
    if thread_1.loraReceived:  # thread's flag when valid lora packet received
        print("Lora received")
        if thread_1.loraReceived == "D":  # received a discover packet
            print(reachableNet)




            # ici pour la réception du message de discover




        else:
            if thread_1.loraReceived == "T":  # received a transmission packet
                dump = json.dumps(dict_request).replace(" ","")+"\n"
            else:  # received an acknowledge packet
                dump = json.dumps(dict_reqback).replace(" ","")+"\n"
            print("Sending to Zolertia : ", dump)
            ser.write(bytes(dump, "utf-8"))
            timeTZigbee = time.time()
        #    ser.write((json.dumps(dict_lora).replace(" ","")+"\n").encode())

    if threadInitiated and not thread_1.is_alive():
        #print("waiting end of thread")
        thread_1.join()  # need to join the thread before reinitializing it
        #print("thread ended")
        threadInitiated = False

    if not threadInitiated:
        thread_1 = Receive()  # reset the thread
        threadInitiated = True
        thread_1.start()  # restart the thread

    #print("Listening to the serial port.")
    try:
        zolertia_info=""
        zolertia_info=str(ser.readline().decode("utf-8"))
        if zolertia_info != "":
            print("zolertia info = "+zolertia_info+"\n")
            if zolertia_info[0] == "{":  # zigbee json received
                elapsed = time.time() - timeTZigbee
                print(f'Time of Zigbee transmission : {elapsed:.2}ms')
                zolertiadicback=json.loads(zolertia_info)  # convertion into a dictionnary
                if ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])]==True ) :
                    print("Publishing on the server ")
                elif  ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])]==False ):
                    print("Sending to the lora module")
                    proc = subprocess.Popen(["../Lora/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    thread_1.join()  # we close the reception thread before transmission
                    #penser à enculer le thread
                    threadInitiated = False
                    TimeTLora = time.time()
                    if "ACK" in zolertiadicback.keys():  # return message processing
                        if str(zolertiadicback["R"]) == "led_on" or zolertiadicback["R"] == 1:
                            proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "1"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                            # ex : ../Lora/Transmit A NETD NETS SENSORID ACK R
                            # ex : ../Lora/Transmit A 1 2 1 1 1
                        elif str(zolertiadicback["R"]) == "led_off" or zolertiadicback["R"] == 0:
                            proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                            # ex : ../Lora/Transmit A NETD NETS SENSORID ACK R
                            # ex : ../Lora/Transmit A 1 2 1 1 0
                        else:
                            proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), "0", "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                            # ex : ../Lora/Transmit A NETD NETS SENSORID ACK R
                            # ex : ../Lora/Transmit A 1 2 1 0 0
                    else:
                        proc = subprocess.Popen(["../Lora/Transmit", "T", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["T"]), str(zolertiadicback["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                            # ex : ../Lora/Transmit T NETD NETS SENSORID T O
                            # ex : ../Lora/Transmit A 1 2 1 1 1
                    print(proc)
                    stdout, stderr = proc.communicate(timeout=15)
                    elapsed = time.time() - TimeTLora
                    print(f'Time of Lora : {elapsed:.2}ms')
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
