#ifndef _RX_MAIN_H
#define _RX_MAIN_H

#if defined __cplusplus
extern "C" {
#endif


typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	uint16 rxDemands[20];	// Demanded positions from tx
	uint16 rxMixedDemands[20];  //positions post any mixing on rx
	int radioOverflows;
} Rx;

PUBLIC void AppColdStart(void);

void dbgPrintf(const char *fmt, ...);

#if defined __cplusplus
}
#endif
#endif
