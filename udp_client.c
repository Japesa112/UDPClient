#include "contiki.h"
#include "net/uip.h"
#include "net/simple-udp.h"
#include <stdio.h>

#define UDP_CLIENT_PORT 5678
#define UDP_SERVER_PORT 1234
#define SEND_INTERVAL (CLOCK_SECOND)
#define MAX_RETRIES 3

static struct simple_udp_connection udp_conn;
static struct etimer timer;
static uint32_t tx_count = 0;
static uint32_t retries = 0;

PROCESS(udp_client_process, "UDP Client");
AUTOSTART_PROCESSES(&udp_client_process);

static void udp_rx_callback(struct simple_udp_connection *c,
                             const uip_ipaddr_t *sender_addr,
                             uint16_t sender_port,
                             const uip_ipaddr_t *receiver_addr,
                             uint16_t receiver_port,
                             const uint8_t *data,
                             uint16_t datalen) {
  printf("Received response: '%.*s'\n", datalen, (char *)data);
  retries = 0; // Reset retries on successful reception
}

PROCESS_THREAD(udp_client_process, ev, data) {
  uip_ipaddr_t dest_ipaddr;
  
  PROCESS_BEGIN();
  
  // Register the UDP connection
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  
  etimer_set(&timer, SEND_INTERVAL);
  
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    
    if(NETSTACK_ROUTING.node_is_reachable() && 
       NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      char msg[20];
      snprintf(msg, sizeof(msg), "Hello %u", tx_count);
      printf("Sending request: '%s'\n", msg);
      
      // Send message to server
      simple_udp_sendto(&udp_conn, msg, strlen(msg), &dest_ipaddr);
      tx_count++;
      
      // Start retry mechanism
      retries = MAX_RETRIES;
      etimer_set(&timer, SEND_INTERVAL); // Reset timer for next send
    }
    
    // Check for retries if no acknowledgment received
    if(retries > 0) {
      etimer_set(&timer, SEND_INTERVAL); // Wait before retrying
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      printf("Retrying... (%u)\n", MAX_RETRIES - retries + 1);
      simple_udp_sendto(&udp_conn, msg, strlen(msg), &dest_ipaddr);
      retries--;
    }
  }
  
  PROCESS_END();
}
