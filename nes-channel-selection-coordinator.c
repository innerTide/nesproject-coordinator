/*
 * 
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Coordinator Source Code for NES project
 * \author
 *         Joakim Eriksson, joakime@sics.se
 *          Adam Dunkels, adam@sics.se
 * 			Yuefeng Wu, y.wu.5@student.tue.nl
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/etimer.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "cc2420.h"
#include "cc2420_const.h"
#include <stdlib.h>
#include "dev/spi.h"
#include <stdio.h>
#include <string.h>


/*---------------------------------------------------------------------------*/
/* This is a set of predefined values for this laboratory*/

#define RSSI_THRESHOLD 20
#define DEBUG 1

/*---------------------------------------------------------------------------*/
/* This is a set of global variables for this laboratory*/

int currentChannel = 26;
unsigned int selectionCounter = 0;
char instructionBuff [6];

/*---------------------------------------------------------------------------*/
/* This assumes that the CC2420 is always on and "stable" */

static void instruction_generator (int preferredChannel, int selectionCounter){
	instructionBuff[0]='C';
	instructionBuff[1]=preferredChannel/10 + 48;
	instructionBuff[2]=preferredChannel%10 + 48;
	instructionBuff[3]=selectionCounter/10 + 48;
	instructionBuff[4]=selectionCounter%10 + 48;
	instructionBuff[5]='\0';
}


/*---------------------------------------------------------------------------*/
/* Reversed linear scan*/
static int find_channel_algorithm_0(void)
{
	int channel;
	int preferredChannel=0;
	int minRssiChannel=0;
	int minRssi=100;
	for(channel = 26; channel >= 11; channel--) {
		cc2420_set_channel(channel);
		int tmp = cc2420_rssi()+100;
		if (DEBUG){
			tmp = rand() % 100 ;
		}
		if (tmp <minRssi){
			minRssi = tmp;
			minRssiChannel = channel;
			if (tmp < RSSI_THRESHOLD){
				preferredChannel = channel;
				printf ("The best channel is CH-%d and RSSI is %d.\n",preferredChannel,tmp);
				break;
			}
		}
	}
	if (preferredChannel==0){
		preferredChannel = minRssiChannel;
		printf ("The least bad channel is CH-%d.\n", preferredChannel);
	}
	return preferredChannel;
}


/*---------------------------------------------------------------------------*/
/* Wifi avoidance*/
static int find_channel_algorithm_1 (void)
{
	int channel;
	int preferredChannel=0;
	int minRssiChannel=0;
	int minRssi=100;
	
}

/*---------------------------------------------------------------------------*/
PROCESS(channel_selector, "Channel Selector");
AUTOSTART_PROCESSES(&channel_selector);
/*---------------------------------------------------------------------------*/
/* Broadcast channel selection to clients*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	printf("broadcast message received from %d.%d: '%s'\n",
		   from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(channel_selector, ev, data)
{
	static struct etimer etScan;
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	
	PROCESS_BEGIN();
	/* switch mac layer off, and turn radio on */
	
	broadcast_open(&broadcast, 129, &broadcast_call);
	
	NETSTACK_MAC.off(0);
	cc2420_on();
	
	while(1) {
		etimer_set (&etScan, CLOCK_SECOND*100);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etScan));
		int preferredChannel = find_channel_algorithm_0();
		if (DEBUG || currentChannel!=preferredChannel){
			printf("Need to change channel!\n");
			
			/*This is the instruction for channel change
			 * Code format:
			 * First digit:C
			 * Second and third digit: Channel number, e.g. 17
			 * Last 2 digit: Channel selection counter.
			 * This rule is implemented in instruction_generator()
			 */
			
			instruction_generator(preferredChannel,selectionCounter);
			
			packetbuf_copyfrom(instructionBuff, 6);
			
			/*Return to initial channel to broadcast channel change inst.*/
			cc2420_set_channel(currentChannel);
			
			broadcast_send(&broadcast);
			printf ("instruction : %s is broadcasted!\n",instructionBuff);
			
			currentChannel=preferredChannel;
			
			/*Change to best channel*/
			cc2420_set_channel(currentChannel);
			selectionCounter = (selectionCounter+1)%100;
		}
		else{
			printf("Do nothing!\n");
		}
	}
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
