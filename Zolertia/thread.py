import random
import sys
from threading import Thread, RLock
from time import sleep
#import queue


class Afficheur(Thread):
    def __init__(self, mot):
        Thread.__init__(self)
        self.mot = mot

    def run(self):
        i = 0
        while i < 5:
            if __name__ == '__main__':
                with verrou:
                    for lettre in self.mot:
                    #for j in range(6):
                        sys.stdout.write(lettre)
                        #sys.stdout.write(self.mot.get())
                        sys.stdout.flush()
                        attente = 0
                        attente += random.randint(1, 60) / 100
                        sleep(attente)
                i += 1
                sleep(0.001)


#q = queue.Queue()
verrou = RLock()

#q.put("canardTORTUEcanardTORTUEcanardTORTUEcanardTORTUEcanardTORTUE")
thread_1 = Afficheur("canard")
thread_2 = Afficheur("TORTUE")

thread_1.start()
thread_2.start()

thread_1.join()
thread_2.join()
