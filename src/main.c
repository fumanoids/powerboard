/*
 * main.c
 *
 *  Created on: Jun 8, 2012
 *      Author: lutz
 */

#include <flawless/init/systemInitializer.h>

#include <flawless/core/msg_msgPump.h>

int main(void)
{
	systemInitialize();


//	// the main loop
	msgPump_pumpMessage();


	return 0;
}

