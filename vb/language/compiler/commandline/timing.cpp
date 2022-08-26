// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
// ===========================================================================
// File: timing.cpp
//
// Defined the timer functions, which allow reporting performance
// information for the compiler. This is sort of a built-in mini-profiler
// that is always available. This allows anyone to do a quick investigation of
// performance problems and try to pinpoint where things have changed.
// <



#include "stdafx.h"

// Although it is verboten in most parts of the compiler, we use static
// data in the timings module. This means that the timings will
// not be correct if multiple compiles happen at the same time. Since
// this is an internal debugging tool anyway, that's OK for this
// one specific thing. It's important to be static because we want this
// to be very low overhead if it's not in use.

bool g_isTimingActive = false;

LARGE_INTEGER g_qpcStartTime;  // In units returns by QueryPerformanceCounter
LARGE_INTEGER g_qpcStopTime;

__int64 g_startTime;            // In units returned by GetTickCountrTick
__int64 g_lastTime;
__int64 g_stopTime;
TIMERID g_timeridStack[1000];
int g_timeridStackPtr; // points to top USED value on stack, or -1 if stack is empty.

struct TIMERSECTIONINFO
{
    PWSTR name;
    int subTotal;
};

struct TIMERSECTIONDATA
{
    unsigned totalCount;
    __int64 totalTime;
};

TIMERSECTIONDATA g_timerData[TIMERID_MAX];

#define TIMER_GROUP(cat, name)
#define TIMERID(id, text, subtotal) { L##text, subtotal} ,
const TIMERSECTIONINFO g_timerInfo[TIMERID_MAX] =
    {
#include "timerids.h"
    };
#undef TIMERID
#undef TIMER_GROUP

struct TimerGroupName
{
    int m_group_id;
    PCWSTR m_group_name;
};

const TimerGroupName g_timergroups[] =
    {
#define TIMERID(id, text, group)
#define TIMER_GROUP(cat, name) { cat, L##name },
#include "timerids.h"
#undef TIMER_GROUP
#undef TIMERID
    };


// QueryPerformanceCounter is slow enough that is skews the
// timing somewhat. The rdtsc (read cycle counter) instruction
// is much better. We have to restrict execution to a single
// processor to make it reliable, though.
#if defined(_M_IX86) && !defined(FEATURE_PAL)

#pragma warning(disable: 4035)  // no return value
__forceinline __int64 GetCurrentTimerTick()
{
    __asm rdtsc
}
#pragma warning(default: 4035)

typedef BOOL (WINAPI *PFNSETPROCESSAFFINITYMASK)(HANDLE, DWORD_PTR);

void InitializeTimerTick()
{
    // SetProcessAffinityMask not available on Win95.
    PFNSETPROCESSAFFINITYMASK pfnSetProcessAffinityMask =
        (PFNSETPROCESSAFFINITYMASK)GetProcAddress (GetModuleHandleW(L"kernel32.dll"), "SetProcessAffinityMask");

    // Set to only run on one processor so that they processor time count is accurate.
    if (pfnSetProcessAffinityMask != NULL)
        pfnSetProcessAffinityMask(GetCurrentProcess(), 1);
}

#else //_M_IX86 && !FEATURE_PAL

__forceinline __int64 GetCurrentTimerTick()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

// Initialize the counter, if needed.
void InitializeTimerTick()
{}

#endif //_M_IX86 && !FEATURE_PAL

__int64 GetCurrentTimerTickM()
{
    return GetCurrentTimerTick();
}

/*
 * Start the timing and reset all timer counts.
 */
void ActivateTiming()
{

    for (TIMERID id = (TIMERID) 0; id < TIMERID_MAX; id = (TIMERID) (id + 1))
    {
        g_timerData[id].totalCount = 0;
        g_timerData[id].totalTime = 0;
    }

    InitializeTimerTick();
    g_stopTime = 0;
    g_timeridStackPtr = -1;
    g_isTimingActive = true;
    QueryPerformanceCounter(&g_qpcStartTime);
    g_lastTime = g_startTime = GetCurrentTimerTick();
}

