/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Coordinator Source Code for 
 *         the values
 * \author
 *         Joakim Eriksson, joakime@sics.se
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "sys/etimer.h"
#include "dev/leds.h"
#include "cc2420.h"
#include "cc2420_const.h"
#include "dev/spi.h"
#include <stdio.h>


/*---------------------------------------------------------------------------*/
/* This is a set of predefined values for this laboratory*/

#define RSSI_THRESHOLD 20

/*---------------------------------------------------------------------------*/
/* This is a set of global variables for this laboratory*/

int currentChannel = 11;


/*---------------------------------------------------------------------------*/
/* This assumes that the CC2420 is always on and "stable" */
static void
set_frq(int c)
{
	int f;
	/* We can read even other channels with CC2420! */
	/* Fc = 2048 + FSCTRL  ;  Fc = 2405 + 5(k-11) MHz, k=11,12, ... , 26 */
	f = c*5 + 357; /* Start from 2405 MHz to 2480 MHz, step by 5*/
	
	/* Write the new channel value */
	CC2420_SPI_ENABLE();
	SPI_WRITE_FAST(CC2420_FSCTRL);
	SPI_WRITE_FAST((uint8_t)(f >> 8));
	SPI_WRITE_FAST((uint8_t)(f & 0xff));
	SPI_WAITFORTx_ENDED();
	SPI_WRITE(0);
	
	/* Send the strobe */
	SPI_WRITE(CC2420_SRXON);
	CC2420_SPI_DISABLE();
}


static int find_channel_algorithm_0(void)
{
	int channel;
	int preferredChannel=0;
	int minRssiChannel=0;
	int minRssi=100;
	for(channel = 15; channel >= 0; channel--) {
		set_frq(channel);
		int tmp = cc2420_rssi()+100;
		printf ("RSSI: %d\n",tmp);
		if (tmp <minRssi){
			minRssi = tmp;
			minRssiChannel = channel + 11;
			if (tmp < RSSI_THRESHOLD){
				preferredChannel = channel + 11;
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
PROCESS(channel_selector, "Channel Selector");
AUTOSTART_PROCESSES(&channel_selector);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(channel_selector, ev, data)
{
	static struct etimer etScan;
	
	PROCESS_BEGIN();
	/* switch mac layer off, and turn radio on */
	NETSTACK_MAC.off(0);
	cc2420_on();
	
	while(1) {
		etimer_set (&etScan, CLOCK_SECOND*5);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etScan));
		int preferredChannel = find_channel_algorithm_0();
		if (currentChannel!=preferredChannel){
			printf("Need to change channel!\n");
			currentChannel=preferredChannel;
		}
		else{
			printf("Do nothing!\n");
		}
	}
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
