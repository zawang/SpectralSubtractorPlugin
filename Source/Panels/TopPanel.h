/*
  ==============================================================================

    TopPanel.h
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"
#include "../Helper/NonAutoParameterComponent.h"
#include "../Helper/Parameters.h"
#include "../Plugin/PluginProcessor.h"

class TopPanel
:   public PanelBase,
    private juce::AsyncUpdater
{
public:
    TopPanel (SpectralSubtractorAudioProcessor* inProcessor);
    ~TopPanel();
    
    void resized() override;
    void handleAsyncUpdate() override;

private:
    TextButton mLoadFileButton;
    
    NonAutoParameterComboBox mFFTSizeComboBox {*mProcessor->getNonAutoParameterWithID (ParameterID[kParameter_FFTSize])};
    NonAutoParameterComboBox mWindowOverlapComboBox {*mProcessor->getNonAutoParameterWithID (ParameterID[kParameter_WindowOverlap])};
    NonAutoParameterComboBox mWindowComboBox {*mProcessor->getNonAutoParameterWithID (ParameterID[kParameter_Window])};
    
    juce::Label mFFTSizeLabel {ParameterID[kParameter_FFTSize], ParameterLabel[kParameter_FFTSize]};
    juce::Label mWindowOverlapLabel {ParameterID[kParameter_WindowOverlap], ParameterLabel[kParameter_WindowOverlap]};
    juce::Label mWindowComboLabel {ParameterID[kParameter_Window], ParameterLabel[kParameter_Window]};
    
    std::unique_ptr<juce::FileChooser> mFileChooser;
        
    void loadFile();
};
