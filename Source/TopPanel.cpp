/*
  ==============================================================================

    TopPanel.cpp
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "TopPanel.h"

TopPanel::TopPanel(SpectralSubtractorAudioProcessor* inProcessor)
:    PanelBase(inProcessor)
{
    mLoadFileButton = std::make_unique<TextButton>();
    mLoadFileButton->setButtonText("Import noise file...");
    mLoadFileButton->addListener(this);
    addAndMakeVisible(*mLoadFileButton);
}

TopPanel::~TopPanel() {}

void TopPanel::resized()
{
    auto width = getWidth();
    auto height = getHeight();
    
    mLoadFileButton->setBounds(width * 0.025, height * 0.125, width * 0.4, height * 0.3);
}

void TopPanel::buttonClicked(Button* b)
{
    if (b == mLoadFileButton.get())
    {
        loadFile();
    }
}

void TopPanel::loadFile()
{
    if (fileChooser != nullptr)
        return;
    
    fileChooser.reset (new juce::FileChooser ("Select an audio file",
                                              juce::File::getSpecialLocation (juce::File::userDesktopDirectory),
                                              mProcessor->getFormatManager()->getWildcardForAllFormats(),
                                              true,
                                              false,
                                              nullptr));
    
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) mutable
                              {
                                  juce::File file = fc.getResult();
                                  if (file != juce::File{})
                                  {
                                      (new NoiseSpectrumProcessingThread (mProcessor, file))->launchThread (7);
                                  }
                                  
                                  fileChooser = nullptr;
                              }, nullptr);
}
