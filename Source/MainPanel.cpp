/*
  ==============================================================================

    MainPanel.cpp
    Created: 15 Aug 2019 11:12:20am
    Author:  Zachary Wang

  ==============================================================================
*/

#include "MainPanel.h"

MainPanel::MainPanel(ExperimentalFilterAudioProcessor* inProcessor)
:   PanelBase(inProcessor)
{
    mTopPanel = std::make_unique<TopPanel>(inProcessor);
    addAndMakeVisible(*mTopPanel);
    
    mBottomPanel = std::make_unique<BottomPanel>(inProcessor);
    addAndMakeVisible(*mBottomPanel);
}

MainPanel::~MainPanel() {}

void MainPanel::resized()
{
    auto width = getWidth();
    auto height = getHeight();
    
    mTopPanel->setBounds (0, 0, width, 0.2 * height);
    mBottomPanel->setBounds (0, mTopPanel->getHeight(), width, height - mTopPanel->getHeight());
}
