/*
  ==============================================================================

    ProcessingNoiseSpectrumThread.h
    Created: 4 Oct 2021 7:59:13am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"

class ProcessingNoiseSpectrumThread : public juce::ThreadWithProgressWindow
{
public:
    ProcessingNoiseSpectrumThread(SpectralSubtractorAudioProcessor* inProcessor, juce::File inFile)
        : juce::ThreadWithProgressWindow ("Loading noise spectrum...", true, true),
          mProcessor (inProcessor),
          mNoiseFile (inFile)
    {
        setStatusMessage ("Getting ready...");
        mFormatManager.registerBasicFormats();
    }

    void run() override
    {
        mProcessor->suspendProcessing (true);
        
        setProgress (-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar...
        setStatusMessage ("Reading noise file...");
          
        if (mNoiseFile != juce::File{})
        {
            // Read the file
            mReader.reset (mFormatManager.createReaderFor (mNoiseFile));
            if (mReader.get() != nullptr)
            {
                mProcessor->calcAndStoreNoiseSpectrum(mReader.get());
            }
            else
            {
                mErrorLoadingFile = true;
                return;
            }
        }
    
        /*wait (2000);

        int thingsToDo = 10;

        for (int i = 0; i < thingsToDo; ++i)
        {
            // must check this as often as possible, because this is
            // how we know if the user's pressed 'cancel'
            if (threadShouldExit())
                return;

            // this will update the progress bar on the dialog box
            setProgress (i / (double) thingsToDo);

            setStatusMessage (String (thingsToDo - i) + " things left to do...");

            wait (500);
        }

        setProgress (-1.0); // setting a value beyond the range 0 -> 1 will show a spinning bar..
        setStatusMessage ("Finishing off the last few bits and pieces!");
        wait (2000);*/
        
        mProcessor->suspendProcessing(false);
    }

    // This method gets called on the message thread once our thread has finished..
    void threadComplete (bool userPressedCancel) override
    {
        juce::String messageString;
        if (userPressedCancel)
            messageString = "You pressed cancel!";
        else if (mErrorLoadingFile)
            messageString = juce::String("Unable to load ") + mNoiseFile.getFileName();
        else
            messageString = "Successfully loaded noise spectrum!";
        
        juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                           .withIconType (MessageBoxIconType::InfoIcon)
                                           .withTitle ("Progress window")
                                           .withMessage (messageString),
                                           nullptr);

        // ..and clean up by deleting our thread object..
        delete this;
    }

private:
    SpectralSubtractorAudioProcessor* mProcessor;
    juce::File mNoiseFile;
    juce::AudioFormatManager mFormatManager;
    std::unique_ptr<juce::AudioFormatReader> mReader;
    bool mErrorLoadingFile { false };
};
