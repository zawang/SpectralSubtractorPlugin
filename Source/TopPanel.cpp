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
    mLoadFileButton = std::make_unique<TextButton>();
    mLoadFileButton->setButtonText("Load File...");
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
    DBG("loadFile start");
    
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
                                  DBG("launchAsync start");
                                  mProcessor->suspendProcessing(true);
        
                                  // What did the user choose?
                                  juce::File file = fc.getResult();
                                    
                                  if (file != juce::File{})
                                  {
                                      // Read the file
                                      reader.reset (mProcessor->getFormatManager()->createReaderFor(file));
                                      if (reader.get() != nullptr)
                                      {
                                          mProcessor->calcAndStoreNoiseSpectrum(reader.get());
                                      }
                                      else
                                      {
                                          juce::NativeMessageBox::showAsync (juce::MessageBoxOptions()
                                                                             .withIconType (juce::MessageBoxIconType::WarningIcon)
                                                                             .withTitle ("Error loading file")
                                                                             .withMessage ("Unable to load audio file"),
                                                                             nullptr);
                                      }
                                  }
                                  
                                  fileChooser = nullptr;
        
                                  mProcessor->suspendProcessing(false);
                                  DBG("launchAsync end");
                              }, nullptr);
    
    DBG("loadFile end");
}
