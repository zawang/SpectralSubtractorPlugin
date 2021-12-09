#pragma once

#include "JuceHeader.h"
#define FloatType float

void getAudioFile (juce::AudioBuffer<FloatType>& buffer, const juce::File& file)
{
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader;
    reader.reset (formatManager.createReaderFor (file));
    if (reader.get() != nullptr)
    {
        buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), 0, buffer.getNumSamples());
    }
}

/** @see Same as appendToFile() in juce_PerformanceCounter.cpp */
static void appendToFile (const juce::File& f, const juce::String& s)
{
    if (f.getFullPathName().isNotEmpty())
    {
        juce::FileOutputStream out (f);

        if (! out.failedToOpen())
            out << s << newLine;
    }
}

/** Based off of juce::PerformanceCounter. */
class PerformanceProfiler
{
public:
    /** Same as juce::PerformanceCounter constructor. */
    PerformanceProfiler (const juce::String& name, int runsPerPrintout, const juce::File& loggingFile)
        : runsPerPrint (runsPerPrintout), startTime (0), outputFile (loggingFile)
    {
        stats.name = name;
        appendToFile (outputFile, "**** Counter for \"" + name + "\" started at: " + Time::getCurrentTime().toString (true, true));
    }

    /** Same as juce::PerformanceCounter destructor. */
    ~PerformanceProfiler()
    {
        if (stats.numRuns > 0)
            printStatisticsAndReset();
    }

    /** Same as juce::PerformanceCounter::start(). */
    void start() noexcept
    {
        startTime = juce::Time::getHighResolutionTicks();
    }

    /** Similar to juce::PerformanceCounter::stop(). */
    bool stop()
    {
        stats.addResult (juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - startTime));

        if (stats.numRuns < runsPerPrint)
            return false;

        calculateAverageSeconds();
        return true;
    }
    
    double getAverageSeconds()
    {
        return stats.averageSeconds;
    }

    /** Similar to juce::PerformanceCounter::printStatistics(). */
    void printStatisticsAndReset()
    {
        const juce::String desc (getStatisticsAndReset().toString());

        juce::Logger::writeToLog (desc);
        appendToFile (outputFile, desc);
    }

private:
    //==============================================================================
    juce::PerformanceCounter::Statistics stats;
    juce::int64 runsPerPrint, startTime;
    juce::File outputFile;
    
    void calculateAverageSeconds()
    {
        if (stats.numRuns > 0)
            stats.averageSeconds = stats.totalSeconds / (float) stats.numRuns;
        else
            jassertfalse;
    }
    
    /** Similar to juce::PerformanceCounter::getStatisticsAndReset(). */
    juce::PerformanceCounter::Statistics getStatisticsAndReset()
    {
        juce::PerformanceCounter::Statistics s (stats);
        stats.clear();

        return s;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformanceProfiler)
};
