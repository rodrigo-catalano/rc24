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


#include <jendefs.h>
#include <string.h>

#include "store.h"
#include "model.h"


int interp(int x, int16* table);
void buildExpoLut(mixEx* mix);

void doMixingEx(int* in, int* out, modelEx* mod)
{
    int i,mixidx;

    for(i=0;i<mod->nActiveChannels;i++)
    {
        out[i]=0;
    }

    for(mixidx=0;mixidx<MAXCHANNELS*2;mixidx++)
    {
        mixEx* mix = &mod->mixes[mixidx];
        if(mix->inChannel!=NOMIX && mix->outChannel<MAXCHANNELS )
        {
            int rate;
            if(mod->rateMode==0)rate=(int)mix->rateLow;
            else rate=(int)mix->rateHigh;

            int inp=in[mix->inChannel]+mod->trim[mix->inChannel];
            if(mix->lutType!=NO_LUT) out[mix->outChannel]+= interp(inp,mix->lut)*rate/100;
            else out[mix->outChannel]+= inp*rate/100;
        }
    }

    //adjust to 12bit value and apply end limits

    for(i=0;i<MAXCHANNELS;i++)
    {
        out[i]+=2048;
        if(out[i]<mod->lowEndStop[i])out[i]=mod->lowEndStop[i];
        if(out[i]>=mod->highEndStop[i])out[i]=mod->highEndStop[i];
    }

}

