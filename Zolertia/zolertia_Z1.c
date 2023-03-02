/*
This code receives messages from the network. It also manages the IDs and IP adresses of each ZN.
It prints the data it receives from the network. The raspberry is then supposed to get these logs from the buffer.

authors : MAHRAZ Anass and Robyns Jonathan
*/

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "contiki-lib.h"
#include "dev/serial-line.h"
#include "dev/leds.h"
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define ENABLE_DEBUG_LOG 1 // 1 to enable the debug logs
/* Note that depending on the machine you use, enabling the debug logs can interfere with the communication
between the raspberry pi and the zolertia because the required baudrate of the serial port can change */

typedef struct {
// This structure defines a ZN equipment with its IP address and the IDs of its sensors
  int sensor_nb;  // number of sensors
  int *table_id; // table of the sensors IDs
  uip_ipaddr_t ip; // IP address
}zoul_info;

zoul_info ZN_network[10];   // Table that contains the equipments identified on the network
int identified_zolertia_count=0;    // Counter of the number of ZNs identified on the network

static struct simple_udp_connection udp_conn;
static uip_ipaddr_t ipaddr;
PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

static void my_udp_rx_callback(struct simple_udp_connection *c,
        const uip_ipaddr_t *sender_addr,
        uint16_t sender_port,
        const uip_ipaddr_t *receiver_addr,
        uint16_t receiver_port,
        const uint8_t *data,
        uint16_t datalen)
{
/*
c : our udp connection
sender_addr : sender IP address
sender_port : the port the sender uses to send the message
receiver_addr : our IP adress
receiver_port : the port the we use to get the message
data : the message
datalen : message length

This fonction is called when we get a message
*/
  static char buffer_OK[256];
  int is_know=0;    // 0 is the zolertia has not been identified yet, else 1
  int* ID_table = NULL;   // IDs of the sensors

  if (data[0]=='Z') {   // If the message begins with a 'Z', it's an identification message
    for (int h=0; h<identified_zolertia_count; h++) {
      if (uip_ip6addr_cmp(sender_addr, &ZN_network[h].ip)) {
        is_know=1;
      }
    }
    if (is_know==0) {
      ID_table = malloc(data[1] * sizeof(int));   // the second character of data is the number of sensors
      ZN_network[identified_zolertia_count].sensor_nb=data[1];
      for (int i=0; i<data[1]; i++) {
        ID_table[i]=data[i+2];    // The IDs of the sensors start at the 3rd character of data
      }
      ZN_network[identified_zolertia_count].table_id=ID_table;
      memcpy(&ZN_network[identified_zolertia_count].ip,sender_addr,sizeof(uip_ipaddr_t));
      LOG_INFO("Zn : ");
      LOG_INFO_6ADDR(&ZN_network[identified_zolertia_count].ip);
      LOG_INFO_(" detected.\n");
      identified_zolertia_count++;
    }

    for (int z=0; z<256; z++) {
      buffer_OK[z]='\0';   // Cleaning the buffer
    }

    snprintf(buffer_OK, sizeof(buffer_OK), "OK");
    #if ENABLE_DEBUG_LOG
    LOG_INFO("Sending '%s'\n", buffer_OK);
    #endif
    simple_udp_sendto(&udp_conn, buffer_OK, strlen(buffer_OK), sender_addr);
  }

  else {    // If the message is not an identification message
    #if ENABLE_DEBUG_LOG
    LOG_INFO("Received response '%.*s'\n", datalen, data);
    #endif
    printf("%s\n", (char *) data);   // transfer data to the raspberry
    #if ENABLE_DEBUG_LOG
    //printf("\n");
    #endif
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_server_process, ev, data) {
  /*
  This process reads messages from the serial port and sends messages on the network
  */
  PROCESS_BEGIN();
  serial_line_init();
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();    // This Zolertia is the root of our network
  /* Initialize UDP connection */
  static char buffer_message[256];
  int check_ID;
  const uint8_t *rasp_msg;
  int index;    // index for our strings "buffer-message" and "rasp_msg"
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, my_udp_rx_callback);

  while (1) {
    PROCESS_YIELD(); // we wait for a message from the serial port

    check_ID=0;   // Check if the ID of the sensor is registered in our network
    if (ev==serial_line_event_message && data != NULL) {
      rasp_msg=data;    // cast of "data" in a uint8_t table
      index=6; // index starts at 6 because there are 6 characters before the ID in the message
      while (index<20 && rasp_msg[index]!=',') {    // Reading the JSON message and getting the ID
        buffer_message[index-6]=rasp_msg[index];
        index++;
      }
      index=atoi((char *) buffer_message);   // Now the index variable describes the sensor ID (we re-use this variable for memory purpose)
      for (int z=0; z<index+5; z++) {
        buffer_message[z]='\0';   // Cleaning the buffer
      }

      for (int zn_index=0; zn_index<10;zn_index++) {
        for (int sensor_index=0; sensor_index<ZN_network[zn_index].sensor_nb; sensor_index++) {
          if (index==ZN_network[zn_index].table_id[sensor_index]) {
            ipaddr=ZN_network[zn_index].ip;
            check_ID=1;
          }
        }
      }

      if (check_ID==1) {
        #if ENABLE_DEBUG_LOG
        LOG_INFO("Sending ");
        printf("%s to : ",(char *) data);
        LOG_INFO_6ADDR(&ipaddr);
        LOG_INFO_("\n");
        #endif
        snprintf(buffer_message, sizeof(buffer_message), "%s",(char *) data);
        simple_udp_sendto(&udp_conn, buffer_message, strlen(buffer_message)+1,&ipaddr);
      }
    }
  }
  PROCESS_END();
}
