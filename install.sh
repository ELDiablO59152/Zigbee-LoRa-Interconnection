#/bin/bash

echo "Install Dependencies ?(y/n)"
read choice
if [ "$choice" == "y" ]; then
    grep completion-ignore-case ~/.bashrc > /dev/null
    if [ $? != 0 ]; then echo "bind 'set completion-ignore-case on'" >> ~/.bashrc; fi
    sudo apt update; sudo apt full-upgrade -y; sudo apt autoremove --purge -y; sudo apt clean
    rustc --version
    if [ $? != 0 ]; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    fi
    sudo apt install -y libgirepository1.0-dev
    sudo apt install -y libdbus-1-dev
    sudo apt install -y libglib2.0-dev
    sudo apt install -y libcairo2-dev
    sudo apt install -y libssl-dev
    sudo apt install -y libopenblas-dev
    sudo apt install -y pkg-config
    sudo apt install -y python3-pip
    pip3 install -U pip
    # pip3 list -o | cut -f1 -d' ' | tr " " "\n" | awk '{if(NR>=3)print}' | cut -d' ' -f1 | xargs -n1 pip3 install -U
fi

echo "Install Contiki ?(y/n)"
read choice
if [ "$choice" == "y" ]; then
    git clone https://github.com/contiki-ng/contiki-ng.git
    cd contiki-ng
    git submodule update --init
    sudo apt install -y gcc-arm-none-eabi
    sudo apt install -y libatlas-base-dev
    sudo apt install -y gfortran
    sudo pip3 install pyserial
    sudo pip3 install intelhex
    sudo pip3 install python-magic
    sudo pip3 install grovepi
    sudo pip3 install pytz
    sudo chmod 666 /dev/ttyUSB0
fi

echo "Install LoRa ?(y/n)"
read choice
if [ "$choice" == "y" ]; then
    cd ~
    git clone https://github.com/ELDiablO59152/Zigbee-LoRa-Interconnection.git
    cd ~/Zigbee-LoRa-Interconnection/Lora
    make
fi

echo "Flash ZigBee ?(y/n)"
read choice
if [ "$choice" == "y" ]; then
    echo "Mother Node ?(y/n)"
    read choice
    if [ "$choice" == "y" ]; then
        cp ~/Zigbee-LoRa-Interconnection/Zolertia/zolertia_Z1.c ~/contiki-ng/examples/rpl-udp/
        cd ~/contiki-ng/examples/rpl-udp/
        sudo make BOARD=firefly TARGET=zoul zolertia_Z1.upload login
    else
        cp ~/Zigbee-LoRa-Interconnection/Zolertia/zolertia_ZN.c ~/contiki-ng/examples/rpl-udp/
        cd ~/contiki-ng/examples/rpl-udp/
        sudo make BOARD=firefly TARGET=zoul zolertia_ZN.upload login
    fi
fi

cd ~/Zigbee-LoRa-Interconnection/Rasp/
python raspb_ZN.py

# ../../tools/cc2538-bsl/cc2538-bsl.py -e -w -v -a 0x00202000 \
# ../../tools/serial-io/serialdump -b115200 /dev/ttyUSB0
# {"ID":1,"T":1,"O":1,"NETD":2,"NETS":1}
# {"ID":1,"ACK":1,"R":0,"NETD":1,"NETS":2}