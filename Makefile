# Path to Contiki (update if necessary)
CONTIKI = ../contiki2.7

# Specify the target platform (e.g., sky for Tmote Sky motes)
TARGET = sky

# Define the program name based on the C file
PROJECT = udp-client

# Include the Contiki build system
include $(CONTIKI)/Makefile.include
