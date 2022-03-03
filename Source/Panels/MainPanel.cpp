/*
  ==============================================================================

    MainPanel.cpp
    Created: 15 Aug 2019 11:12:20am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "MainPanel.h"

MainPanel::MainPanel (SpectralSubtractorAudioProcessor* inProcessor)
    : PanelBase (inProcessor),
      mTopPanel (inProcessor)/*,
      mBottomPanel (inProcessor)*/
{
    addAndMakeVisible (mTopPanel);
//    addAndMakeVisible (mBottomPanel);
}

MainPanel::~MainPanel() {}

void MainPanel::resized()
{
    int width = getWidth();
    int height = getHeight();
    
    mTopPanel.setBounds (0, 0, width, 0.4f * height);
//    mBottomPanel.setBounds (0, mTopPanel.getHeight(), width, height - mTopPanel.getHeight());
}
