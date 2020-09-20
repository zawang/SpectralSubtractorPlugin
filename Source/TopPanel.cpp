/*
  ==============================================================================

    TopPanel.cpp
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "TopPanel.h"

TopPanel::TopPanel(ExperimentalFilterAudioProcessor* inProcessor)
:    PanelBase(inProcessor) {
    setSize(TOP_PANEL_WIDTH, TOP_PANEL_HEIGHT);
    
    mLoadFileButton = std::make_unique<TextButton>();
    mLoadFileButton->setButtonText("Load File...");
    mLoadFileButton->setBounds(TOP_PANEL_WIDTH * 0.05, TOP_PANEL_HEIGHT * 0.125, TOP_PANEL_WIDTH * 0.4, TOP_PANEL_HEIGHT * 0.3);
    mLoadFileButton->addListener(this);
    addAndMakeVisible(*mLoadFileButton);
}

TopPanel::~TopPanel() {
    
}

void TopPanel::buttonClicked(Button* b) {
    if (b == mLoadFileButton.get()) {
        loadFile();
    }
}

void TopPanel::loadFile() {
    mProcessor->suspendProcessing(true);
    
    // Choose a file
    FileChooser chooser ("Choose a Wav or Aiff file",
                         File::getSpecialLocation(File::userDesktopDirectory),
                         "*.wav; *.aiff",
                         true,
                         false,
                         nullptr);
    
    // If the user chooses a file
    if (chooser.browseForFileToOpen()) {
        // What did the user choose?
        File file = chooser.getResult();
        
        // Read the file
        std::unique_ptr<AudioFormatReader> reader (mProcessor->getFormatManager()->createReaderFor(file));
        
        if (reader != nullptr) {
            AudioSampleBuffer* noiseBuffer = mProcessor->getNoiseBuffer();
            noiseBuffer->clear();
            
            noiseBuffer->setSize((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (noiseBuffer,
                          0,
                          (int) reader->lengthInSamples,
                          0,
                          true,
                          true);
            
            // Reset mPosition to 0. This only matters for the purposes of the getNextAudioBlock function in PluginProcessor (not used in the final plugin).
            *(mProcessor->getPosition()) = 0;
            
            // Calculate and store the average magnitude spectrum of mNoiseBuffer.
            mProcessor->storeNoiseSpectrum(*noiseBuffer);
        } else {
            // Error handling here...
            DBG("reader error!!!");
        }
    }
    
    mProcessor->suspendProcessing(false);
}
