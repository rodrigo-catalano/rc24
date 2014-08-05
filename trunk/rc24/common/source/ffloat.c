//#include <jendefs.h>
#include <stdint.h>
#include <math.h>
#include "ffloat.h"


unsigned char msbit2[]={ 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
						5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
						6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
						6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
						7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
						7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
						7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
						7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
						8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};


float ffabs(float a)
{
	__ffmath x;
	x.f=a;
	x.i&=0x7fffffff;
	return x.f;
}



float rshift(float f,int i)
{
	//TODO limit check and optimise
	__ffmath x;
	int am,ae;
	x.f=f;
	am=x.i & 0x807fffff ;
	ae=(x.i>>23)&0x000000ff;
	ae-=i;
	if(ae<0)
	{
		ae=0;
	}
	x.i = am  | (ae<<23);
	return x.f;
}
float lshift(float f,int i)
{
	//TODO limit check and optimise
	__ffmath x;
	int am,ae;
	x.f=f;
	am=x.i & 0x807fffff ;
	ae=(x.i>>23)&0x000000ff;
	ae+=i;
	if(ae>255)ae=255;
	x.i = am | ((ae)<<23);
	return x.f;
}
float ffadd(float a, float b)
{
	__ffmath ai,bi,ret;
	int am,ae,bm,be,re,rm,shift,s;
	ai.f=a;
	bi.f=b;

	ae=(ai.i>>23)&0x000000ff;
	if(ae==0)am=0;
	else if(ai.i&0x80000000) am= -((ai.i & 0x007fffff )| 0x00800000);
	else am=(ai.i & 0x007fffff )| 0x00800000;

	be=(bi.i>>23)&0x000000ff;
	if(be==0)bm=0;
	else if(bi.i&0x80000000) bm= -((bi.i & 0x007fffff )| 0x00800000);
	else bm=(bi.i & 0x007fffff )| 0x00800000;

	if(ae>be)
	{
		s=ae-be;
		if(s>31)return a;
		re=ae;
		rm=am+(bm>>s);
	}
	else
	{
		s=be-ae;
		if(s>31)return b;
		re=be;
		rm=bm+(am>>(be-ae));
	}

//	normalise(&ret);
	if(rm==0)return 0.0f;
	s=rm;
	if(s<0)
	{
		s=-s;
	}
	if(s>=(1<<16))
	{
		if(s>=(1<<24))
		{
			shift=-(int)msbit2[s>>24];//+24;
		}
		else
		{
			shift=8-(int)msbit2[s>>16];//+16;
		}
	}
	else if(s>=(1<<8))
	{
		shift=16-(int)msbit2[s>>8];//+8;
	}
	else
	{
		shift=24-(int)msbit2[s];
	}
//	shift=24-shift;
	if(shift>0)rm<<=shift;
	else rm>>=-shift;
	re-=shift;

	if(rm>0)ret.i= (rm & 0x807fffff )| ((re)<<23);
	else ret.i= (-rm & 0x807fffff )| ((re)<<23) | 0x80000000;

	return ret.f;
}
float ffsub(float a, float b)
{
	__ffmath ai,bi,ret;
	int am,ae,bm,be,re,rm,shift,s;


	ai.f=a;
	bi.f=b;

	ae=(ai.i>>23)&0x000000ff;
	if(ae==0)am=0;
	else if(ai.i&0x80000000) am= -((ai.i & 0x007fffff )| 0x00800000);
	else am=(ai.i & 0x007fffff )| 0x00800000;

	be=(bi.i>>23)&0x000000ff;
	if(be==0)bm=0;
	else if(bi.i&0x80000000) bm= -((bi.i & 0x007fffff )| 0x00800000);
	else bm=(bi.i & 0x007fffff )| 0x00800000;


	if(ae>be)
	{
		s=ae-be;
		if(s>31)return a;
		re=ae;
		rm=am-(bm>>s);
	}
	else
	{
		s=be-ae;
		if(s>31)return -b;
		re=be;
		rm=(am>>s)-bm;
	}


//	normalise(&ret);
	if(rm==0)return 0.0f;
	s=rm;
	if(s<0)
	{
		s=-s;
	}
	if(s>=(1<<16))
	{
		if(s>=(1<<24))
		{
			shift=-(int)msbit2[s>>24];//+24;
		}
		else
		{
			shift=8-(int)msbit2[s>>16];//+16;
		}
	}
	else if(s>=(1<<8))
	{
		shift=16-(int)msbit2[s>>8];//+8;
	}
	else
	{
		shift=24-(int)msbit2[s];
	}
//	shift=24-shift;
	if(shift>0)rm<<=shift;
	else rm>>=-shift;
	re-=shift;


	if(rm>0)ret.i= (rm & 0x807fffff )| ((re)<<23);
	else ret.i= (-rm & 0x807fffff )| ((re)<<23) | 0x80000000;

	return ret.f;
}

