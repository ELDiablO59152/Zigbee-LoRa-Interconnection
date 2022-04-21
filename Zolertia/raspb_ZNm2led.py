"""
This scripts simulates sensor reading and actions on equipments of our network. It also writes the messages we send back to Z1.
The boolean "simul" must be set to False if you are manipulating real equipment with grovepi. Otherwise, set it to True, and the leds/fans will only be represented by variables.

authors : Mahraz Anass and Robyns Jonathan
"""

simul = False # boolean set tro true if we use mock data, false if we use real leds

import json
import time
import serial
if not simul:
        from grovepi import *
from datetime import datetime
import pytz
import random
import RPi.GPIO as GPIO

ser = serial.Serial(
        port='/dev/ttyUSB0',
        baudrate = 115200,
)

leds = [0,0,0] # mock data. Represents the three leds (id 1, 24 and 182). 1 for on and 0 for off
fans = [0,0] # mock data. Represents the theree fans's speeds (id 102 and 144)

order_back = {"ID": None, "ACK": None, "R": None ,"NET":None } # The message we send back to Z1

def my_debug_message(msg):
        """
        msg (string) : debug message to write in infor_debug.txt

        This fonction has no return value. It simply writes msg in "info_debug.txt"
        """
        with open("info_debug.txt",'a') as debug_file:
                debug_file.write(msg)
                debug_file.write("\n")

def my_read_sensor(id,NET):
        """
        id (int) : ID of the sensor we read

        This function has no return value. It simulates the reading of the sensor that has the id given in paramater.
        """
        global order_back
        global leds
        global fans
        order_back["ACK"]=None
        order_back["NET"]=NET
        id = int(id)
        GPIO.setmode(GPIO.BOARD)
        GPIO.setwarnings(False)
        LED = 7 #Définit le numéro du port GPIO qui alimente la led

        GPIO.setup(LED, GPIO.OUT) #Active le contrôle du GPIO
        state=GPIO.input(LED)
        print("Reading sensor %d"%(id))
        if id ==1:
                order_back["ID"] = id
                order_back["R"]=state
                order_back_json = json.dumps(order_back) # convert the message in JSON before sending it
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8')) # the response is sent in the serial port
        elif id==24:
                order_back["R"]=state
                order_back["ID"] = id
                order_back_json = json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id==102:
                order_back["ID"] = id
                order_back["R"]= fans[0]
                order_back_json = json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id==144:
                order_back["ID"] = id
                order_back["R"]= fans[1]
                order_back_json = json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id==182:
                order_back["ID"] = id
                order_back["R"]= leds[2]
                order_back_json = json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        else :
                pass

def setLeds(i,id,val):
        """
        i (int) : index of the led in the leds list
        id (int) : id of the led
        val (int) : 1 to turn the led on, 0 to turn it off

        returns the message to put in order_back["ACK"]
        """

        GPIO.setmode(GPIO.BOARD) #Définit le mode de numérotation (Board)
        GPIO.setwarnings(False) #On désactive les messages d'alerte

        LED = 7 #Définit le numéro du port GPIO qui alimente la led

        GPIO.setup(LED, GPIO.OUT) #Active le contrôle du GPIO
        
        state = GPIO.input(LED) #Lit l'état actuel du GPIO, vrai si allumé, faux si éteint
        a=state
        if state != val and val==0 : #Si l'état actuel de la LED est différent de l'état voulu et que l'ordre est d'éteindre
             GPIO.output(LED, GPIO.LOW) #On l’éteint
             a="led_off"
        elif state != val and val==1 : #Si l'état actuel de la LED est différent de l'état voulu et que l'ordre est d'allumer
             GPIO.output(LED, GPIO.HIGH) #On l'allume
             a="led_on"
        return(a)
def setFans(i,id,val):
        """
        i (int) : index of the fan in the leds list
        id (int) : id of the fan
        val (int) : speed wanted for the fan

        returns the message to put in order_back["ACK"]
        """
        global fans
        fans[i]=val
        return "Fan "+str(id)+" set to speed "+str(val)+"."

def my_action_device(id, val,NET):
        """
        id (int) : ID of the device we want to give an order to
        val (int) : value defining the order. It can have several meanings depending on the nature of the device

        THis function has no return value. It is supposed to give an order to a device, for example, a led lamp.

        """
        global order_back
        global leds
        global fans
        order_back["NET"]=NET
        order_back["R"]= None
        print("Action on the sensor %s"%(id))
        if id==1:
                order_back["ID"] = id
                order_back["R"] = setLeds(0,id,val)
                if order_back["R"] !=None:   # Ici on crée un accusé de réception si l'ordre a bien été reçu et effectué
                        order_back["ACK"]=1
                order_back_json= json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id == 24:
                order_back["ID"] = id
                order_back["R"] = setLeds(1,id,val)
                order_back_json= json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id == 102:
                order_back["ID"] = id
                order_back["ACK"] = setFans(0,id,val)
                order_back_json= json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id == 144:
                order_back["ID"] = id
                order_back["ACK"] = setFans(1,id,val)
                order_back_json= json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))
        elif id == 182:
                order_back["ID"] = id
                order_back["R"] = setLeds(2,id,val)
                order_back_json= json.dumps(order_back)
                print("order_back_json = "+order_back_json+"\n")
                ser.write(bytes(order_back_json+"\n",'utf-8'))

while True:
        zolertia_info=str(ser.readline().decode("utf-8"))
        print("zolertia info = "+zolertia_info+"\n")
        zolertia_info2=zolertia_info[29:57] # on récupère ici la partie qui nous intéresse dans le zolertia_info c'est-à-dire les informations contenues dans le JSON
        if len(zolertia_info2) != 0 and zolertia_info2[0] == "{": # on vérifie que l'info reçue est bien un JSON
                zolertia_info_dic=json.loads(zolertia_info2) # convertion into a dictionnary
                if "T" in zolertia_info_dic: # T indicates if we must read or write on our device. NB: the ACKs have no "T" value
                        if zolertia_info_dic["T"] ==  0 :
                                my_read_sensor(zolertia_info_dic["ID"],zolertia_info_dic["NET"])
                        elif zolertia_info_dic["T"] == 1 :
                                my_action_device(zolertia_info_dic["ID"], zolertia_info_dic["O"],zolertia_info_dic["NET"])
                elif "ACK" in zolertia_info_dic :
                        pass
                else :
                        print("NO EXISTING SENSOR MATCHING THIS ID\n")

        else: # debug messages
                now=datetime.utcnow()
                now=now.replace(tzinfo=pytz.utc)
                now=now.astimezone(pytz.timezone("Europe/Paris"))
                current_time = now.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                my_debug_message(current_time+zolertia_info)