int interp(int x, int16* table)
{
    x-= -2048;
    int lIdx=x>>8;
    if(lIdx<0)return table[0];
    if(lIdx>=16)return table[16];
    int ly=table[lIdx];
    int uy=table[lIdx+1];
    return ly+((uy-ly)*(x-(lIdx<<8))>>8);
}
void setupDefaultModel(modelEx* mod)
{
    int i;
    strcpy(mod->name,"Default Model");
    mod->nActiveChannels=20;
    mod->rateMode=1;
    for(i=0;i<MAXCHANNELS;i++)
    {
        mod->mixes[i].inChannel=i;
        mod->mixes[i].outChannel=i;
        mod->mixes[i].rateLow=70;
        mod->mixes[i].rateHigh=100;
        mod->mixes[i].lutType=NO_LUT;
        mod->mixes[i].lutParam=0;

   //     buildExpoLut(&mod->mixes[i]);

        mod->highEndStop[i]=4095;
        mod->lowEndStop[i]=0;
        mod->trim[i]=0;

        mod->mixes[i+MAXCHANNELS].inChannel=NOMIX;

    }
}
void buildExpoLut(mixEx* mix)
{
    int ii;
    int exp=-(int)mix->lutParam;
    if (exp < 0) exp = exp * 57 / 100;

    for ( ii= 0; ii < 17; ii++)
    {
         int i = ii*256-2048;
         //avoid 32 bit overflow on 2048 cubed
         mix->lut[ii] = (100-exp) * i / 100 + i * i / 8 * i / 100 * exp / 524288;

    }
}
uint16 storeModels(store* s,modelEx* mod, store* old, int16 modelIdx)
{

	// we only keep the current model in memory so
    // copy all from old store except for current live model
    store section;
    store oldmodels;
    store oldmodel;
    uint16 oldidx=0;
    modelEx omodel;
    bool modelStored=FALSE;

    storeStartSection(s,STOREMODELSSECTION,&section);

    storeInt16Section(&section,STORE_LASTMODELIDX,modelIdx);

    if(old!=NULL && storeFindSection(old,STOREMODELSSECTION,&oldmodels)==TRUE)
    {
        while(storeFindSection(&oldmodels,STORE_MODEL,&oldmodel)!=FALSE)
        {
            if(oldidx==modelIdx)
            {
                //store live model
                storeModel(&section,mod);
                modelStored=TRUE;
            }
            else
            {
                // copy model from old store
                readModel(&oldmodel,&omodel);
                storeModel(&section,&omodel);
            }
            oldidx++;

        }
    }

    if(modelStored==FALSE)storeModel(&section,mod);

    return storeEndSection(s,&section);

}
void loadModelByIdx(store* s,modelEx* mod,int16 modelIdx)
{
    store oldModels;
    store section;
    int16 modelsFound=0;
    if(storeFindSection(s,STOREMODELSSECTION,&oldModels)==TRUE)
    {
        while(storeFindSection(&oldModels,STORE_MODEL,&section)==TRUE)
        {
            if(modelsFound== modelIdx)
            {
                 readModel(&section,mod);
            }
            modelsFound++;
         }
    }
}
void readModels(store* s,modelEx* mod,int16* modelIdx, int16* nmodels)
{
    int tag;
    store section;
    *modelIdx=0;
    int16 modelsFound=0;
    while((tag=storeGetSection(s,&section))>0)
    {
        switch(tag)
        {
           case STORE_LASTMODELIDX:
           {
               *modelIdx=readInt16(&section);
                break;
           }
           case STORE_MODEL:
           {
                if(modelsFound== *modelIdx)
                {
                    readModel(&section,mod);
                }
                modelsFound++;
           }
        }
    }
    *nmodels=modelsFound;
}
uint16 storeModel(store* s,modelEx* mod)
{
    int i;
    store section;
    storeStartSection(s,STORE_MODEL,&section);

    writeInt32(&section,mod->rxMACh);
    writeInt32(&section,mod->rxMACl);
    writeString(&section,&(mod->name[0]));
    writeUint8(&section,mod->nActiveChannels);
    for(i=0;i<MAXCHANNELS;i++)writeInt16(&section,mod->highEndStop[i]);
    for(i=0;i<MAXCHANNELS;i++)writeInt16(&section,mod->lowEndStop[i]);
    for(i=0;i<MAXCHANNELS;i++)writeInt16(&section,mod->trim[i]);
    for(i=0;i<MAXCHANNELS;i++)writeInt16(&section,mod->failsafe[i]);
    uint8 nMixes=0;
    for(i=0;i<MAXCHANNELS*2;i++)
    {
        if(mod->mixes[i].inChannel!=NOMIX)nMixes++;
    }
    writeUint8(&section,nMixes);
    for(i=0;i<MAXCHANNELS*2;i++)
    {
        if(mod->mixes[i].inChannel!=NOMIX)
        {
            storeMix(&section,&mod->mixes[i]);
        }
    }
    writeUint8(&section,mod->rateMode);
    return storeEndSection(s,&section);

}
uint16 storeMix(store* s,mixEx* mix)
{
    //todo use structured store properly

	int i;
    store section;
    storeStartSection(s,1,&section);

    writeUint8(&section,mix->inChannel);
    writeUint8(&section,mix->outChannel);
    writeInt16(&section,mix->rateLow);
    writeInt16(&section,mix->rateHigh);
    writeUint8(&section,mix->lutType);
    writeInt16(&section,mix->lutParam);
    if(mix->lutType!=NO_LUT)
    {
        for(i=0;i<17;i++) writeInt16(&section,mix->lut[i]);
    }

    return storeEndSection(s,&section);
}
void readModel(store* s,modelEx* mod)
{
    int i;

    mod->rxMACh=readInt32(s);
    mod->rxMACl=readInt32(s);
    readString(s,mod->name,sizeof(mod->name));

    mod->nActiveChannels=readUint8(s);
    for(i=0;i<MAXCHANNELS;i++)mod->highEndStop[i]=readInt16(s);
    for(i=0;i<MAXCHANNELS;i++)mod->lowEndStop[i]=readInt16(s);
    for(i=0;i<MAXCHANNELS;i++)mod->trim[i]=readInt16(s);
    for(i=0;i<MAXCHANNELS;i++)mod->failsafe[i]=readInt16(s);
    uint8 nMixes=readUint8(s);

    for(i=0;i<nMixes;i++)
    {
        readMix(s,&mod->mixes[i]);
    }
    //clear remaining mixes
    for(i=nMixes;i<MAXCHANNELS*2;i++)
    {
        mod->mixes[i].inChannel=NOMIX;
    }

    mod->rateMode=readUint8(s);
}
void readMix(store* s,mixEx* mix)
{
    int i;
    readUint8(s);//type
    readInt16(s);//len

    mix->inChannel=readUint8(s);
    mix->outChannel=readUint8(s);
    mix->rateLow=readInt16(s);
    mix->rateHigh=readInt16(s);
    mix->lutType=readUint8(s);
    mix->lutParam=readInt16(s);
    if(mix->lutType!=NO_LUT)
    {
        for(i=0;i<17;i++) mix->lut[i]=readInt16(s);
    }
}
void setModelDefaultsEx(modelEx* mod)
{

    int i,mix;
    for(i=0;i<MAXCHANNELS;i++)
    {
        mod->highEndStop[i]=4095;
        mod->lowEndStop[i]=0;
        mod->trim[i]=0;
        mod->failsafe[i]=0;

        mod->name[0]='\0';
        mod->rateMode = 0;
        mod->mixFunction=NULL;

    }
    for(mix=0;mix<2*MAXCHANNELS;mix++)
    {
        mod->mixes[mix].inChannel=NOMIX;
        mod->mixes[mix].rateHigh=0;
        mod->mixes[mix].rateLow=0;
    }

}
