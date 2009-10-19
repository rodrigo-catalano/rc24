/*
 Copyright 2008 - 2009 © Alan Hopper

 This file is part of rc24.

 rc24 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 rc24 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with rc24.  If not, see <http://www.gnu.org/licenses/>.


 */

// TODO allow for high power jn5148 having 1 fewer channel.

#include <jendefs.h>
#include <stdlib.h>
#include "hopping.h"

static uint16 rawHopSeq[] =
{ 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 25, 24, 23,
		22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11 };

static uint16 hopSeq[] =
{ 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 25, 24, 23,
		22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11 };

static uint16 fixedSeq[] =
{ 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11 };

uint16* channelSeq;
uint8 rxStartupSeq = 0;

hopMode currentHopMode = hoppingFixed;

void setHopMode(hopMode mode)
{
	currentHopMode = mode;
}
hopMode getHopMode()
{
	return currentHopMode;
}
#define RANDA 1664525
#define RANDC 1013904223

void randomizeHopSequence(uint32 seed)
{

	channelSeq = hopSeq;
	//we don't want interrupts using the rand() function during this!

	int perm[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	int i, n, temp;

	//create a seeded pseudo random permutation of channels by swapping

	//   srand(seed);
	//use private rand function to avoid interrupt problem when changing models
	//quick and dirty generator from 'numerical recipes in C'
	uint32 randn = seed;

	for (i = 0; i < 15; i++)
	{
		//  n=rand()%(16-i)+i;
		randn = RANDA * randn + RANDC;
		n = randn % (16 - i) + i;

		temp = perm[n];
		perm[n] = perm[i];
		perm[i] = temp;
	}

	for (i = 0; i < 31; i++)
	{
		hopSeq[i] = perm[rawHopSeq[i] - 11] + 11;
	}

}
uint32 getNextInHopSequence(uint8* currentidx)
{
	uint8 idx = (*currentidx);
	idx++;
	if (idx > 30)
		idx = 0;
	*currentidx = idx;
//	return (uint32) channelSeq[idx];

	return getHopChannel(idx*20000*16);

}
uint32 getHopChannel(uint32 seqClock)
{
	switch (currentHopMode)
	{
	case hoppingContinuous:
		return (uint32) channelSeq[seqClock / (20000* 16 )];
	case hoppingFixed:
		return 11;
	case hoppingRxStartup:
	{
		//change every 16 frames
		rxStartupSeq++;//let it overflow and start again
		return (rxStartupSeq >> 4) + 11;
	}
	}
	return 11;
}
