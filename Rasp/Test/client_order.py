"""
This program allow the user to send an order in the zolertia network and ask them data about their sensors

Author : MAHRAZ Anass
"""
import paho.mqtt.client as mqtt
import json

mqttBroker ="test.mosquitto.org" # internet addresse of the broker

order=["null","null","null","null"] # list used to build an order 
#{"ID":order[0],"T":order[1],"O":order[2],"NET":order[3]}
#ex:{"ID":24,"T":1,"O":1,"NET":1}

# connection to the broker
client = mqtt.Client()
client.connect("test.mosquitto.org", 1883, 60)
client.loop_start()

while True: # ask for action to do 
        print("Welcome to the EBalanceplus demo.\n")
        order[0] = int(input("Choose the ID of the equipment you want.\n"))
        order[3]= int(input("WHAT  NETWORK YOU WANT TO REACH \n"))
        if order[0] in [1,24,182]:
            order[1]= int(input("You have chosen a Lamp.\nTo know if it is on or off, type 0.\nTo change its state, type 1.\n"))
            if order[1] == 1:
                order[2]= int(input("Type 1 to turn it on, 0 tu turn it off.\n"))
        
        """elif order[0] in [102,144]:
            order[1]= int(input("You have chosen a fan.\nTo know its speed, type 0.\nTo change it, type 1.\n"))
            if order[1] == 1:
                order[2]= int(input("Type the new speed you want.\n"))"""
    
        order_str='{"ID":'+str(order[0])+',"T":'+str(order[1])+',"O":'+str(order[2])+',"NET":'+str(order[3])+'}' # build string order from the list order
        #print(order) # debug
        #print(order_str) # debug
        #print(type(order)) # debug
        client.publish("/EBalanceplus/order",order_str) # send the order to the broker
