/*
  ==============================================================================

    TopPanel.cpp
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "TopPanel.h"

TopPanel::TopPanel (SpectralSubtractorAudioProcessor* inProcessor)
    : PanelBase (inProcessor)
{
    mLoadFileButton.setButtonText ("Import noise file...");
    addAndMakeVisible (mLoadFileButton);
    mLoadFileButton.onClick = [this] { loadFile(); };
    
    addAndMakeVisible (mFFTSizeComboBox);
    mFFTSizeComboBox.onChange = [this] { mProcessor->prepareAndResetSpectralSubtractor(); };
    
    addAndMakeVisible (mWindowOverlapComboBox);
    mWindowOverlapComboBox.onChange = [this] { mProcessor->prepareAndResetSpectralSubtractor(); };
    
    addAndMakeVisible (mWindowComboBox);
    mWindowComboBox.onChange = [this] { mProcessor->prepareAndResetSpectralSubtractor(); };
}

TopPanel::~TopPanel() {}

void TopPanel::resized()
{
    int width = getWidth();
    int height = getHeight();
    float borderGap = width * 0.025f;
    
    mLoadFileButton.setBounds (borderGap, borderGap, width * 0.4f, height * 0.15f);

    mFFTSizeComboBox.setBounds (borderGap, mLoadFileButton.getBottom() + borderGap, width * 0.5f, height * 0.2f);
    
    mWindowOverlapComboBox.setBounds (borderGap, mFFTSizeComboBox.getBottom() + borderGap, width * 0.5f, height * 0.2f);
    
    mWindowComboBox.setBounds (borderGap, mWindowOverlapComboBox.getBottom() + borderGap, width * 0.5f, height * 0.2f);
}

void TopPanel::loadFile()
{
    if (mFileChooser != nullptr)
        return;
    
    mFileChooser.reset (new juce::FileChooser ("Select an audio file",
                                              juce::File::getSpecialLocation (juce::File::userDesktopDirectory),
                                              mProcessor->getFormatManager()->getWildcardForAllFormats(),
                                              true,
                                              false,
                                              nullptr));
    
    mFileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  juce::File file = fc.getResult();
                                  if (file != juce::File{})
                                  {
                                      (new NoiseSpectrumProcessingThread<float> (mProcessor, file, mProcessor->getFFTSize(), mProcessor->getWindowOverlap()))->launchThread (juce::Thread::realtimeAudioPriority);
                                  }
                                  
                                  mFileChooser = nullptr;
                              }, nullptr);
}