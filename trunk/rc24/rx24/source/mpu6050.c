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
#include <AppHardwareApi.h>
#include <SMBus.h>
#include "hwutils.h"
#include "mpu6050.h"

#define MPU6050_AUX_VDDIO          0x01   // R/W
 #define MPU6050_SMPLRT_DIV         0x19   // R/W
 #define MPU6050_CONFIG             0x1A   // R/W
 #define MPU6050_GYRO_CONFIG        0x1B   // R/W
 #define MPU6050_ACCEL_CONFIG       0x1C   // R/W
 #define MPU6050_FF_THR             0x1D   // R/W
 #define MPU6050_FF_DUR             0x1E   // R/W
 #define MPU6050_MOT_THR            0x1F   // R/W
 #define MPU6050_MOT_DUR            0x20   // R/W
 #define MPU6050_ZRMOT_THR          0x21   // R/W
 #define MPU6050_ZRMOT_DUR          0x22   // R/W
 #define MPU6050_FIFO_EN            0x23   // R/W
 #define MPU6050_I2C_MST_CTRL       0x24   // R/W
 #define MPU6050_I2C_SLV0_ADDR      0x25   // R/W
 #define MPU6050_I2C_SLV0_REG       0x26   // R/W
 #define MPU6050_I2C_SLV0_CTRL      0x27   // R/W
 #define MPU6050_I2C_SLV1_ADDR      0x28   // R/W
 #define MPU6050_I2C_SLV1_REG       0x29   // R/W
 #define MPU6050_I2C_SLV1_CTRL      0x2A   // R/W
 #define MPU6050_I2C_SLV2_ADDR      0x2B   // R/W
 #define MPU6050_I2C_SLV2_REG       0x2C   // R/W
 #define MPU6050_I2C_SLV2_CTRL      0x2D   // R/W
 #define MPU6050_I2C_SLV3_ADDR      0x2E   // R/W
 #define MPU6050_I2C_SLV3_REG       0x2F   // R/W
 #define MPU6050_I2C_SLV3_CTRL      0x30   // R/W
 #define MPU6050_I2C_SLV4_ADDR      0x31   // R/W
 #define MPU6050_I2C_SLV4_REG       0x32   // R/W
 #define MPU6050_I2C_SLV4_DO        0x33   // R/W
 #define MPU6050_I2C_SLV4_CTRL      0x34   // R/W
 #define MPU6050_I2C_SLV4_DI        0x35   // R
 #define MPU6050_I2C_MST_STATUS     0x36   // R
 #define MPU6050_INT_PIN_CFG        0x37   // R/W
 #define MPU6050_INT_ENABLE         0x38   // R/W
 #define MPU6050_INT_STATUS         0x3A   // R
 #define MPU6050_ACCEL_XOUT_H       0x3B   // R
 #define MPU6050_ACCEL_XOUT_L       0x3C   // R
 #define MPU6050_ACCEL_YOUT_H       0x3D   // R
 #define MPU6050_ACCEL_YOUT_L       0x3E   // R
 #define MPU6050_ACCEL_ZOUT_H       0x3F   // R
 #define MPU6050_ACCEL_ZOUT_L       0x40   // R
 #define MPU6050_TEMP_OUT_H         0x41   // R
 #define MPU6050_TEMP_OUT_L         0x42   // R
 #define MPU6050_GYRO_XOUT_H        0x43   // R
 #define MPU6050_GYRO_XOUT_L        0x44   // R
 #define MPU6050_GYRO_YOUT_H        0x45   // R
 #define MPU6050_GYRO_YOUT_L        0x46   // R
 #define MPU6050_GYRO_ZOUT_H        0x47   // R
 #define MPU6050_GYRO_ZOUT_L        0x48   // R
 #define MPU6050_EXT_SENS_DATA_00   0x49   // R
 #define MPU6050_EXT_SENS_DATA_01   0x4A   // R
 #define MPU6050_EXT_SENS_DATA_02   0x4B   // R
 #define MPU6050_EXT_SENS_DATA_03   0x4C   // R
 #define MPU6050_EXT_SENS_DATA_04   0x4D   // R
 #define MPU6050_EXT_SENS_DATA_05   0x4E   // R
 #define MPU6050_EXT_SENS_DATA_06   0x4F   // R
 #define MPU6050_EXT_SENS_DATA_07   0x50   // R
 #define MPU6050_EXT_SENS_DATA_08   0x51   // R
 #define MPU6050_EXT_SENS_DATA_09   0x52   // R
 #define MPU6050_EXT_SENS_DATA_10   0x53   // R
 #define MPU6050_EXT_SENS_DATA_11   0x54   // R
 #define MPU6050_EXT_SENS_DATA_12   0x55   // R
 #define MPU6050_EXT_SENS_DATA_13   0x56   // R
 #define MPU6050_EXT_SENS_DATA_14   0x57   // R
 #define MPU6050_EXT_SENS_DATA_15   0x58   // R
 #define MPU6050_EXT_SENS_DATA_16   0x59   // R
 #define MPU6050_EXT_SENS_DATA_17   0x5A   // R
 #define MPU6050_EXT_SENS_DATA_18   0x5B   // R
 #define MPU6050_EXT_SENS_DATA_19   0x5C   // R
 #define MPU6050_EXT_SENS_DATA_20   0x5D   // R
 #define MPU6050_EXT_SENS_DATA_21   0x5E   // R
 #define MPU6050_EXT_SENS_DATA_22   0x5F   // R
 #define MPU6050_EXT_SENS_DATA_23   0x60   // R
 #define MPU6050_MOT_DETECT_STATUS  0x61   // R
 #define MPU6050_I2C_SLV0_DO        0x63   // R/W
 #define MPU6050_I2C_SLV1_DO        0x64   // R/W
 #define MPU6050_I2C_SLV2_DO        0x65   // R/W
 #define MPU6050_I2C_SLV3_DO        0x66   // R/W
 #define MPU6050_I2C_MST_DELAY_CTRL 0x67   // R/W
 #define MPU6050_SIGNAL_PATH_RESET  0x68   // R/W
 #define MPU6050_MOT_DETECT_CTRL    0x69   // R/W
 #define MPU6050_USER_CTRL          0x6A   // R/W
 #define MPU6050_PWR_MGMT_1         0x6B   // R/W
 #define MPU6050_PWR_MGMT_2         0x6C   // R/W
 #define MPU6050_FIFO_COUNTH        0x72   // R/W
 #define MPU6050_FIFO_COUNTL        0x73   // R/W
 #define MPU6050_FIFO_R_W           0x74   // R/W
 #define MPU6050_WHO_AM_I           0x75   // R

