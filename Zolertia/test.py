import subprocess # for lora transmit C code
from threading import Thread # for lora receive thread

if False:
    proc = subprocess.Popen(["../Rasp/Transmit", "LED_ON", str(network["NET"]), str(network["ID"])], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)

    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout)

if True:
    proc = subprocess.Popen(["../Rasp/Receive"], shell=False, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    print(proc)
    
    stdout, stderr = proc.communicate(timeout=15)
    print("Output:\n", stdout)