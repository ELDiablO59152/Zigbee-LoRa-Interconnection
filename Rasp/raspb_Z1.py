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
from fcntl import fcntl, F_GETFL, F_SETFL
from os import O_NONBLOCK, read

print("LoRa ZigBee v0.1")

# dictionnary of network adresses
NETWORK= {"1": False, "2": False, "3": False}  # possibility to discover the network to assign unique ID
dict_request = {"ID": None, "T": None, "O": None, "NETD": None, "NETS": None}
dict_reqback = {"ID": None, "ACK": None, "R": None, "NETD": None, "NETS": None}
reachableNet = [[0]*2 for i in range(3)]  # matrix of reachable network, 1 line/net contains the active flag and RSSI

try:
    ser = serial.Serial(
        port = '/dev/ttyUSB0',
        baudrate = 115200,
        timeout = 0.5
    )
except Exception as e:
    print("\n\nHave you plugged in the Zolertia ?\n\n")
    raise

def my_debug_message(msg):
    """
    msg (string) : debug message to write in infor_debug.txt

    This fonction has no return value. It simply writes msg in "info_debug.txt"
    """
    with open("info_debug.txt",'a') as debug_file:
        debug_file.write(f"{msg}\n")

myNet = str(input('Enter the network number : '))
for key in NETWORK.keys():
    if key == myNet:
        NETWORK[key] = True

