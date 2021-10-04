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

class TopPanel
:   public PanelBase,
    public Button::Listener
{
public:
    TopPanel(SpectralSubtractorAudioProcessor* inProcessor);
    ~TopPanel();
    
    void resized() override;
    void buttonClicked(Button* b) override;

private:
    std::unique_ptr<TextButton> mLoadFileButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioFormatReader> reader;
        
    void loadFile();
};