//#if (defined JN5148 || defined JN5168 )
//#else
	#define bAHI_SiMasterPollRxNack bAHI_SiPollRxNack
//#endif

PRIVATE bool_t bi2cBusWait(void)
{

	while(bAHI_SiPollTransferInProgress()); /* busy wait */

	if (bAHI_SiPollArbitrationLost() | bAHI_SiMasterPollRxNack())	{

		/* release bus & abort */
		vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT,
						 E_AHI_SI_STOP_BIT,
						 E_AHI_SI_NO_SLAVE_READ,
						 E_AHI_SI_SLAVE_WRITE,
						 E_AHI_SI_SEND_ACK,
						 E_AHI_SI_NO_IRQ_ACK);
		return(FALSE);
	}

	return(TRUE);

}
PUBLIC bool_t bi2cBusWriteCmd(uint8 u8Address, uint8 u8Command)
{

	/* Send address with write bit set */
	vAHI_SiWriteSlaveAddr(u8Address, E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bi2cBusWait()) return(FALSE);


	/* Send command byte */
	vAHI_SiWriteData8(u8Command);
	vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bi2cBusWait()) return(FALSE);
	return TRUE;
}

PUBLIC bool_t bi2cBusRandomRead(uint8 u8Address, uint8 u8Command, uint8 u8Length, uint8* pu8Data)
{

	/* Send address with write bit set */
	vAHI_SiWriteSlaveAddr(u8Address, E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiSetCmdReg(E_AHI_SI_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bi2cBusWait()) return(FALSE);


	/* Send command byte */
	vAHI_SiWriteData8(u8Command);
	vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT,
					 E_AHI_SI_NO_STOP_BIT,
					 E_AHI_SI_NO_SLAVE_READ,
					 E_AHI_SI_SLAVE_WRITE,
					 E_AHI_SI_SEND_ACK,
					 E_AHI_SI_NO_IRQ_ACK);

	if(!bi2cBusWait()) return(FALSE);


	vAHI_SiWriteSlaveAddr(u8Address, !E_AHI_SI_SLAVE_RW_SET);

	vAHI_SiSetCmdReg(E_AHI_SI_START_BIT,
						 E_AHI_SI_NO_STOP_BIT,
						 E_AHI_SI_NO_SLAVE_READ,
						 E_AHI_SI_SLAVE_WRITE,
						 E_AHI_SI_SEND_ACK,
						 E_AHI_SI_NO_IRQ_ACK);

	if(!bi2cBusWait()) return(FALSE);


	while(u8Length > 0){

		u8Length--;

		/*
		 * If its the last byte to be read, read with
		 * stop sequence set
		 */
		if(u8Length == 0){

			vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_NACK,
							 E_AHI_SI_NO_IRQ_ACK);

		} else {

			vAHI_SiSetCmdReg(E_AHI_SI_NO_START_BIT,
							 E_AHI_SI_NO_STOP_BIT,
							 E_AHI_SI_SLAVE_READ,
							 E_AHI_SI_NO_SLAVE_WRITE,
							 E_AHI_SI_SEND_ACK,
							 E_AHI_SI_NO_IRQ_ACK);

		}

		while(bAHI_SiPollTransferInProgress()); /* busy wait */

		*pu8Data++ = u8AHI_SiReadData8();

	}

	return(TRUE);

}
bool_t bSMBusWriteByte(uint8 address,uint8 reg,uint8 b)
{
	return bSMBusWrite(address, reg, 1, &b);
}