float ffmul(float a, float b)
{

	__ffmath ai,bi,ret;
	int am,ae,bm,be,re,rm,shift,s;

	ai.f=a;
	bi.f=b;

	am=(ai.i & 0x007fffff )| 0x00800000;
	ae=(ai.i>>23)&0x000000ff;

	bm=(bi.i & 0x007fffff )| 0x00800000;
	be=(bi.i>>23)&0x000000ff;

	if(ae==0 || be==0)return 0.0f;


	rm=(am>>12)*(bm>>12)+(((am>>12)*(bm&0x00000fff)+(bm>>12)*(am&0x00000fff))>>12)+((am&0x00000fff)*(bm&0x00000fff)>>24);

	re=ae+be-126;

	if(rm==0)return 0.0f;
	s=rm;
	if(s<0)
	{
		s=-s;
	}
	if(s>=(1<<16))
	{
		if(s>=(1<<24))
		{
			shift=-(int)msbit2[s>>24];//+24;
		}
		else
		{
			shift=8-(int)msbit2[s>>16];//+16;
		}
	}
	else if(s>=(1<<8))
	{
		shift=16-(int)msbit2[s>>8];//+8;
	}
	else
	{
		shift=24-(int)msbit2[s];
	}
//	shift=24-shift;
	if(shift>0)rm<<=shift;
	else rm>>=-shift;
	re-=shift;

	ret.i= (rm & 0x807fffff )| ((re)<<23) | ((ai.i & 0x80000000) ^ (bi.i & 0x80000000)) ;

	return ret.f;
}
float ffmac(float a, float b,float c)
{

	__ffmath ai,bi,ci,ret;
	int am,ae,bm,be,cm,ce,re,rm,shift,s;

	ai.f=a;
	bi.f=b;
	ci.f=c;

	am=(ai.i & 0x007fffff )| 0x00800000;
	ae=(ai.i>>23)&0x000000ff;

	bm=(bi.i & 0x007fffff )| 0x00800000;
	be=(bi.i>>23)&0x000000ff;

	if(ae==0 || be==0)return c;


	rm=(am>>12)*(bm>>12)+(((am>>12)*(bm&0x00000fff)+(bm>>12)*(am&0x00000fff))>>12)+((am&0x00000fff)*(bm&0x00000fff)>>24);

	re=ae+be-126;

	if((ai.i & 0x80000000) ^ (bi.i & 0x80000000)) rm=-rm;

	//now add

	ce=(ci.i>>23)&0x000000ff;
	if(ce==0)cm=0;
	else if(ci.i&0x80000000) cm= -((ci.i & 0x007fffff )| 0x00800000);
	else cm=(ci.i & 0x007fffff )| 0x00800000;

	if(ce>re)
	{
		s=ce-re;
		if(s>31)return c;
		re=ce;
		rm=cm+(rm>>s);
	}
	else
	{
		s=re-ce;
		if(s<=31)rm=rm+(cm>>(re-ce));
	}

//	normalise(&ret);
	if(rm==0)return 0.0f;
	s=rm;
	if(s<0)
	{
		s=-s;
	}
	if(s>=(1<<16))
	{
		if(s>=(1<<24))
		{
			shift=-(int)msbit2[s>>24];//+24;
		}
		else
		{
			shift=8-(int)msbit2[s>>16];//+16;
		}
	}
	else if(s>=(1<<8))
	{
		shift=16-(int)msbit2[s>>8];//+8;
	}
	else
	{
		shift=24-(int)msbit2[s];
	}
//	shift=24-shift;
	if(shift>0)rm<<=shift;
	else rm>>=-shift;
	re-=shift;

	if(rm>0)ret.i= (rm & 0x807fffff )| ((re)<<23);
	else ret.i= (-rm & 0x807fffff )| ((re)<<23) | 0x80000000;

	return ret.f;
}

