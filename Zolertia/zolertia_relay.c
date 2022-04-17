/* 
This scrpit only relays information on the network, but doesn't process anything

author : DUNKELS Adam
*/

#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"

ENABLE_DEBUG_LOG=1

#if !NETSTACK_CONF_WITH_IPV6 || !UIP_CONF_ROUTER || !UIP_IPV6_MULTICAST || !UIP_CONF_IPV6_RPL
#error "This example can not work with the current contiki configuration"
#error "Check the values of: NETSTACK_CONF_WITH_IPV6, UIP_CONF_ROUTER, UIP_CONF_IPV6_RPL"
#endif
/*---------------------------------------------------------------------------*/
PROCESS(mcast_intermediate_process, "Intermediate Process");
AUTOSTART_PROCESSES(&mcast_intermediate_process);
/*---------------------------------------------------------------------------*/

#if ENABLE_DEBUG_LOG
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
  printf("%s\n", (char *) data); // transfer data to the raspberry
}
#endif

PROCESS_THREAD(mcast_intermediate_process, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_BEGIN();
  
  #if ENABLE_DEBUG_LOG
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, my_udp_rx_callback);
  #endif

  PROCESS_END();
}
