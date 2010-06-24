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



#define MAXCHANNELS 20
#define MIXESPERCHANNEL 2
#define MAXMIXES (MAXCHANNELS * 2)

#define NOMIX 255

typedef void (*CUSTOM_MIX_FN)(int* in,int* out,void* mod);


typedef enum
{
    NO_LUT,
    EXPO_LUT,
    FREE_LUT,
}mixLutType;

typedef struct
{
    uint8 inChannel;
    uint8 outChannel;
    int16 rateLow;
    int16 rateHigh;
    mixLutType lutType;
    int16 lutParam;  //eg expo rate
    int16 lut[17];
}mixEx;


typedef struct
{
    uint32 rxMACh;
    uint32 rxMACl;
    char name[32];
    uint8 nActiveChannels;
    uint16 highEndStop[MAXCHANNELS];
    uint16 lowEndStop[MAXCHANNELS];
    int trim[MAXCHANNELS];
    int failsafe[MAXCHANNELS];
    mixEx mixes[MAXCHANNELS*2];
    int rateMode;
    CUSTOM_MIX_FN mixFunction;
    uint8 nFullSpeedChannels;

}modelEx;




void doMixingEx(int* in, int* out, modelEx* mod);
void setupDefaultModel(modelEx* mod);

uint16 storeModels(store* s,modelEx* mod, store* old, int16 modelIdx);
uint16 storeModel(store* s,modelEx* mod);
uint16 storeMix(store* s,mixEx* mix);
void readModels(store* s,modelEx* mod,int16* modelIdx, int16* nmodels);
void readModel(store* s,modelEx* mod);
void readMix(store* s,mixEx* mod);
void setModelDefaultsEx(modelEx* mod);
void loadModelByIdx(store* s,modelEx* mod,int16 modelIdx);

