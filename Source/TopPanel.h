/*
  ==============================================================================

    TopPanel.h
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "NoiseSpectrumProcessingThread.h"
#include "ParameterComponent.h"

class TopPanel
:   public PanelBase
{
public:
    TopPanel (SpectralSubtractorAudioProcessor* inProcessor);
    ~TopPanel();
    
    void resized() override;

private:
    TextButton mLoadFileButton;
    ParameterComboBox mFFTSizeComboBox {mProcessor->apvts, ParameterID[kParameter_FFTSize], ParameterLabel[kParameter_FFTSize]};
    std::unique_ptr<juce::FileChooser> mFileChooser;
        
    void loadFile();
};
