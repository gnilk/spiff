/*-------------------------------------------------------------------------
	File    :	Timer.cpp
	Author  :	Fredrik KLing
	EMail   :	fredrik.kling@home.se
	Orginal :	02.11.2006
	Descr   :	Implements various common helper routines
			Also contains some OS abstraction stuff (timers)
				
 
   Note: Code kindly donated by FKling 
	    [sux not having enough time have to harvest your private code base]
 
--------------------------------------------------------------------------- 
    Todo [-:undone,+:inprogress,!:done]:
	
Changes: 

-- Date -- | -- Name ------- | -- Did what...
22.02.2009 | FKling          | Moved to SqNXCore [was SqNXFramework]
27.01.2006 | FKling          | Imported from private stack of lame code
06.11.2006 | FKling          | Linux/Unix timing handling fixed..
02.11.2006 | FKling          | Implementation

---------------------------------------------------------------------------*/
#ifdef WIN32
#define WIN32_LEAN_MEAN
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#endif

#include <math.h>

#include "Timer.h"

using namespace Utils;

//////////////////////////////////////////////////////////////////////////
//
// Implement a timer, currently Unix and Windows support
//
Timer::Timer()
{
#ifdef WIN32
	LARGE_INTEGER lif;
	double tmp;

	QueryPerformanceFrequency(&lif);
	freq = (double)lif.QuadPart;
	QueryPerformanceCounter(&lif);
	tmp = (double)lif.QuadPart;
	tLast = ( tmp) / freq;
#else
	// assume all other OS:es atleast have "clock" since it is ISO
	//clock_t now; 
	//now = clock();
	//tLast = ((double)now) / CLOCKS_PER_SEC;
	double tmp;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	tmp = (double)tv.tv_usec;
	tmp = tmp / (1000.0f * 1000.0f);
	tmp += (double)(tv.tv_sec);
	tInternal = tmp;
#endif

	tInternal = tLast;
	tStart = tLast;
	dtInternal = 0;
	fps_counter = 0;
	fps_tmp = 0;
	fps_avg_current = 0;
}

void Timer::Dispose() {
	delete this;
}

//////////////////////////////////////////////////////////////////////////
/**
	\brief	Sample the timer
	
	Sample the timer and calculates the delta's between last
	sample and accumulated FPS. You should call this one each
	time you render something.
	
	On windows we use the performance counter (without care to
	thread affinity, might resolve in strange behaviour on multi-core
	or multi-processor machines)
	
	Unix/Linux uses "clock", which hopefully works all over...
	
	\param	dt	Delta time, between calls, NULL per default
	
	\retval	double	Current time, in seconds, 1.0 = one second
*/
double Timer::Sample(double *dt)
{
	tLast = tInternal;
		
	//
	// Perform OS dependent stuff
	//	
#ifdef WIN32
	//
	// On window, use performance counter, don't care about
	// thread affinity
	//
	LARGE_INTEGER lif;
	double tmp;
	QueryPerformanceCounter(&lif);
	tmp = (double)lif.QuadPart;
	tInternal = (tmp) / freq;
#else
	// 
	// Unix/Linux is assumed to have gettimeofday
	//
	struct timeval tv;
	double tmp;
	
	gettimeofday(&tv,NULL);
	tmp = (double)tv.tv_usec;
	// This is ISO
	// tmp = tmp / (1000.0 * 1000.0);	// Unix spec SysV2 defines it as 1000000
	tmp = tmp / ((double)CLOCKS_PER_SEC);	// ISO defines it as CLOCKS_PER_SEC, which is what FreeBSD conforms to
	tmp += (double)(tv.tv_sec);
	tInternal = tmp;
#endif
	//
	// same code, regardless of OS
	//
	dtInternal = tInternal - tLast;
	if (dt != NULL) 
		*dt = dtInternal;

	fps_tmp += 1.0 / dtInternal;
	fps_counter++;
	if (fps_counter > 10)
	{
		fps_avg_current = fps_tmp / 10.0;
		fps_tmp=0;
		fps_counter=0;
	}
	return tInternal;
}

//////////////////////////////////////////////////////////////////////////
/**
	\brief	Returns the average FPS
	
	\retval	double	the average FPS
*/
double Timer::GetFPSAverage()
{
	return fps_avg_current;
}

//////////////////////////////////////////////////////////////////////////
/**
	\brief	Returns the absolute FPS
	
	This returns the realtime FPS calculated between
	call's to the sample function. You can use this if you
	want to accumulate and average it your self.
	
	\retval double	the current FPS
*/
double Timer::GetFPS()
{
	double fps;
	fps = 1.0 / (tInternal - tLast);
	return fps;
}

//////////////////////////////////////////////////////////////////////////
/**
	\brief	Reset the timer
	
	Just reset the the timer
*/
void Timer::Reset()
{
	// do nothing...
	tStart = tLast;
}


