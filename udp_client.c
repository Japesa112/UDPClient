#include "contiki.h"
#include "net/netstack.h"
#include "net/ip/uip.h"
#include "sys/etimer.h"
#include "net/udp/udp.h"
#include <stdio.h>
#include <stdlib.h>

#define UDP_PORT 1234
#define SERVER_ADDR 0xaaaa // replace with the server's IP address
#define MAX_RETRANSMISSIONS 3
#define TIMEOUT_INTERVAL (CLOCK_SECOND * 2)

static struct udp_conn *client_conn;
static int retransmissions;

PROCESS(udp_client_process, "UDP Client Process");
AUTOSTART_PROCESSES(&udp_client_process);

static void udp_sent_callback(struct udp_conn *c, const uip_ipaddr_t *to, uint16_t to_port) {
    printf("Message sent to %d.%d.%d.%d\n", to->u8[0], to->u8[1], to->u8[2], to->u8[3]);
}

static void udp_receive_ack(struct udp_conn *c, const uip_ipaddr_t *sender_addr,
                             uint16_t sender_port, uint8_t *data, uint16_t len) {
    if (strncmp(data, "ACK", 3) == 0) {
        printf("Acknowledgment received from %d.%d.%d.%d\n", sender_addr->u8[0], sender_addr->u8[1], sender_addr->u8[2], sender_addr->u8[3]);
        retransmissions = 0; // Reset retransmission counter on successful ACK
    }
}

static void send_message() {
    char message[] = "Hello, Server!";
    udp_packetbuf_clear();
    udp_packetbuf_copyfrom(message, sizeof(message));
    udp_sendto(client_conn, &SERVER_ADDR, UIP_HTONS(UDP_PORT));
}

PROCESS_THREAD(udp_client_process, ev, data) {
    static struct etimer et;
    static uip_ipaddr_t server_ip;

    PROCESS_BEGIN();

    // Set up UDP connection
    uip_ip6addr(&server_ip, SERVER_ADDR, 0, 0, 0, 0, 0, 0, 0);  // replace with actual server IP

    client_conn = udp_new(NULL, UIP_HTONS(UDP_PORT), udp_receive_ack);
    udp_bind(client_conn, UIP_HTONS(UDP_PORT));

    printf("UDP client started\n");

    // Retransmission logic
    while(1) {
        etimer_set(&et, TIMEOUT_INTERVAL);  // Wait for a response or timeout
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        send_message();  // Send message to server

        // Wait for acknowledgment
        etimer_reset(&et);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        if (retransmissions < MAX_RETRANSMISSIONS) {
            printf("No ACK received, retransmitting...\n");
            retransmissions++;
            send_message();  // Retransmit if no acknowledgment was received
        } else {
            printf("Max retransmissions reached, giving up.\n");
            retransmissions = 0;  // Reset counter for the next message
        }
    }

    PROCESS_END();
}