/*
 * Stop the timing.
 */
void FinishTiming()
{
    g_stopTime = GetCurrentTimerTick();
    QueryPerformanceCounter(&g_qpcStopTime);
    g_isTimingActive = false;
}


#pragma optimize("", off) // Otherwise the compiler ----s up stack walking in retail and confuses Watson
 __declspec(noreturn) inline void OnCriticalInternalError()
{
      // Force an AV.
      *((volatile int*)0);
}


/*
 * Record the start of a new section of timing
 */
void DoTimerStart(TIMERID timerId)
{
    __int64 now = GetCurrentTimerTick();
    int stackPtr = g_timeridStackPtr;
    TIMERID oldId;

    // Record the amount of time so far in the containing section, if any.
    if (stackPtr >= 0)
    {
        oldId = g_timeridStack[stackPtr];
        g_timerData[oldId].totalTime += (now - g_lastTime);
    }

    // update the stack

    int newStackPtr = stackPtr + 1;
#ifdef DEBUG
    ASSERT(newStackPtr >= 0, "in timing.cpp");
#else // !DEBUG
    (!(newStackPtr >= 0) ? OnCriticalInternalError() : 0);
#endif

#define lengthof(a) (sizeof(a) / sizeof((a)[0]))

    if (newStackPtr < (int)lengthof(g_timeridStack))
    {
        ++stackPtr;
        g_timeridStack[stackPtr] = timerId;
        g_timeridStackPtr = stackPtr;
    }
    else
    {
        ASSERT(false, "too many timer ids on the stack.");  // probably a logic error if this is hit.
    }

    // update the count and remember when we started.
    ++g_timerData[timerId].totalCount;
    g_lastTime = now;
}


/*
 * Record the end of a section of timing
 */
void DoTimerStop(TIMERID timerId)
{
    __int64 now = GetCurrentTimerTick();
    int stackPtr = g_timeridStackPtr;

    // Pop the stack. If the id doesn't match, pop until it does (exception thrown, maybe?)
    while (stackPtr > 0 && g_timeridStack[stackPtr] != timerId)
    {
        --stackPtr;
    }
    ASSERT(stackPtr >= 0 && g_timeridStack[stackPtr] == timerId, "in timing.cpp");  // if this is hit, we never found our timer id. Probably logic bug.
    --stackPtr;

    // Record the amount of time so far in the current section, if any.
    g_timerData[timerId].totalTime += (now - g_lastTime);
    g_timeridStackPtr = stackPtr;

    // remember the new time
    g_lastTime = now;
}

#ifndef CSEE

/* 

void ReportTimesInXML (PCWSTR fname)
{
    if (!_wcsicmp (fname, L"stdout"))
        ReportTimesInXML (stdout);
    else
    {
        FILE* f = NULL;
        if (!_wfopen_s (&f, fname, L"a+"))
        {
            ReportTimesInXML (f);
            fclose (f);
        }
    }
}

void ReportTimesInXML (FILE* file)
{
    fwprintf (file, PerfXML::s_RootTag);

    SYSTEM_INFO sinfo;
    memset (&sinfo, 0, sizeof (sinfo));
    GetSystemInfo (&sinfo);
    const wchar_t* arch = L"?";
    switch (sinfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            arch = L"Intel";
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            arch = L"IA64";
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            arch = L"AMD64";
            break;
    }

    MEMORYSTATUS minfo;
    memset (&minfo, 0, sizeof (minfo));
    GlobalMemoryStatus (&minfo);

    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(& qpcFreq);

    fwprintf (file, L"<SystemInfo ProcessorArchitecture=\"%s\" ProcessorModel=\"%d\" RAM=\"%d\" TimerFrequencyKHz=\"%I64u\"/>", arch, sinfo.wProcessorLevel, minfo.dwTotalPhys / (1024 * 1024), qpcFreq.QuadPart / 1000);

    int idSection = 0;
    bool section_started = false;

    for (TIMERID id = (TIMERID)0; id < TIMERID_MAX; id = (TIMERID) (id + 1))
    {
        if (g_timerData[id].totalCount != 0)
        {
            if (!section_started)
            {
                section_started = true;
                // find the name of this section and write it.
                const TimerGroupName* tgroup = g_timergroups;
                while (tgroup->m_group_id != LAST_TIMER_GROUP_ID)
                {
                    if (tgroup->m_group_id == idSection)
                    {
                        fwprintf (file, L"<TimedSection Name=\"%s\">\n", tgroup->m_group_name);
                        break;
                    }
                    else
                        tgroup++;
                }
            }

            fwprintf (file, L"<CodeTimeBlock CodeSectionName=\"%s\" Hits=\"%d\" Time=\"%I64u\"/>\n", g_timerInfo[id].name, g_timerData[id].totalCount, g_timerData[id].totalTime);
        }

#pragma warning(suppress: 6200)
        if (id + 1 >= TIMERID_MAX || g_timerInfo[id + 1].subTotal != g_timerInfo[id].subTotal)
        {
            idSection++;

            if (section_started)
            {
                section_started = false;
                fwprintf (file, L"</TimedSection>\n");
            }
        }
    }
    fwprintf (file, PerfXML::s_RootEndTag);
}
*/