print("Init LoRa Module")
try:
    proc = subprocess.Popen(["../Lora/Init"], shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate(timeout=10)
    print("Output:\n", stdout.decode(), stderr.decode())
except Exception as e:
    print(e, "\n\nHave you run the make command or are you in the Rasp directory ?\n\n")
    raise

timeTZigbee = 0  # recording the time of transfer
loop = 0  # 0 means infinit
loraReceived = ""
try:
    proc = subprocess.Popen(["../Lora/API", myNet, str(loop)], shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=0)  # this function allows to start the sub-process
    print(proc)
    
    flags = fcntl(proc.stdout, F_GETFL)  # reading output of proc asynchronously
    fcntl(proc.stdout, F_SETFL, flags | O_NONBLOCK)

    while True:
        stdout, stderr = b'', b''
        stdout = proc.stdout.read()
        if stdout and stdout[-1] != b'\n':
            tmp = proc.stdout.read()
            if tmp:
                stdout += tmp
        # stderr = proc.stderr.read() # takes too much time
        if stdout or stderr: print("API output:\n", stdout.decode(), stderr.decode())

        if stdout:  # interpretation of the LoRa program output
            # if len(stdout.decode().split("T")[len(stdout.decode().split("T")) - 1].split("\n")[0].split(",")) == 5 and len(stdout.decode().split("T")[len(stdout.decode().split("T")) - 1].split("\n")[0].split(",")[0]) == 1:
            #     jsonLora = stdout.decode().split("T")[len(stdout.decode().split("T")) - 1].split("\n")[0].split(",")  # lora payload contain a transmission packet ?
            #     print(jsonLora)
            #     dict_request["ID"] = int(jsonLora[2])
            #     dict_request["T"] = int(jsonLora[3])
            #     dict_request["O"] = int(jsonLora[4])
            #     dict_request["NETD"] = int(jsonLora[0])
            #     dict_request["NETS"] = int(jsonLora[1])
            #     reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
            #     loraReceived = "T"
            # elif len(stdout.decode().split("A")[len(stdout.decode().split("A")) - 1].split("\n")[0].split(",")) == 5 and len(stdout.decode().split("A")[len(stdout.decode().split("A")) - 1].split("\n")[0].split(",")[0]) == 1:
            #     jsonLora = stdout.decode().split("A")[len(stdout.decode().split("A")) - 1].split("\n")[0].split(",") # lora payload contain an acknowledge packet ?
            #     print(jsonLora)
            #     dict_reqback["ID"] = int(jsonLora[2])
            #     dict_reqback["ACK"] = int(jsonLora[3])
            #     dict_reqback["R"] = int(jsonLora[4])
            #     dict_reqback["NETD"] = int(jsonLora[0])
            #     dict_reqback["NETS"] = int(jsonLora[1])
            #     reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
            #     loraReceived = "A"
            if len(stdout.decode().split("D")[len(stdout.decode().split("D")) - 1].split("\n")[0].split(",")) == 2 and len(stdout.decode().split("D")[len(stdout.decode().split("D")) - 1].split("\n")[0].split(",")[0]) == 1:
                jsonLora = stdout.decode().split("D")[len(stdout.decode().split("D")) - 1].split("\n")[0].split(",")  # lora payload contain a discover packet ?
                print(jsonLora)
                reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
                reachableNet[int(jsonLora[0]) - 1][1] = int(jsonLora[1])  # save the RSSI
                loraReceived = "D"
            elif len(stdout.decode().split("P")[len(stdout.decode().split("P")) - 1].split("\n")[0].split(",")) == 2 and len(stdout.decode().split("P")[len(stdout.decode().split("P")) - 1].split("\n")[0].split(",")[0]) == 1:
                jsonLora = stdout.decode().split("P")[len(stdout.decode().split("P")) - 1].split("\n")[0].split(",")  # lora payload contain a ping packet ?
                print(jsonLora)
                reachableNet[int(jsonLora[0]) - 1][0] = 1  # net is reachable
                reachableNet[int(jsonLora[0]) - 1][1] = int(jsonLora[1])  # save the RSSI
                loraReceived = "P"
            elif len(stdout.decode().split("TO")[len(stdout.decode().split("TO")) - 1].split("\n")[0].split(",")) == 2 and len(stdout.decode().split("TO")[len(stdout.decode().split("TO")) - 1].split("\n")[0].split(",")[0]) == 1:
                jsonLora = stdout.decode().split("TO")[len(stdout.decode().split("TO")) - 1].split("\n")[0].split(",")  # lora payload contain a timeout packet ?
                print(jsonLora)
                reachableNet[int(jsonLora[0]) - 1][0] = 1  # ZigBee Timeout
                reachableNet[int(jsonLora[0]) - 1][1] = int(jsonLora[1])  # save the RSSI
                loraReceived = "TO"
            else:
                print("-------------------------------------\n")

            for line in stdout.decode().split('\n'):
                if line and line[0] == "{":  # zigbee json received
                    jsonLora = json.loads(line)  # convertion into a dictionnary
                    print(f"json lora = {jsonLora}")
                    if "T" in jsonLora.keys():
                        dict_request = jsonLora
                        reachableNet[jsonLora['NETS']][0] = 1  # net is reachable
                        loraReceived = "T"
                        print(f"dict_request = {dict_request}")
                    elif "ACK" in jsonLora.keys():
                        dict_reqback = jsonLora
                        reachableNet[jsonLora['NETS']][0] = 1  # net is reachable
                        loraReceived = "A"
                        print(f"dict_reqback = {dict_reqback}")

        if loraReceived:  # flag when valid lora packet received
            print(f"Lora received, message type: {loraReceived}")
            if loraReceived == "D":  # received a discover packet
                print(reachableNet)
            elif loraReceived == "P":  # received a discover packet
                print(reachableNet)
            elif loraReceived == "TO":  # received a discover packet
                print("Timeout ZigBee")
            else:
                if loraReceived == "T":  # received a transmission packet
                    dump = json.dumps(dict_request).replace(" ","")
                elif loraReceived == "A":  # received an acknowledge packet
                    dump = json.dumps(dict_reqback).replace(" ","")
                print(f"Sending to Zolertia : {dump}")
                ser.write(bytes(dump + "\n", "utf-8"))
                timeTZigbee = time.time()
            loraReceived = ""
            print("-------------------------------------\n")

        try:
            zolertia_info = ser.readline().decode()
            if zolertia_info != "":
                print(f"zolertia info = {zolertia_info}")
                if zolertia_info[0] == "{":  # zigbee json received
                    elapsed = time.time() - timeTZigbee
                    print(f"Time of Zigbee transmission : {elapsed:.2}ms")
                    zolertiadicback = json.loads(zolertia_info)  # convertion into a dictionnary
                    if ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])] == True ) :
                        print("Publishing on the server ")
                    elif  ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])] == False ):
                        print("Sending to the lora module")
                        TimeTLora = time.time()

                        if "ACK" in zolertiadicback.keys():  # return message processing
                            if str(zolertiadicback["R"]) == "led_on" or zolertiadicback["R"] == 1:
                                proc.stdin.write(("A" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + str(zolertiadicback["ACK"]) + "1" + "\n").encode())
                                # ex : ../Lora/Transmit A NETD GTW SENSORID ACK R
                                # ex : ../Lora/Transmit A 1 1 1 1 1
                            elif str(zolertiadicback["R"]) == "led_off" or zolertiadicback["R"] == 0:
                                proc.stdin.write(("A" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + str(zolertiadicback["ACK"]) + "0" + "\n").encode())
                                # ex : ../Lora/Transmit A NETD GTW SENSORID ACK R
                                # ex : ../Lora/Transmit A 1 1 1 1 0
                            else:
                                proc.stdin.write(("A" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + "0" + "0" + "\n").encode())
                                # ex : ../Lora/Transmit A NETD GTW SENSORID ACK R
                                # ex : ../Lora/Transmit A 1 1 1 0 0
                        else:
                            proc.stdin.write(("T" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + str(zolertiadicback["T"]) + str(zolertiadicback["O"]) + "\n").encode())
                            # ex : ../Lora/Transmit T NETD GTW SENSORID T O
                            # ex : ../Lora/Transmit T 2 2 1 1 1

                        try:
                            print(f"Time of Lora : {(time.time() - TimeTLora):.2}ms")
                            stdout = proc.stdout.read()
                            # stderr = proc.stderr.read() # takes too much time
                            if stdout or stderr: print("API output:\n", stdout.decode(), stderr.decode())
                        except Exception as e:
                            print(e)

                else :  # debug messages
                    now = datetime.now()
                    now = now.replace(tzinfo = pytz.utc)
                    now = now.astimezone(pytz.timezone("Europe/Paris"))
                    current_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                    my_debug_message(current_time + " " + zolertia_info)

                print("-------------------------------------\n")

        except Exception as e:
            print(e)

        except KeyboardInterrupt:
            print("\nExiting....")
            proc.stdin.write(b"exit\n")
            # time.sleep(2)
            # stdout = proc.stdout.read()
            # stderr = proc.stderr.read()
            stdout, stderr = proc.communicate(timeout=15)
            print("API output:\n", stdout.decode(), stderr.decode())
            raise

except Exception as e:
    print(e)
