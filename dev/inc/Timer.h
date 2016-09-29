#pragma once

#include <stdlib.h>
#include <stdio.h>

namespace Utils
{
	class ITimer {
	public:
		virtual double Sample(double *dt=NULL) = 0;
		virtual void Reset() = 0;
		virtual void Dispose() = 0;
	};

	class Timer : public ITimer
	{
	protected:
	#ifdef WIN32
		double freq, tVal;
	#endif
		double tInternal;
		double tLast;
		double dtInternal;
		double tStart;
		double fps_tmp;
		double fps_avg_current;
		int fps_counter;
	public:
		Timer();
		virtual double Sample(double *dt=NULL);
		virtual void Reset();	
		virtual void Dispose();
		double GetFPS();
		double GetFPSAverage();
	};
}