bool MPU9150_setupCompass(mpu6050* mpu)
{
  //code taken from http://playground.arduino.cc/Main/MPU-9150
	bool_t bOk= TRUE;
	uint8 buff[3];


	bOk &=bSMBusWriteByte(mpu->I2Caddress,MPU6050_INT_PIN_CFG , 0x02); //set i2c bypass enable pin to true to access magnetometer

	bOk &=bSMBusWriteByte(mpu->compassI2Caddress,0x0A, 0x0f); //read fuse bits

	bOk &=bi2cBusRandomRead(mpu->compassI2Caddress, 0x10, 3, buff);


	//((asa-128)*0.5)/128+1;
	mpu->rawValues.x_mag_sens=(buff[0]-128)+256;
	mpu->rawValues.y_mag_sens=(buff[1]-128)+256;
	mpu->rawValues.z_mag_sens=(buff[2]-128)+256;

	dbgPrintf("sens %i %i %i   ",mpu->rawValues.x_mag_sens,mpu->rawValues.y_mag_sens,mpu->rawValues.z_mag_sens);

	bOk &=bSMBusWriteByte(mpu->compassI2Caddress,0x0A, 0x00); //PowerDownMode
	cycleDelay(120*16);
	bOk &=bSMBusWriteByte(mpu->compassI2Caddress, 0x0A, 0x01); //start sample capture

/*
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x24, 0x40); //Wait for Data at Slave0
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x25, 0x8C); //Set i2c address at slave0 at 0x0C
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x26, 0x02); //Set where reading at slave 0 starts
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x27, 0x88); //set offset at start reading and enable
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x28, 0x0C); //set i2c address at slv1 at 0x0C
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x29, 0x0A); //Set where reading at slave 1 starts
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x2A, 0x81); //Enable at set length to 1
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x64, 0x01); //overvride register
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x67, 0x03); //set delay rate
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x01, 0x80);

	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x34, 0x04); //set i2c slv4 delay
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x64, 0x00); //override register
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x6A, 0x00); //clear usr setting
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x64, 0x01); //override register
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x6A, 0x20); //enable master i2c mode
	bOk &=bSMBusWriteByte(mpu->I2Caddress,0x34, 0x13); //disable slv4
*/
	return bOk;
}