float ffdiv(float a, float b)
{

	__ffmath ai,bi,ret;
	int am,ae,bm,be,re,rm,shift,s;

	//TODO decide what to do about division by zero;
	if(a==0 || b==0) return 0.0f;

	ai.f=a;
	bi.f=b;

	am=(ai.i & 0x007fffff )| 0x00800000;
	ae=(ai.i>>23)&0x000000ff;

	bm=(bi.i & 0x007fffff) | 0x00800000;
	be=(bi.i>>23)&0x000000ff;

	re=(ae-be)+121;
	rm=((int64_t)am<<29)/(int64_t)bm ;

	if(rm==0)return 0.0f;

	s=rm;
	if(s<0)
	{
		s=-s;
	}
	if(s>=(1<<16))
	{
		if(s>=(1<<24))
		{
			shift=-(int)msbit2[s>>24];//+24;
		}
		else
		{
			shift=8-(int)msbit2[s>>16];//+16;
		}
	}
	else if(s>=(1<<8))
	{
		shift=16-(int)msbit2[s>>8];//+8;
	}
	else
	{
		shift=24-(int)msbit2[s];
	}
//	shift=24-shift;
	if(shift>0)rm<<=shift;
	else rm>>=-shift;
	re-=shift;

	ret.i= (rm & 0x807fffff )| ((re)<<23) | ((ai.i & 0x80000000) ^ (bi.i & 0x80000000)) ;

	return ret.f;
}

float ffrsqrt(float x)
{
	float x2;
	__ffmath y;
	const float threehalfs = 1.5f;

	x2 = ffmul(x, 0.5f);
	y.f  = x;
	y.i  = 0x5f375a86 - ( y.i >> 1 );
	y.f  = ffmul(y.f ,  ffsub(threehalfs , ffmul( x2 ,ffmul( y.f ,y.f ))));
	y.f  = ffmul(y.f ,  ffsub(threehalfs , ffmul( x2 ,ffmul( y.f ,y.f ))));

	return y.f;
}
float ffsqrt(float x)
{
	float x2;
	__ffmath y;
	const float threehalfs = 1.5f;

	x2 = ffmul(x, 0.5f);
	y.f  = x;
	y.i  = 0x5f375a86 - ( y.i >> 1 );
	y.f  = ffmul(y.f ,  ffsub(threehalfs , ffmul( x2 ,ffmul( y.f ,y.f ))));
	y.f  = ffmul(y.f ,  ffsub(threehalfs , ffmul( x2 ,ffmul( y.f ,y.f ))));

	return ffmul(y.f,x);
}

//http://www.flipcode.com/archives/Fast_log_Function.shtml
float fflog2 (float val)
{
	__ffmath ai;
	int x,log_2;
	ai.f=val;

   x = ai.i;
   log_2 = ((ai.i >> 23) & 255) - 128;
   ai.i &= ~(255 << 23);
   ai.i += 127 << 23;

   ai.f = ffsub(ffmul(ffadd(ffmul(-1.0f/3, ai.f) , 2) ,ai.f) , 2.0f/3);

   return ffadd(ai.f , log_2);
}

float fflog (float val)
{
   return ffmul(fflog2 (val) , 0.69314718f);
}
float ffatan2( float y, float x )
{
	float atan,z;
	if ( ffabs( x ) >ffabs( y ) )
	{
		if ( x == 0 )
		{
			if ( y > 0 ) return M_PI/2;
			if ( y == 0 ) return 0;
			return -M_PI/2;
		}
		z = ffdiv(y,x);
		atan = ffmul(z,ffadd((M_PI/4)+0.273,ffmul(-0.273,ffabs(z))));
		if ( x < 0 )
		{
			if ( y < 0 ) return atan - M_PI;
			return atan + M_PI;
		}
	}
	else
	{
		if ( y == 0 )
		{
			if ( x > 0 ) return 0;
			if ( x == 0 ) return 0;
			return M_PI;
		}
		z = ffdiv(x,y);
		atan = ffsub(M_PI/2,ffmul(z,ffadd((M_PI/4)+0.273,ffmul(-0.273,ffabs(z)))));
		if ( y < 0 )
		{
			return atan - M_PI;
		}
	}
	return atan;
}
float ffacos(float x)
{
	return ffatan2(ffsqrt(ffmul(ffadd(1,x) , ffsub(1, x))),x);
}

