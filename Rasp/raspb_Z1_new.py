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

def my_debug_message(msg):
    """
    msg (string) : debug message to write in infor_debug.txt

    This fonction has no return value. It simply writes msg in "info_debug.txt"
    """
    with open("info_debug.txt",'a') as debug_file:
        debug_file.write(msg)
        debug_file.write("\n")

myNet = str(input('Enter the network number : '))
for key in NETWORK.keys():
    if key == myNet:
        NETWORK[key] = True

print("Init LoRa Module")
proc = subprocess.Popen(["../Lora/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
stdout, stderr = proc.communicate(timeout=10)
print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

'''thread_1 = Receive()  # loop = 10 by default
threadInitiated = True
thread_1.start()  # start thread
timeTZigbee = 0  # recording the time of transfer'''

loop=100
loraReceived = ""
try:
    print("Process receive")
    proc = subprocess.Popen(["../Lora/Receive", myNet, str(loop)], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE) # this function allows to start the sub-process

    while(True):

        stdout=proc.stdout.read().decode()
        if(stdout!=""):
            #Tell the receive process to stop
            proc.stdin.write("Test ici")
            stdout, stderr = proc.communicate(timeout=10)
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
                    loraReceived = "T"
            if len(stdout.decode('utf-8').split("A")) > 1:  # lora payload contain an acknowledge packet ?
                jsonLora = stdout.decode('utf-8').split("A")[len(stdout.decode('utf-8').split("A")) - 1].strip("\n").split(",")
                if len(jsonLora) == 5:
                    print(jsonLora)
                    dict_reqback["ID"] = int(jsonLora[0])
                    dict_reqback["ACK"] = int(jsonLora[1])
                    dict_reqback["R"] = int(jsonLora[2])
                    dict_reqback["NETD"] = int(jsonLora[3])
                    dict_reqback["NETS"] = int(jsonLora[4])
                    loraReceived = "A"
            if len(stdout.decode('utf-8').split("D")) > 1:  # lora payload contain a discover packet ?
                jsonLora = stdout.decode('utf-8').split("D")[len(stdout.decode('utf-8').split("D")) - 1].strip("\n").split(",")
                if len(jsonLora) == 2:
                    print(jsonLora)
                    reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
                    reachableNet[int(jsonLora[0]) - 1][1] = int(jsonLora[1])  # save the RSSI
                    loraReceived = "D"
            print("Message type received: ", loraReceived)


            if loraReceived:  #flag when valid lora packet received
                print("Lora received")
               
                if loraReceived == "D":  # received a discover packet
                    print(reachableNet)
                else:
                    if loraReceived == "T":  # received a transmission packet
                        dump = json.dumps(dict_request).replace(" ","")+"\n"
                    else:  # received an acknowledge packet
                        dump = json.dumps(dict_reqback).replace(" ","")+"\n"
                    print("Sending to Zolertia : ", dump)
                    ser.write(bytes(dump, "utf-8"))
                    timeTZigbee = time.time()
                #    ser.write((json.dumps(dict_lora).replace(" ","")+"\n").encode())

            
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

                                TimeTLora = time.time()
                                if "ACK" in zolertiadicback.keys():  # return message processing
                                    if str(zolertiadicback["R"]) == "led_on" or zolertiadicback["R"] == 1:
                                        proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "1"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                                        # ex : ../Lora/Transmit A NETD NETS SENDORID ACK R
                                        # ex : ../Lora/Transmit A 1 2 1 1 1
                                    elif str(zolertiadicback["R"]) == "led_off" or zolertiadicback["R"] == 0:
                                        proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), str(zolertiadicback["ACK"]), "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                                        # ex : ../Lora/Transmit A NETD NETS SENDORID ACK R
                                        # ex : ../Lora/Transmit A 1 2 1 1 0
                                    else:
                                        proc = subprocess.Popen(["../Lora/Transmit", "A", str(zolertiadicback["NETD"]), myNet, str(zolertiadicback["ID"]), "0", "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                                        # ex : ../Lora/Transmit A NETD NETS SENDORID ACK R
                                        # ex : ../Lora/Transmit A 1 2 1 0 0
                                else:
                                    proc = subprocess.Popen(["../Lora/Transmit", "T", str(network["NETD"]), myNet, str(network["ID"]), str(network["T"]), str(network["O"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                                        # ex : ../Lora/Transmit T NETD NETS SENDORID T O
                                        # ex : ../Lora/Transmit A 1 2 1 1 1
                                
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
                    #Tell the receive process to start again
                    #

except Exception as e:
    print(e)
    #self.loraReceived = False
