/*
  ==============================================================================

    TopPanel.h
    Created: 15 Aug 2019 11:52:53am
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once
#include "PanelBase.h"

class TopPanel
:   public PanelBase,
    public Button::Listener
{
public:
    TopPanel(ExperimentalFilterAudioProcessor* inProcessor);
    ~TopPanel();
    
    void buttonClicked(Button* b) override;

private:
    std::unique_ptr<TextButton> mLoadFileButton;
        
    void loadFile();
};