/*
 * Print a report of the times spent in each timed section to a file.
 */
void ReportTimes(FILE * outputFile)
{
    double elapsedTime = (double)(g_stopTime - g_startTime);

    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(& qpcFreq);

    double elapsedTimeMsec = (double)(g_qpcStopTime.QuadPart - g_qpcStartTime.QuadPart) / (double) qpcFreq.QuadPart * 1000.0;
    __int64 subTotal;
    __int64 total;

    fwprintf(outputFile, L"All times are mutually exclusive. Total compile time: %.1f ms.\n", elapsedTimeMsec);
    fwprintf(outputFile, L"\n");

    subTotal = 0;
    total = 0;

    fwprintf(outputFile, L"%-50s  %10s  %7s\n", L"Name of code section", L"Hits", L"  Time %");
    fwprintf(outputFile, L"==========================================================================\n");
    for (TIMERID id = (TIMERID)0; id < TIMERID_MAX; id = (TIMERID) (id + 1))
    {
        if (g_timerData[id].totalCount != 0)
        {
            fwprintf(outputFile, L"%-50s  %10d  %7.3f%%\n", g_timerInfo[id].name,
                     g_timerData[id].totalCount,
                     (double) g_timerData[id].totalTime / elapsedTime * 100.0);
        }

        subTotal += g_timerData[id].totalTime;
        total += g_timerData[id].totalTime;

#pragma warning(suppress: 6200)
        if (id + 1 >= TIMERID_MAX || g_timerInfo[id + 1].subTotal != g_timerInfo[id].subTotal)
        {
            // print subtotal
            if (subTotal > 0)
            {
                fwprintf(outputFile, L"--------------------------------------------------------------------------\n");
                fwprintf(outputFile, L"%-50s  %10s  %7.3f%%\n\n", L"SUBTOTAL", L"",
                         (double) subTotal / elapsedTime * 100.0);
            }

            subTotal = 0;
        }
    }

    // print total
    fwprintf(outputFile, L"--------------------------------------------------------------------------\n");
    fwprintf(outputFile, L"%-50s  %10s  %7.3f%%\n\n", L"TOTAL OF TIMED SECTIONS", L"",
             (double) total / elapsedTime * 100.0);
}
#endif

#ifdef CSEE
void TimerGuard::Dump(__int64 e)
{
    FILE *f = NULL;
    if (!_wfopen_s(&f, outputfile, L"a"))
    {
        ASSERT(false);
        return ;
    }

    fprintf(f, "%s %d", file, line);
    if (string)
    {
        fwprintf(f, L" %s", string);
    }
    fprintf(f, " %I64d\n", e);
    fclose(f);
};
#endif




