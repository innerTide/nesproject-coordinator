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
 *         Yuefeng Wu, y.wu.5@student.tue.nl
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

#define RSSI_THRESHOLD 30
#define DEBUG 1
#define AVERAGE_COUNT 4
#define TOTAL_DELAY_TIME 256 /*Unit: uSec*/
#define SCAN_ALGORITHM 0 /*0 for Reversed Scan; 1 for Wi-Fi Aviodance; 2 for Greedy*/

/*---------------------------------------------------------------------------*/
/* This is a set of global variables for this laboratory*/

int currentChannel = 26;
unsigned int selectionCounter = 0;
char instructionBuff [6];

/*---------------------------------------------------------------------------*/
/* This is to generate the inst. to other clients */

static void instruction_generator (int preferredChannel, int selectionCounter){
	instructionBuff[0]='C';
	instructionBuff[1]=preferredChannel/10 + 48;
	instructionBuff[2]=preferredChannel%10 + 48;
	instructionBuff[3]=selectionCounter/10 + 48;
	instructionBuff[4]=selectionCounter%10 + 48;
	instructionBuff[5]='\0';
}

/*---------------------------------------------------------------------------*/
/* This is to calculate the average RSSI*/
static int cc2420_average_rssi(){
    int averagedRssi = 0;
    int i;
    for (i=0; i < AVERAGE_COUNT; i++){
        averagedRssi = averagedRssi + cc2420_rssi();
        clock_delay_usec (TOTAL_DELAY_TIME/AVERAGE_COUNT);
    }
    return averagedRssi/AVERAGE_COUNT;
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
                
                #if DEBUG
                /*This part of code is for debugging only.*/
                tmp = rand() % 100 ;
                #endif
		
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
	int preferredChannel=0;
	int minRssiChannel=0;
	int minRssi=100;
        unsigned char channlScanOrderArray[16] = {26,25,20,15,24,23,22,21,19,18,17,16,14,13,12,11};
        int i;
        for(i=0; i<16; i++) {
            cc2420_set_channel((int) channlScanOrderArray[i]);
            int tmp = cc2420_rssi()+100;
            #if DEBUG
            /*This part of code is for debugging only.*/
            tmp = rand() % 100 ;
            #endif
            if (tmp <minRssi){
                minRssi = tmp;
                minRssiChannel = (int) channlScanOrderArray[i];
                if (tmp < RSSI_THRESHOLD){
                    preferredChannel = (int) channlScanOrderArray[i];
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
/*Greedy Scan*/
static int find_channel_algorithm_2 (void){
    int preferredChannel=0;
    int minRssiChannel=0;
    int minRssi=100;
    unsigned char middleChannels[3] = {13,18,23};
    
    /*We need to firstly obtain the RSSI of CH-26 to chech if it is feasiable*/
    cc2420_set_channel(26);
    int rssiCh_26 = cc2420_rssi()+100;
    if (rssiCh_26 < RSSI_THRESHOLD){
        printf ("The best channel is CH-26 and RSSI is %d.\n",rssiCh_26);
        return 26;
    }
    
    int i;
    for(i=0; i<3; i++) {
        cc2420_set_channel((int) middleChannels[i]);
        int tmp = cc2420_rssi()+100;
        
        if (tmp <minRssi){
            minRssi = tmp;
            minRssiChannel = (int) middleChannels[i];
            if (tmp < RSSI_THRESHOLD){
                preferredChannel = (int) middleChannels[i] ;
                printf ("The best channel is CH-%d and RSSI is %d.\n",preferredChannel,tmp);
                break;
            }
        }
        
    }
    return preferredChannel;
    
    
    if ((preferredChannel==0) && (minRssiChannel!=26)){
        
        int leftSideRssi,rightSideRssi;
        
        /*Begin to exam the channel on the left*/
        cc2420_set_channel (minRssiChannel-1);
        leftSideRssi = cc2420_rssi()+100;
        if (leftSideRssi < RSSI_THRESHOLD){
            printf ("The best channel is CH-%d and RSSI is %d.\n",minRssiChannel-1,leftSideRssi);
            return minRssiChannel - 1;
        }
        
        /*Begin to exam the channel on the right*/
        cc2420_set_channel (minRssiChannel+1);
        rightSideRssi = cc2420_rssi()+100;
        if (rightSideRssi < RSSI_THRESHOLD){
            printf ("The best channel is CH-%d and RSSI is %d.\n",minRssiChannel+1,rightSideRssi);
            return minRssiChannel + 1;
        }
        
        
        if (leftSideRssi > rightSideRssi){
            cc2420_set_channel(minRssiChannel + 2);
            int tmp = cc2420_rssi() + 100;
            if (tmp < rightSideRssi){
                printf ("The least bad channel is CH-%d.\n", minRssiChannel+2);
                return minRssiChannel + 2;
            }
            else{
                printf ("The least bad channel is CH-%d.\n", minRssiChannel+1);
                return minRssiChannel + 1;
            }
        }
        else{
            cc2420_set_channel(minRssiChannel - 2);
            int tmp = cc2420_rssi() + 100;
            if (tmp < leftSideRssi){
                printf ("The least bad channel is CH-%d.\n", minRssiChannel-2);
                return minRssiChannel - 2;
            }
            else{
                printf ("The least bad channel is CH-%d.\n", minRssiChannel-1);
                return minRssiChannel - 1;
            }
        }
        
    }
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

                #if SCAN_ALGORITHM==0
                int preferredChannel = find_channel_algorithm_0();

                #elif SCAN_ALGORITHM==1
                int preferredChannel = find_channel_algorithm_1();

                #elif SCAN_ALGORITHM==2
                int preferredChannel = find_channel_algorithm_2();
                

                #else
                printf ("Macro is wrong, use algorithm 0!\n");
                int preferredChannel = find_channel_algorithm_0();

                #endif
                
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
