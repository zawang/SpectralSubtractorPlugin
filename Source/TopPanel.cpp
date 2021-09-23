/*
  ==============================================================================

    TopPanel.cpp
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "TopPanel.h"

TopPanel::TopPanel(ExperimentalFilterAudioProcessor* inProcessor)
:    PanelBase(inProcessor)
{
    setSize(TOP_PANEL_WIDTH, TOP_PANEL_HEIGHT);
    
    mLoadFileButton = std::make_unique<TextButton>();
    mLoadFileButton->setButtonText("Load File...");
    mLoadFileButton->setBounds(TOP_PANEL_WIDTH * 0.05, TOP_PANEL_HEIGHT * 0.125, TOP_PANEL_WIDTH * 0.4, TOP_PANEL_HEIGHT * 0.3);
    mLoadFileButton->addListener(this);
    addAndMakeVisible(*mLoadFileButton);
}

TopPanel::~TopPanel() {}

void TopPanel::buttonClicked(Button* b)
{
    if (b == mLoadFileButton.get())
    {
        loadFile();
    }
}

void TopPanel::loadFile()
{
    mProcessor->suspendProcessing(true);
    
    if (fileChooser != nullptr)
        return;
    
    fileChooser.reset (new juce::FileChooser ("Choose a Wav or Aiff file",
                                              juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                              "*.wav; *.aiff",
                                              true,
                                              false,
                                              nullptr));
    
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) mutable
                              {
                                  // What did the user choose?
                                  juce::File file = fc.getResult();
                                  
                                  // Read the file
                                  reader.reset (mProcessor->getFormatManager()->createReaderFor(file));
                                  if (reader.get() != nullptr)
                                  {
                                      juce::AudioSampleBuffer* noiseBuffer = mProcessor->getNoiseBuffer();
                                      noiseBuffer->clear();
                                      noiseBuffer->setSize((int) reader->numChannels, (int) reader->lengthInSamples);
                                      reader->read (noiseBuffer,
                                                    0,
                                                    (int) reader->lengthInSamples,
                                                    0,
                                                    true,
                                                    true);
                                      
                                      // Calculate and store the average magnitude spectrum of mNoiseBuffer.
                                      mProcessor->storeNoiseSpectrum(*noiseBuffer);
                                  } else
                                  {
                                      juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                                                         .withIconType (juce::MessageBoxIconType::WarningIcon)
                                                                         .withTitle ("Error loading file")
                                                                         .withMessage ("Unable to load audio file"),
                                                                         nullptr);
                                  }
                                  
                                  fileChooser = nullptr;
                              }, nullptr);
    
    mProcessor->suspendProcessing(false);
}
