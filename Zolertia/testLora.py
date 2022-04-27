import subprocess  # for lora transmit C code
from threading import Thread  # for lora receive thread
from time import sleep

myNet = str(input('Enter the sender network number : '))
destNet = str(input('Enter the target network number : '))

if True:
    print("Initializing LoRa module")
    proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)  # initializing lora module

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

#sleep(5)

if False:
    print("Transmitting a LED_ON order")
    proc = subprocess.Popen(["../Rasp/Transmit", "T", destNet, myNet, "1", "1", "1"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # turn on the led

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Waiting for a LoRa packet")
    proc = subprocess.Popen(["../Rasp/Receive", myNet, "20"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)  # Waiting for zigbee ack

    stdout, stderr = proc.communicate(timeout=60)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
    if len(stdout.decode('utf-8').split('A')) > 1:
        print(stdout.decode('utf_8').split('A')[1].strip('\n').split(','))

sleep(5)

if False:
    print("Transmitting a LED_OFF order")
    proc = subprocess.Popen(["../Rasp/Transmit", "T", destNet, myNet, "1", "1", "0"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)  # shut off the led

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Waiting for a LoRa packet")
    proc = subprocess.Popen(["../Rasp/Receive", 1, "20"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)  # waiting for zigbee ack

    stdout, stderr = proc.communicate(timeout=60)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))
    if len(stdout.decode('utf-8').split('A')) > 1:
        print(stdout.decode('utf_8').split('A')[1].strip('\n').split(','))
