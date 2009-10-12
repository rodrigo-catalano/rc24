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
For the time being models are defined here and a recompile is needed to add or
adjust models.  In the long term they will be stored in flash and setup
on the controller.

*/

#include <jendefs.h>
#include <string.h>

#include "store.h"
#include "model.h"
#include "mymodels.h"
/*

int setupModels(model models[16])
{

    int i;
    for(i=0;i<16;i++)setModelDefaults(&models[i]);

    //alula

    //input channels 0 alierons 1 elevator

    strcpy(models[0].name,"Alula");
    models[0].rxMACl=0x000aa329;
    models[0].rxMACh=0x00158d00;


    // mix aileron to output0
    models[0].mixes[0][0].inChannel=0;
    models[0].mixes[0][0].highrate=40;
    models[0].mixes[0][0].lowrate=25;

    // mix elevator to output0
    models[0].mixes[0][1].inChannel=1;
    models[0].mixes[0][1].highrate=20;
    models[0].mixes[0][1].lowrate=14;

    // mix -aileron to output1
    models[0].mixes[1][0].inChannel=0;
    models[0].mixes[1][0].highrate=-40;
    models[0].mixes[1][0].lowrate=-25;

    // mix elevator to output0
    models[0].mixes[1][1].inChannel=1;
    models[0].mixes[1][1].highrate=20;
    models[0].mixes[1][1].lowrate=14;


    //dark impact

    strcpy(models[1].name,"Dark Impact");
    models[1].rxMACl=0x000aa329;
    models[1].rxMACh=0x00158d00;

    //Monster

    strcpy(models[2].name,"Monster");
    models[2].rxMACl=0x00068398;
    models[2].rxMACh=0x00158d00;

  //prop steer
    strcpy(models[3].name,"prop steer");
    models[3].rxMACl=0x000aa329;
    models[3].rxMACh=0x00158d00;


    // mix rudder to output2
    models[3].mixes[2][0].inChannel=3;
    models[3].mixes[2][0].highrate=100;
    models[3].mixes[2][0].lowrate=100;

    // mix throttle to output2
    models[3].mixes[2][1].inChannel=2;
    models[3].mixes[2][1].highrate=100;
    models[3].mixes[2][1].lowrate=100;

    // mix -rudder to output3
    models[3].mixes[3][0].inChannel=3;
    models[3].mixes[3][0].highrate=-100;
    models[3].mixes[3][0].lowrate=-100;

    // mix throttle to output3
    models[3].mixes[3][1].inChannel=2;
    models[3].mixes[3][1].highrate=100;
    models[3].mixes[3][1].lowrate=100;



    return 3;
}
*/
void setupAlula(modelEx* model)
{
    setModelDefaultsEx(model);

    strcpy(model->name,"Alula Evo");
    model->rxMACl=0x000aa329;
    model->rxMACh=0x00158d00;

    model->nActiveChannels=2;
    //aileron
    model->mixes[0].inChannel=0;
    model->mixes[0].outChannel=0;
    model->mixes[0].rateLow=-60;
    model->mixes[0].rateHigh=-90;
    model->mixes[0].lutType=NO_LUT;

    model->mixes[1].inChannel=0;
    model->mixes[1].outChannel=1;
    model->mixes[1].rateLow=-60;
    model->mixes[1].rateHigh=-90;
    model->mixes[1].lutType=NO_LUT;

    //elevator

    model->mixes[2].inChannel=1;
    model->mixes[2].outChannel=0;
    model->mixes[2].rateLow=-25;
    model->mixes[2].rateHigh=-37;
    model->mixes[2].lutType=NO_LUT;

    model->mixes[3].inChannel=1;
    model->mixes[3].outChannel=1;
    model->mixes[3].rateLow=25;
    model->mixes[3].rateHigh=37;
    model->mixes[3].lutType=NO_LUT;

}