vector3f v_cross(vector3f* a, vector3f* b)
{
	vector3f r;
	r.x=ffsub(ffmul(a->y,b->z),ffmul(a->z,b->y));
	r.y=ffsub(ffmul(a->z,b->x),ffmul(a->x,b->z));
	r.z=ffsub(ffmul(a->x,b->y),ffmul(a->y,b->x));
	return r;
}
void v_normalize(vector3f* a)
{
	float rmag=ffrsqrt(ffadd(ffadd(ffmul(a->x,a->x),ffmul(a->y,a->y)),ffmul(a->z,a->z)));
	a->x=ffmul(a->x,rmag);
	a->y=ffmul(a->y,rmag);
	a->z=ffmul(a->z,rmag);
}
vector3f v_rotate(vector3f* a,quaternionf* rot)
{
	//might be better to convert to rotation matrix
	quaternionf t,tt,invrot;
	vector3f ret;
	t.w=0;
	t.x=a->x;
	t.y=a->y;
	t.z=a->z;
	invrot=q_invert(rot);

	q_mul(rot,&t,&tt);
	q_mul(&tt,&invrot,&t);

	ret.x=t.x;
	ret.y=t.y;
	ret.z=t.z;
	return ret;
}

void q_mulold(quaternionf* a,quaternionf* b,quaternionf* c)
{
	c->w = ffsub(ffsub(ffsub(ffmul(a->w,b->w) , ffmul(a->x,b->x)) , ffmul(a->y,b->y)) , ffmul(a->z,b->z));
	c->x = ffsub(ffadd(ffadd(ffmul(a->w,b->x) , ffmul(a->x,b->w)) , ffmul(a->y,b->z)) , ffmul(a->z,b->y));
	c->y = ffadd(ffadd(ffsub(ffmul(a->w,b->y) , ffmul(a->x,b->z)) , ffmul(a->y,b->w)) , ffmul(a->z,b->x));
	c->z = ffadd(ffsub(ffadd(ffmul(a->w,b->z) , ffmul(a->x,b->y)) , ffmul(a->y,b->x)) , ffmul(a->z,b->w));
}
#define fmac4(a,b,c,d,e,f,g,h)\
ffmac(g,h,ffmac(e,f,ffmac(c,d,ffmul(a,b))))\
\

