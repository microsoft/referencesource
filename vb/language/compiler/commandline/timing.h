// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
// ===========================================================================
// File: timing.h
//
// Defined the timer functions, which allow reporting performance
// information for the compiler. This is sort of a built-in mini-profiler
// that is always available. This allows anyone to do a quick investigation of
// performance problems and try to pinpoint where things have changed.
// <



#ifndef __timing_h__
#define __timing_h__

// Create the TIMERID enum. To add a new timerid, edit the timerids.h file.
#define TIMER_GROUP(x, y)
#define TIMERID(id, text, subtotal) id,
enum TIMERID {
#include "timerids.h"
    TIMERID_MAX
};
#undef TIMERID
#undef TIMER_GROUP



// Although it is verboten in most parts of the compiler, we use static
// data in the timings module. This means that the timings will
// not be correct if multiple compiles happen at the same time. Since
// this is an internal debugging tool anyway, that's OK for this
// one specific thing. It's important to be static because we want this
// to be very low overhead if it's not in use.


// Is timing on?
extern bool g_isTimingActive;

// The timer functions.
extern void ActivateTiming();
extern void FinishTiming();
extern void DoTimerStart(TIMERID timerId);
extern void DoTimerStop(TIMERID timerId);

// Begin timing something.
__forceinline void TimerStart(TIMERID timerId)
{
    if (g_isTimingActive)
        DoTimerStart(timerId);
}

// Finish timing something
__forceinline void TimerStop(TIMERID timerId)
{
    if (g_isTimingActive)
        DoTimerStop(timerId);
}

// class to time a block of code
class TIMERBLOCK
{
public:
    TIMERBLOCK(TIMERID timerId) : timerId(timerId)
    {
        TimerStart(timerId);
    }
    ~TIMERBLOCK()
    {
        TimerStop(timerId);
    }
private:
    TIMERID timerId;
};

#if IDE || IDE64
#define TIMEBLOCK(timerId) 
#else
#define TIMEBLOCK(timerId) TIMERBLOCK __timerId(timerId)
#endif

#ifndef CSEE 

void ReportTimes(FILE * outputFile);
void ReportTimesInXML(FILE * outputFile);

// pass in "stdout" to output to console. Otherwise, specified file is opened for append.
void ReportTimesInXML(PCWSTR outputFile);

#else   //CSEE

extern __int64 GetCurrentTimerTickM();
extern void InitializeTimerTick();

class TimerGuard
{
public:
    TimerGuard(_In_ WCHAR * outputfile, _In_ char * file, unsigned line) : outputfile(outputfile), file(file), line(line), string(NULL)
    {
        Start();
    }
    ~TimerGuard()
    {
        Stop();
    }
    void SetData(const WCHAR * str)
    {
        string = str;
    }
private:
    char * file;
    unsigned line;
    __int64 beginTime;
    const WCHAR * string;
    WCHAR * outputfile;
    void Start()
    {
        beginTime = GetCurrentTimerTickM();
    }
    void Dump(__int64 e);
    void Stop()
    {
        __int64 elapsed = GetCurrentTimerTickM() - beginTime;
        Dump(elapsed);
    }
};

#ifdef EETIMING
#define TIMEEEBLOCK TimerGuard  __tguard(L"eetimings.txt", __FILE__, __LINE__);
#define TIMEINFO(s)   __tguard.SetData(s);
#else
#define TIMEEEBLOCK
#define TIMEINFO(s)
#endif


#endif



#endif // __timing_h__
