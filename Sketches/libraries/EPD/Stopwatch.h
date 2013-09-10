//////////////////////////////////////////////////////////////////////////
// FILE: Stopwatch.h
// DESC: Implementation of CStopwatch class, to measure C++ code
//       performances.
//////////////////////////////////////////////////////////////////////////

#pragma once
#include <Arduino.h>


using namespace std;

//Brody Kenrick borrowed from: |||
//                             VVV
//========================================================================
// CStopwatch
//
// Class used to measure performances of C++ code.
// (This class uses high-resolution performance counters.)
//
// By Giovanni Dicanio <giovanni.dicanio@gmail.com>
//
// 2010, January 11th
//
// ----------------------------------------------------------------------
//
// To use this class, follow this pattern:
//
// 1. Create an instance of the class nearby the code you want to measure.
//
// 2. Call Start() method immediately before the code to measure.
//
// 3. Call Stop() method immediately after the code to measure.
//
// 4. Call ElapsedTimeSec() or ElapsedTimeMillisec() methods to get the
//    elapsed time (in seconds or milliseconds, respectively).
//
//
//========================================================================
class CStopwatch
{
public:

    // Does some initialization to get consistent results for all tests.
    CStopwatch( String name )
        : m_name(name), m_startCount(0), m_elapsedTimeMilliSec(0), m_eventsRecorded(0)
    {
    }

    // Starts measuring performance
    // (to be called before the block of code to measure).
    void Start()
    {
        // Start ticking
        m_startCount = millis();
    }

    // Stops measuring performance
    // (to be called after the block of code to measure).
    void Stop()
    {
        // Stop ticking
        long stopCount = millis();

        // Calculate total elapsed time since Start() was called;
        // add onto accumulated time
        m_elapsedTimeMilliSec += (stopCount - m_startCount);

        // Clear start count (it is spurious information)
        m_startCount = 0;
        
        m_eventsRecorded++;
    }

    void SerialPrint()
    {
        Serial.print( "Duration of\t" );
        Serial.print( m_name );
        Serial.print( " is \t" );
        Serial.print( ElapsedTimeMilliSec() );
        Serial.print( " ms  in\t" );
        Serial.print( EventsRecorded() );
        Serial.print( " events avg =\t" );
        Serial.print( AverageTimeMilliSec() );
        Serial.println( " ms)." );
    }

    // Returns total elapsed time (in milliseconds) in the Start-Stop intervals.
    long ElapsedTimeMilliSec() const
    {
        return m_elapsedTimeMilliSec;
    }

    long AverageTimeMilliSec() const
    {
        if(m_eventsRecorded != 0)
        {
            return m_elapsedTimeMilliSec/m_eventsRecorded;
        }
        else
        {
            return 0;
        }
    }

    // Returns total number of stops (and starts if they are equal....).
    long EventsRecorded() const
    {
        return m_eventsRecorded;
    }

    //--------------------------------------------------------------------
    // IMPLEMENTATION
    //--------------------------------------------------------------------
private:

    //
    // *** Data Members ***
    //

    String m_name;

    // The value of counter on start ticking
    long m_startCount;
    
    // The time (in seconds) elapsed in Start-Stop interval
    long m_elapsedTimeMilliSec;
    
    //How many times we started/stopped
    long m_eventsRecorded;


    //
    // *** Ban copy ***
    //
private:
    CStopwatch(const CStopwatch &);
    CStopwatch & operator=(const CStopwatch &);
};
