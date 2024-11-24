#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include <stdio.h>

#define SERVER_PORT 1234
#define MAX_RETRIES 5
#define ACK_TIMEOUT CLOCK_SECOND

PROCESS(udp_client_process, "UDP Client with Retransmission");
AUTOSTART_PROCESSES(&udp_client_process);

static struct etimer retransmission_timer;
static int retries = 0;
static uint8_t ack_received = 0;

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct uip_udp_conn *conn;
  static char message[] = "Hello, Server!";

  PROCESS_BEGIN();

  // Create the UDP connection
  conn = udp_new(NULL, UIP_HTONS(SERVER_PORT), NULL);
  if (conn == NULL) {
    printf("Error: Could not create UDP connection.\n");
    PROCESS_EXIT();
  }

  // Set the server address (Assume server is a local node with IP address)
  uip_ipaddr_t server_ip;
  uip_ip6addr(&server_ip, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa); // Server's IPv6 address
  uip_udp_packet_sendto(conn, message, sizeof(message), &server_ip, UIP_HTONS(SERVER_PORT));

  etimer_set(&retransmission_timer, ACK_TIMEOUT);

  while (1) {
    PROCESS_WAIT_EVENT();

    // Check if we received an ACK
    if (ev == PROCESS_EVENT_TIMER) {
      if (!ack_received && retries < MAX_RETRIES) {
        retries++;
        printf("No ACK received, retrying... (Attempt %d/%d)\n", retries, MAX_RETRIES);
        uip_udp_packet_sendto(conn, message, sizeof(message), &server_ip, UIP_HTONS(SERVER_PORT));
        etimer_reset(&retransmission_timer); // Reset timer for next retransmission
      } else if (retries >= MAX_RETRIES) {
        printf("Max retries reached, stopping retransmission.\n");
        break;
      }
    }
  }

  PROCESS_END();
}

void udp_server_callback(struct uip_udp_conn *c, const uip_ipaddr_t *sender_addr,
                         uint16_t sender_port, const uint8_t *data, uint16_t len)
{
  printf("Received data from %d.%d.%d.%d: %s\n", sender_addr->u8[0], sender_addr->u8[1], sender_addr->u8[2], sender_addr->u8[3], data);
  // Acknowledge received data (by sending back a simple ACK message)
  uip_udp_packet_sendto(c, "ACK", 3, sender_addr, sender_port);
}
