from socket import *
import subprocess
from threading import Thread
#import random
#import sys
#from time import sleep


class ClientThread(Thread):
    def __init__(self, conn, address, threads_list):
        Thread.__init__(self)
        self.conn = conn
        self.address = address
        self.threads_list = threads_list

    def run(self):
        if __name__ == '__main__':
            try:
                while data != "exit" and data != "kill":
                    self.conn.send(" Enter a command : ".encode('utf-8'))
                    data = self.conn.recv(1024).decode('utf-8').strip(' \n')
                    print(self.address, data)

                    if data == "pouet pouet":
                        for threads in self.threads_list:
                            threads.conn.send(" Other user send you : 'pouet pouet'")

                    if data == "ping":
                        self.conn.send(" pong\n Disconnection\n".encode('utf-8'))
                        print("pong")

                    elif data == "cmd":
                        print("cmd spawned")
                        self.conn.send(" cmd opened\n".encode('utf-8'))

                        while data != "exit":
                            self.conn.send("C:\\Users\\fabie\\Desktop>".encode('utf-8'))
                            command = self.conn.recv(1024).decode('utf-8')
                            data = command.strip('\n').split()
                            print(self.address, data)

                            if data[0] == "ping" or data[0] == "dir" or data[0] == "cd" or data[0] == "echo" or command.strip(3)== "./" or command.strip(3) == "../":
                                # cmd = [data]
                                # cmd = ['C:\Windows\System32\cmd.exe', '-c', data]

                                proc = subprocess.Popen(data, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                                print(proc)

                                stdout, stderr = proc.communicate(timeout=15)
                                print("Output:\n", stdout)

                                self.conn.sendall(stdout)
                                proc.kill()

                            elif data[0] == "exit" or data[0] == "kill":
                                print(data)
                                data = data[0]
                                print(data)

                            else:
                                self.conn.send(" Not permitted !\n".encode('utf-8'))

                    elif data != "exit" and data != "kill":
                        self.conn.send(" Bad command !!!\n".encode('utf-8'))
                        print(data)

                    if data == "exit" or data == "kill":
                        self.conn.send(" Disconnection\n".encode('utf-8'))
                        self.conn.close()
                        print(str(self.address) + " disconnected!")
                        if data == "kill": return

            except Exception as e:
                print(e)
                self.conn.send(" Disconnection\n".encode('utf-8'))
                self.conn.close()
                print(str(self.address) + " disconnected!")
                return


def socketTest():
    print("Hello")

    sock = socket(AF_INET, SOCK_STREAM)
    sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    #sock.bind(("192.168.1.73", 11000))
    #sock.bind(("10.222.7.126", 11000))
    sock.bind(("192.168.1.10", 11000))
    sock.listen(1024)

    threads_list = []
    
    while True:
        print('Je suis serveur')
        (conn, address) = sock.accept()
        print(str(address) + " connected!")
        # sock.settimeout(2)

        threads_list.append(ClientThread(conn, address, threads_list))
        threadsList[-1].start()

        for thread in threads_list:
            thread.join()
            print("finished")

        print("Hell yeah")


socketTest()
