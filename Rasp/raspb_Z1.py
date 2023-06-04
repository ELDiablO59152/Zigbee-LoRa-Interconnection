"""
This script gets the orders from the client and sends it to the zolertia C script.
It also gets messages from the network.

authors : MAHRAZ Anasset, ROBYNS Jonathan and AMELINCK Fabien
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
dict_request = {"ID": None, "T": None, "O": None, "NETD": None, "NETS": None, "GTW": None}
dict_reqback = {"ID": None, "ACK": None, "R": None, "NETD": None, "NETS": None, "GTW": None}
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


def get_lora_gtw(NETD):
    with open("./routingTable/routingTable.json", "r") as routingFile:
        jsonFile = json.load(routingFile)
        for node in jsonFile["routingTable"]:
            if node["id"] == str(NETD) and node["gtw"] != "":  # if we found the id in the routing table we put the gtw from it
                print(f"gateway {node['gtw']} found for {NETD}")
                return node["gtw"]
    return NETD  # else return NETD


def node_is_up(NETD, GTW, RSSI):
    if str(GTW) == str(myNet):  # if no gateway usefull
        GTW = NETD
    print(f"node_is_up {NETD} {GTW} {RSSI}")
    with open("./routingTable/routingTable.json", "r") as routingFile:
        jsonFile = json.load(routingFile)

    with open("./routingTable/routingTable.json", "w") as routingFile:
        modified = False
        present = False
        for node in jsonFile["routingTable"]:
            if node["id"] == str(NETD):  # if we found the id in the routing table we put the gtw from it
                present = True
                node["gtw"] = str(GTW)
                node["status"] = "up"
                node["rangePower"] = str(RSSI)
                modified = True

        if not present:
            jsonFile["routingTable"].append({
                "id": str(NETD),
                "gtw": str(GTW),
                "pKey": "ABDD479DE546DBEE9B79698248",
                "comment": "",
                "status": "up",
                "rangePower": str(RSSI)
            })
            modified = True

        if modified:
            json.dump(jsonFile, routingFile, indent = 4)


myNet = str(input('Enter the network number: '))
for key in NETWORK.keys():
    if key == myNet:
        NETWORK[key] = True


print("Init LoRa Module")
try:
    proc = subprocess.Popen(["../Lora/Init"], shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate(timeout=10)
    print("Output :\n", stdout.decode(), stderr.decode())
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
        if stdout or stderr: print("API output :\n", stdout.decode(), stderr.decode())

        if stdout:  # interpretation of the LoRa program output
            for line in stdout.decode().split('\n'):
                if line and line[0] == "{":  # zigbee json received
                    jsonLora = json.loads(line)  # convertion into a dictionnary
                    print(f"json lora = {jsonLora}")
                    if "DISCO" in jsonLora.keys():  # lora payload contain a discover packet ?
                        reachableNet[int(jsonLora['NETS']) - 1][0] = 1  # net is reachable
                        reachableNet[int(jsonLora['NETS']) - 1][1] = int(jsonLora['RSSI'])  # save the RSSI
                        node_is_up(jsonLora['NETS'], jsonLora['GTW'], jsonLora['RSSI'])
                        loraReceived = "D"
                    elif "PING" in jsonLora.keys():  # lora payload contain a ping packet ?
                        reachableNet[int(jsonLora['NETS']) - 1][0] = 1  # net is reachable
                        reachableNet[int(jsonLora['NETS']) - 1][1] = int(jsonLora['RSSI'])  # save the RSSI
                        node_is_up(jsonLora['NETS'], jsonLora['GTW'], jsonLora['RSSI'])
                        loraReceived = "P"
                    elif "TIMEOUT" in jsonLora.keys():  # lora payload contain a timeout packet ?
                        reachableNet[int(jsonLora['NETS']) - 1][0] = 1  # net is reachable
                        reachableNet[int(jsonLora['NETS']) - 1][1] = int(jsonLora['RSSI'])  # save the RSSI
                        node_is_up(jsonLora['NETS'], jsonLora['GTW'], jsonLora['RSSI'])
                        loraReceived = "TO"
                    elif "T" in jsonLora.keys():  # lora payload contain a transmission packet ?
                        dict_request = jsonLora
                        reachableNet[int(jsonLora['NETS']) - 1][0] = 1  # net is reachable
                        node_is_up(jsonLora['NETS'], jsonLora['GTW'], "")
                        loraReceived = "T"
                        print(f"dict_request = {dict_request}")
                    elif "ACK" in jsonLora.keys():  # lora payload contain an acknowledge packet ?
                        dict_reqback = jsonLora
                        reachableNet[int(jsonLora['NETS']) - 1][0] = 1  # net is reachable
                        node_is_up(jsonLora['NETS'], jsonLora['GTW'], "")
                        loraReceived = "A"
                        print(f"dict_reqback = {dict_reqback}")
                    else:
                        print("-------------------------------------\n")

        if loraReceived:  # flag when valid lora packet received
            print(f"Lora received, message type : {loraReceived}")
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
                print(f"Sending to Zolertia : {dump}\n")
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
                    if ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])] == True ):
                        print("Publishing on the server ")
                    elif  ( (str(zolertiadicback["NETD"]) in NETWORK ) and NETWORK[str(zolertiadicback["NETD"])] == False ):
                        print("Sending to the lora module")
                        TimeTLora = time.time()
                        zolertiadicback["GTW"] = get_lora_gtw(str(zolertiadicback["NETD"]))

                        if "DISCO" in zolertiadicback.keys():  # zigbee payload contain a discover packet ?
                            proc.stdin.write(("D" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + "\n").encode())
                            # ex : ../Lora/Transmit D NETD GTW
                            # ex : ../Lora/Transmit D 2 2
                        elif "PING" in zolertiadicback.keys():  # zigbee payload contain a ping packet ?
                            proc.stdin.write(("P" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + "\n").encode())
                            # ex : ../Lora/Transmit P NETD GTW
                            # ex : ../Lora/Transmit P 2 2
                        elif "TIMEOUT" in zolertiadicback.keys():  # zigbee payload contain a timeout packet ?
                            proc.stdin.write(("TO" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + "\n").encode())
                            # ex : ../Lora/Transmit TO NETD GTW SENSORID
                            # ex : ../Lora/Transmit TO 2 2 1
                        elif "T" in zolertiadicback.keys():  # zigbee payload contain a transmit packet ?
                            proc.stdin.write(("T" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + str(zolertiadicback["T"]) + str(zolertiadicback["O"]) + "\n").encode())
                            # ex : ../Lora/Transmit T NETD GTW SENSORID T O
                            # ex : ../Lora/Transmit T 2 2 1 1 1
                        if "ACK" in zolertiadicback.keys():  # zigbee payload contain an acknowledge packet ?
                            proc.stdin.write(("A" + str(zolertiadicback["NETD"]) + str(zolertiadicback["GTW"]) + str(zolertiadicback["ID"]) + str(zolertiadicback["ACK"]) + str(zolertiadicback["R"]) + "\n").encode())
                            # ex : ../Lora/Transmit A NETD GTW SENSORID ACK R
                            # ex : ../Lora/Transmit A 1 1 1 1 1

                        try:
                            stdout = proc.stdout.read()
                            # stderr = proc.stderr.read() # takes too much time
                            if stdout or stderr: print("API output :\n", stdout.decode(), stderr.decode())
                            print(f"Time of Lora : {(time.time() - TimeTLora):.2}ms")
                        except Exception as e:
                            print(e)

                else:  # debug messages
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
            print("API output :\n", stdout.decode(), stderr.decode())
            raise

except Exception as e:
    print(e)