bool initMpu6050(mpu6050* mpu)
{
	mpu->rawValues.x_accel=0;
	mpu->rawValues.y_accel=0;
	mpu->rawValues.z_accel=0;
	mpu->rawValues.temperature=0;
	mpu->rawValues.x_gyro=0;
	mpu->rawValues.y_gyro=0;
	mpu->rawValues.z_gyro=0;
	mpu->rawValues.x_mag=0;
	mpu->rawValues.y_mag=0;
	mpu->rawValues.z_mag=0;
	mpu->whoami=0;
	bool_t bOk = TRUE;
  //  vSMBusInit();
    vAHI_SiConfigure(TRUE, FALSE, 7);

    uint8 buff[1];
    buff[0]=0;


    buff[0]=0x00;
    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_PWR_MGMT_1, 1, &buff[0]);
    cycleDelay(800*16);
/*
    cycleDelay(800*16);

    bOk &=bi2cBusRandomRead(mpu->I2Caddress, MPU6050_WHO_AM_I, 1,&(mpu->whoami));

    cycleDelay(800*16);
    buff[0]=0x80;
    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_PWR_MGMT_1, 1, &buff[0]);
    cycleDelay(800*16);
    buff[0]=0x03;
    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_PWR_MGMT_1, 1, &buff[0]);
    cycleDelay(800*16);

    buff[0]=0x00;

    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_CONFIG, 1, &buff[0]);
*/
    buff[0]=0x18;

    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_GYRO_CONFIG, 1, &buff[0]);

    buff[0]=0x10;
    bOk &=bSMBusWrite(mpu->I2Caddress, MPU6050_ACCEL_CONFIG, 1, &buff[0]);


    bOk &=MPU9150_setupCompass(mpu);
    cycleDelay(800*16);



    return bOk;
}
bool readMpu6050(mpu6050* mpu)
{
	uint8 buff[8];

	static int compass=2;
	bool_t bOk = TRUE;

	 if(compass>=10)
	 {
		 bOk &=bi2cBusRandomRead(mpu->compassI2Caddress, 0x02, 8, buff);
	 }
	 bOk &=bi2cBusRandomRead(mpu->I2Caddress, MPU6050_ACCEL_XOUT_H, 14, (uint8 *) &(mpu->rawValues));
	// bOk &=bi2cBusRandomRead(mpu->I2Caddress, MPU6050_ACCEL_XOUT_H, 20, (uint8 *) &(mpu->rawValues));

	 //read compass at slower rate
	 if(compass>=10)
	 {
		 compass=0;
		 bOk &=bSMBusWriteByte(mpu->compassI2Caddress, 0x0A, 0x01);

		 //data ready, valid and not saturated
		 if(buff[0]==1 && buff[7]==0)
		 {
			 int16 t=(buff[2]<<8)|buff[1];
		//	 mpu->rawValues.x_mag=(((int)t)*mpu->rawValues.x_mag_sens+mpu->rawValues.x_mag*63)>>6;
			 mpu->rawValues.x_mag=((int)t)*mpu->rawValues.x_mag_sens;

			 t=(buff[4]<<8)|buff[3];
			// mpu->rawValues.y_mag=(((int)t)*mpu->rawValues.y_mag_sens+mpu->rawValues.y_mag*63)>>6;
			 mpu->rawValues.y_mag=((int)t)*mpu->rawValues.y_mag_sens;

			 t=(buff[6]<<8)|buff[5];
			 //mpu->rawValues.z_mag=(((int)t)*mpu->rawValues.z_mag_sens+mpu->rawValues.z_mag*63)>>6;
			 mpu->rawValues.z_mag=((int)t)*mpu->rawValues.z_mag_sens;
		 }
	 }
	 else compass++;
	 //todo check byte order

	 return bOk;
}
bool initMS5611(MS5611 *sensor)
{
	bool_t bOk = TRUE;
	vAHI_SiConfigure(TRUE, FALSE, 7);

	//reset
	bOk &=bi2cBusWriteCmd(sensor->addr,0x1e);
	cycleDelay(10000*16);

	 bOk &=bi2cBusRandomRead(sensor->addr, 0xa2, 2, &sensor->sens);
	 bOk &=bi2cBusRandomRead(sensor->addr, 0xa4, 2, &sensor->off);
	 bOk &=bi2cBusRandomRead(sensor->addr, 0xa6, 2, &sensor->tcs);
	 bOk &=bi2cBusRandomRead(sensor->addr, 0xa8, 2, &sensor->tco);
	 bOk &=bi2cBusRandomRead(sensor->addr, 0xaa, 2, &sensor->tref);
	 bOk &=bi2cBusRandomRead(sensor->addr, 0xac, 2, &sensor->tempsens);

	 bOk &=bi2cBusWriteCmd(sensor->addr,0x58);

	 dbgPrintf("%i %i %i %i %i %i",
			 sensor->sens,sensor->off,sensor->tcs,sensor->tco,sensor->tref,sensor->tempsens);

	 sensor->state=0;
	 return bOk;
}
bool readMS5611(MS5611 *sensor)
{
	uint32 temp=0;
	uint32 pressure=0;
	bool_t bOk = TRUE;

	switch(sensor->state)
	{
	case 0:
		bOk &=bi2cBusRandomRead(sensor->addr, 0x00, 3, ((uint8*)&temp)+1);
		bOk &=bi2cBusWriteCmd(sensor->addr,0x48);//start p read
		if(bOk)
		{
	//		temp>>=8;
			sensor->dt=temp-(sensor->tref<<8);
			sensor->temp=2000+((((int64)sensor->dt)*((uint64)sensor->tempsens))>>23);
		}
		sensor->state++;
		break;
	case 1:
		bOk &=bi2cBusRandomRead(sensor->addr, 0x00, 3, ((uint8*)&pressure)+1);
		bOk &=bi2cBusWriteCmd(sensor->addr,0x58);// start t read
		sensor->state=0;
		if(bOk)
		{
		//	pressure>>=8;

		//	int64_t off  = ((uint32_t)_C[off] <<16) + (((int64_t)dT * _C[tco]) >> 7);
		//	int64_t sens = ((uint32_t)_C[sens] <<15) + (((int64_t)dT * _C[tcs]) >> 8);
		//	return ((( (rawPress * sens ) >> 21) - off) >> 15) / 100.0;

			int64 off=(((uint32)sensor->off)<<16)+(((uint64)sensor->tco)*((int64)sensor->dt)>>7);
			int64 sens=(((uint32)sensor->sens)<<15)+(((uint64)sensor->tcs)*((int64)sensor->dt)>>8);
			int64 p=((((int64)pressure)*sens>>21)-off)>>15;
			sensor->pressure=p;
		}
		break;
	}
	return bOk;
}
bool readMS5611test(MS5611 *sensor)
{
	int temp=0;
	int pressure=0;

	bool_t bOk = TRUE;

	bOk &=bi2cBusWriteCmd(sensor->addr,0x48);
	cycleDelay(10000*16);
	bOk &=bi2cBusRandomRead(sensor->addr, 0x00, 3, &pressure);

	bOk &=bi2cBusWriteCmd(sensor->addr,0x58);
	cycleDelay(10000*16);
	bOk &=bi2cBusRandomRead(sensor->addr, 0x00, 3, &temp);


	if(bOk)
	{
		temp>>=8;
		pressure>>=8;

		int dt=temp-(sensor->tref<<8);
		sensor->temp=2000+((((int64)dt)*((int64)sensor->tempsens))>>23);

		int64 off=(((int64)sensor->off)<<16)+(((int64)sensor->tco)*((int64)dt)>>7);

		int64 sens=(((int64)sensor->sens)<<15)+(((int64)sensor->tcs)*((int64)dt)>>8);

		int64 p=((((int64)pressure)*sens>>21)-off)>>15;

		sensor->pressure=p;

		dbgPrintf("\r\n%i %i",sensor->temp,sensor->pressure);

		//h=44331.5 - 4946.62 P^0.190263
	}
	else dbgPrintf("alt read failed");

	return bOk;

}