void q_mul(quaternionf* a,quaternionf* b,quaternionf* c)
{
	c->w = fmac4(a->w,b->w , -a->x,b->x , -a->y,b->y , -a->z,b->z);
	c->x = fmac4(a->w,b->x ,  a->x,b->w ,  a->y,b->z , -a->z,b->y);
	c->y = fmac4(a->w,b->y , -a->x,b->z ,  a->y,b->w ,  a->z,b->x);
	c->z = fmac4(a->w,b->z ,  a->x,b->y , -a->y,b->x ,  a->z,b->w);
}
void q_normalize(quaternionf* a)
{
	float rmag=ffrsqrt(ffadd(ffadd(ffadd(ffmul(a->x,a->x),ffmul(a->y,a->y)),ffmul(a->z,a->z)),ffmul(a->w,a->w)));
	a->w=ffmul(a->w,rmag);
	a->x=ffmul(a->x,rmag);
	a->y=ffmul(a->y,rmag);
	a->z=ffmul(a->z,rmag);
}
quaternionf q_invert(quaternionf* a)
{
	quaternionf r;
	r.w=a->w;
	r.x=-a->x;
	r.y=-a->y;
	r.z=-a->z;
	return r;
}
void q_rotate(quaternionf* q,float ax, float ay,float az)
{
	quaternionf rot,ret;
	float halfAngle;
	//calculate axis and angle
	float angle=ffsqrt(ffadd(ffadd(ffmul(ax,ax),ffmul(ay,ay)),ffmul(az,az)));
	if(angle==0)return;
	halfAngle=rshift(angle,1);

	rot.w=ffsub(1.0f,rshift(ffmul(halfAngle,halfAngle),1));
	rot.x=rshift(ax,1);
	rot.y=rshift(ay,1);
	rot.z=rshift(az,1);

	q_mul(&rot,q,&ret);
	*q=ret;
}
quaternionf q_FromVectors(vector3f* a,vector3f* b,vector3f* c)
{
	quaternionf q;
	if ( ffadd(ffadd(a->x , b->y) , c->z) > 0.0f )
	{
 		 float t = ffadd(ffadd(ffadd(a->x , b->y) ,c->z) , 1.0f);
		 float s = rshift(ffrsqrt( t ),1);

		 q.w = ffmul(s , t);
		 q.z = ffmul(ffsub( b->x , a->y ) , s);
		 q.y = ffmul(ffsub( a->z , c->x ) , s);
		 q.x = ffmul(ffsub( c->y , b->z ) , s);
	}
	else if (a->x > b->y && a->x > c->z )
	{
 		 float t =  ffadd(ffsub(ffsub(a->x , b->y) , c->z) , 1.0f);
		 float s = rshift(ffrsqrt( t ),1);

		 q.x = ffmul(s , t);
		 q.y = ffmul(ffadd( b->x , a->y ) , s);
		 q.z = ffmul(ffadd( a->z , c->x ) , s);
		 q.w = ffmul(ffsub( c->y , b->z ) , s);
	}
	else if (b->y > c->z )
	{
 		 float t = ffadd(ffsub(ffsub( b->y , a->x)  , c->z) , 1.0f);
		 float s = rshift(ffrsqrt( t ),1);

		 q.y = ffmul(s , t);
		 q.x = ffmul(ffadd( b->x , a->y ) , s);
		 q.w = ffmul(ffsub( a->z , c->x ) , s);
		 q.z = ffmul(ffadd( c->y , b->z ) , s);
	}
	else
	{
 		 float t = + ffadd(ffsub(ffsub(c->z , a->x) , b->y)  , 1.0f);
		 float s = rshift(ffrsqrt( t ),1);

		 q.z = ffmul(s , t);
		 q.w = ffmul(ffsub( b->x , a->y ) , s);
		 q.x = ffmul(ffadd( a->z , c->x ) , s);
		 q.y = ffmul(ffadd( c->y , b->z ) , s);
	 }
	return q;
 }


quaternionf q_FromGM(vector3f* acc,vector3f* mag)
{
	vector3f accc=*acc;
	vector3f side=v_cross(&accc,mag);
	vector3f north=v_cross(&side,acc);
	quaternionf correct;

	v_normalize(&accc);
	v_normalize(&north);
	v_normalize(&side);

	//now turn rotation matrix into quaternion

	correct = q_FromVectors(&north,&side,&accc);

	q_normalize(&correct);
//	return correct;
	return q_invert(&correct);
}
void q_lowpass(quaternionf* prev,quaternionf* n,float t)
{
	quaternionf ret;
	float ti=ffsub(1.0f,t);
	float dot= ffadd(ffadd(ffadd(ffmul(prev->x , n->x) , ffmul(prev->y , n->y)) , ffmul(prev->z , n->z)) , ffmul(prev->w , n->w));

	if(dot<0)
	{
		prev->w=-prev->w;
		prev->x=-prev->x;
		prev->y=-prev->y;
		prev->z=-prev->z;
	}

	ret.w=ffadd(ffmul(ti,prev->w) , ffmul(t,n->w));
	ret.x=ffadd(ffmul(ti,prev->x) , ffmul(t,n->x));
	ret.y=ffadd(ffmul(ti,prev->y) , ffmul(t,n->y));
	ret.z=ffadd(ffmul(ti,prev->z) , ffmul(t,n->z));

	*prev=ret;
}
quaternioni q_toInt(quaternionf* q,int shift)
{
	quaternioni r;
	r.w=lshift(q->w,shift);
	r.x=lshift(q->x,shift);
	r.y=lshift(q->y,shift);
	r.z=lshift(q->z,shift);
	return r;
}

