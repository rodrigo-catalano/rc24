#ifndef _FFLOAT_H
#define _FFLOAT_H


#if defined __cplusplus
extern "C" {
#endif

typedef union
{
  int i;
  float f;
}__ffmath;

float ffabs(float a);

float ffadd(float a, float b);
float ffsub(float a, float b);
float ffmul(float a, float b);
float ffdiv(float a, float b);

float ffsqrt(float a);
float ffrsqrt(float a);
float fflog (float val);
float ffatan2(float y,float x);
float ffacos(float x);

float rshift(float a,int i);
float lshift(float a,int i);

static inline int toInt(float x)
{
	__ffmath ai;
	int ae,am;
	ai.f=x;

	ae=(ai.i>>23)&0x000000ff;
	if(ae==0)am=0;
	else if(ai.i&0x80000000) am= -((ai.i & 0x007fffff )| 0x00800000);
	else am=(ai.i & 0x007fffff )| 0x00800000;
	return am;

}
typedef struct
{
	float x;
	float y;
	float z;
} vector3f;

typedef struct
{
	float w;
	float x;
	float y;
	float z;
} quaternionf;

typedef struct
{
	int w;
	int x;
	int y;
	int z;
} quaternioni;

vector3f v_rotate(vector3f* a,quaternionf* rot);

void q_mul(quaternionf* a,quaternionf* b,quaternionf* c);
void q_normalize(quaternionf* a);
quaternionf q_invert(quaternionf* a);

void q_rotate(quaternionf* q,float ax, float ay,float az);
void q_lowpass(quaternionf* prev,quaternionf* n,float t);
quaternionf q_FromVectors(vector3f* a,vector3f* b,vector3f* c);
quaternionf q_FromGM(vector3f* acc,vector3f* mag);

quaternioni q_toInt(quaternionf* q,int shift);

#if defined __cplusplus
}
#endif
#endif
