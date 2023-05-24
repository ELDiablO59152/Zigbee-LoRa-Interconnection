/*
This code sends 2 types of messages to Z1 :
  - Identification messages
  - Data from the rapsberry

authors : MAHRAZ Anass and ROBYNS Jonathan
 */

#include "contiki.h"
#include "dev/serial-line.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include <stdio.h>
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define ENABLE_DEBUG_LOG 1 // 1 to enable the debug logs
/* Note that depending on the machine you use, enabling the debug logs can interfere with the communication 
between the raspberry pi and the zolertia because the required baudrate of the serial port can change */

static struct simple_udp_connection udp_conn;

int check=0;    // If check=0, we are in identification mode
struct ctimer identification_timer;    // If this timer ends, we set check=0
/*---------------------------------------------------------------------------*/
PROCESS(identification, "Identification");
PROCESS(serial_process, "Serial process");
AUTOSTART_PROCESSES(&identification,&serial_process);
/*---------------------------------------------------------------------------*/
static void timeout(void *ptr) {
  check=0;
}

/*---------------------------------------------------------------------------*/

static void
my_udp_rx_callback(struct simple_udp_connection *c,
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
  ctimer_set(&identification_timer, 85*CLOCK_SECOND, timeout, NULL);   // If we get a message from Z1, we reset the identification timer
  #if ENABLE_DEBUG_LOG
  LOG_INFO("Received '%.*s'\n", datalen, (char *) data);
  LOG_INFO("check = %d\n", check);
  #endif
  if (check==1){
    printf("\n%s\n", (char *) data);    // transfer data to the raspberry
  }

  if (data[0]=='O' && data[1]=='K') {
    check=1;    // Exit the identification mode
  }

#if ENABLE_DEBUG_LOG
  #if LLSEC802154_CONF_ENABLED
    LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
  #endif
    //LOG_INFO_("\n");
#endif
}

/*---------------------------------------------------------------------------*/

static int size_message;
static void * my_create_message(int table_size,int *table){
/*
table_size : size of the table
table : table

This funtion returns a message which is the table with "Z" + "table size" at the beginning
*/                                          
  unsigned char * message;
  size_message=table_size*sizeof(int)+2;
  message=malloc(size_message);                                                        
  message[0]='Z';
  message[1]=(unsigned char) table_size;
  for (int i=0; i<table_size;i++) {
    message[i+2]=table[i];
 }
  return ((void*) message);
}

/*---------------------------------------------------------------------------*/ 

PROCESS_THREAD(identification, ev, data)
{
/*
This process sends identification messages to the root (Z1)
*/
  uip_ipaddr_t dest_ipaddr;  // root IP address
  static struct etimer d_t;
  int tab[5]={1,24,102,144,182};  // sensors ID (mock data)

  void *message_ids;

  PROCESS_BEGIN();
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, my_udp_rx_callback);

  while(1) {
    while(check==0) {
      if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr) && check==0) {
        /* Send to DAG root (Z1) */
        message_ids=my_create_message(5,tab);
        #if ENABLE_DEBUG_LOG
        LOG_INFO("Sending ID message to ");
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");
        #endif
        simple_udp_sendto(&udp_conn, message_ids, size_message, &dest_ipaddr);     // Fonction that sends the message
      } else {
        #if ENABLE_DEBUG_LOG
        LOG_INFO("Not reachable yet\n");
        #endif
      }
      etimer_set(&d_t, 10*CLOCK_SECOND);    // 10 seconds timer between each ID message
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&d_t));
    }
    PROCESS_PAUSE();
 }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(serial_process, ev, data)
{
/*
This process sends the data from the sensors to the root (Z1)
*/
  static char message_to_Z1[256];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD();
    if (ev==serial_line_event_message && data != NULL) {    // If the event is from the serial port, we send the data to the root
        if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
          /* Send to DAG root */
          #if ENABLE_DEBUG_LOG
          LOG_INFO("Sending ");
          printf("%s to : ",(char *) data);
          LOG_INFO_6ADDR(&dest_ipaddr);
          LOG_INFO_("\n");
          #endif
          snprintf(message_to_Z1, sizeof(message_to_Z1), "%s", (char *) data);
          simple_udp_sendto(&udp_conn, message_to_Z1, strlen(message_to_Z1)+1 , &dest_ipaddr);
        } else {
          #if ENABLE_DEBUG_LOG
          LOG_INFO("Not reachable yet\n");
          #endif
        }
    }
  }

  PROCESS_END();
}
