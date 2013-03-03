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


/*
This provides a means of posting arbitrary events from interrupt context to app context
in much the same way as the jennic AppQueueApi does for the predefined events.





*/

#include <jendefs.h>
#include <AppHardwareApi.h>
#include "swEventQueue.h"

typedef struct
{
    SW_EVENT_FN fn;
    void* context;
    void* data;
}swEventQueueItem;

#define MAX_SW_EVENT_QUEUE 10
swEventQueueItem swEventQueue[MAX_SW_EVENT_QUEUE];
uint8 swEventQueueRead=0;
uint8 swEventQueueWrite=0;

bool swEventQueuePush(SW_EVENT_FN fn ,void* context,void* data)
{
    //called in interrupt context
    //check for space - should never make top=bottom by writing
    uint8 nextIdx=swEventQueueWrite+1;
    if(nextIdx==MAX_SW_EVENT_QUEUE)nextIdx=0;
    if(nextIdx==swEventQueueRead)return FALSE;

    swEventQueue[swEventQueueWrite].fn=fn;
    swEventQueue[swEventQueueWrite].context=context;
    swEventQueue[swEventQueueWrite].data=data;
    swEventQueueWrite=nextIdx;
    return TRUE;
}
bool processSwEventQueue()
{
    //called in app context
    if(swEventQueueRead==swEventQueueWrite)return FALSE;
    else
    {
        (*swEventQueue[swEventQueueRead].fn)(swEventQueue[swEventQueueRead].context,swEventQueue[swEventQueueRead].data);
        if(swEventQueueRead==MAX_SW_EVENT_QUEUE-1)swEventQueueRead=0;
        else swEventQueueRead++;
        return TRUE;
    }
}
