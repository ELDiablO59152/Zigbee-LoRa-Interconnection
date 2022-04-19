import subprocess # for lora transmit C code
from threading import Thread # for lora receive thread
from time import sleep

if False:
    print("Initializing LoRa module")
    proc = subprocess.Popen(["../Rasp/Init"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # Ordonne au raspberry d'allumer la LED

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Transmitting a LED_ON order")
    proc = subprocess.Popen(["../Rasp/Transmit", "LED_ON"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # Ordonne au raspberry d'allumer la LED

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Transmitting a LED_OFF order")
    proc = subprocess.Popen(["../Rasp/Transmit", "LED_OFF"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # Ordonne au raspberry d'éteindre la LED

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Waiting for a LoRa packet")
    proc = subprocess.Popen(["../Rasp/Receive", "15"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # Démarre la réception LoRa

    stdout, stderr = proc.communicate(timeout=60)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))

sleep(5)

if True:
    print("Waiting for a LoRa packet")
    proc = subprocess.Popen(["../Rasp/Receive", "15"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc) # Démarre la réception LoRa

    stdout, stderr = proc.communicate(timeout=60)
    print("Output:\n", stdout.decode('utf-8'), stderr.decode('utf-8'))