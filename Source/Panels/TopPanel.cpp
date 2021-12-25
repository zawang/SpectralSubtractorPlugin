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
    mFFTSizeComboBox.onChange = [this] { triggerAsyncUpdate(); };
    
    addAndMakeVisible (mWindowOverlapComboBox);
    mWindowOverlapComboBox.onChange = [this] { triggerAsyncUpdate(); };
    
    addAndMakeVisible (mWindowComboBox);
    mWindowComboBox.onChange = [this] { triggerAsyncUpdate(); };
    
    addAndMakeVisible (mFFTSizeLabel);
    mFFTSizeLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    mFFTSizeLabel.attachToComponent (&mFFTSizeComboBox, true);
    
    addAndMakeVisible (mWindowOverlapLabel);
    mWindowOverlapLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    mWindowOverlapLabel.attachToComponent (&mWindowOverlapComboBox, true);
    
    addAndMakeVisible (mWindowComboLabel);
    mWindowComboLabel.setColour (juce::Label::textColourId, juce::Colours::black);
    mWindowComboLabel.attachToComponent (&mWindowComboBox, true);
}

TopPanel::~TopPanel() {}

void TopPanel::resized()
{
    int height = getHeight();
    float padding = 0.04f * height;
    float componentHeight = 0.2f * height;
    auto area = getLocalBounds().reduced (padding).removeFromRight (0.6f * getWidth());
    
    mLoadFileButton.setBounds (area.removeFromTop (componentHeight));
    area.removeFromTop (padding);
    mFFTSizeComboBox.setBounds (area.removeFromTop (componentHeight));
    area.removeFromTop (padding);
    mWindowOverlapComboBox.setBounds (area.removeFromTop (componentHeight));
    area.removeFromTop (padding);
    mWindowComboBox.setBounds (area.removeFromTop (componentHeight));
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
                                      juce::String path = file.getFullPathName();
                                      
                                      {
                                          const juce::ScopedLock lock (mProcessor->mPathMutex);
                                          mProcessor->mChosenPath.swapWith (path);
                                      }
                                      
                                      mProcessor->notify();
                                  }
                                  // If file == juce::File{}, it means that the user pressed cancel.
                                  
                                  mFileChooser = nullptr;
                              }, nullptr);
}

/**
 Coalesces FFT setting updates into a single callback.
 When the top panel is constructed, all three FFT setting comboboxes have their onChange callback triggered.
 Coalescing these updates with an AsyncUpdater makes it so that the body of this function executes probably only once instead of three times.
*/
void TopPanel::handleAsyncUpdate()
{
    const juce::ScopedLock lock (mProcessor->mBackgroundMutex);
    mProcessor->mRequiresUpdate = true;
}
