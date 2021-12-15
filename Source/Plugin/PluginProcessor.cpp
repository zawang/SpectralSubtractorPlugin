/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Tests/test_main.cpp"

//==============================================================================
SpectralSubtractorAudioProcessor::SpectralSubtractorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
       juce::Thread ("Noise spectrum processing thread") // thread that calculates a noise file's average spectrum for use in the spectral subtractor

#endif
{
    setParams();
    attachSubTrees();
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    
    // Ensure proper correspondence between the window combobox items and the Spectral Subtractor class window enums.
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeRectangular] == "Rectangular");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeBartlett] == "Bartlett");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHann] == "Hann");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHamming] == "Hamming");

    // Give the background thread realtime audio priority since we suspend processing whenever the background thread does work
    startThread (juce::Thread::realtimeAudioPriority);
    
#if RUN_UNIT_TESTS == 1
    std::cout << "Running unit tests..." << std::endl;
    mUnitTestRunner.runAllTests();
#endif
}

SpectralSubtractorAudioProcessor::~SpectralSubtractorAudioProcessor()
{
    stopThread (10000);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralSubtractorAudioProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> params;
    
    juce::NormalisableRange<float> subtractionStrengthRange {0.0f, 5.0f};
    float subtractionStrengthDefault = 0.0f;
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID[kParameter_SubtractionStrength],
                                                                   ParameterLabel[kParameter_SubtractionStrength],
                                                                   subtractionStrengthRange,
                                                                   subtractionStrengthDefault));
    
    return { params.begin(), params.end() };
}

void SpectralSubtractorAudioProcessor::setParams()
{
    mSubtractionStrengthParam = apvts.getRawParameterValue (ParameterID[kParameter_SubtractionStrength]);
    jassert (mSubtractionStrengthParam);
    mSpectralSubtractor.setSubtractionStrength (mSubtractionStrengthParam);
    
    mFFTSizeParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_FFTSize],
                                                              ParameterLabel[kParameter_FFTSize],
                                                              FFTSizeItemsUI,
                                                              kFFTSize2048);
    mNonAutoParams.emplace (mFFTSizeParam->getParameterID(), mFFTSizeParam.get());
    
    mWindowOverlapParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_WindowOverlap],
                                                                    ParameterLabel[kParameter_WindowOverlap],
                                                                    WindowOverlapItemsUI,
                                                                    kWindowOverlap4);
    mNonAutoParams.emplace (mWindowOverlapParam->getParameterID(), mWindowOverlapParam.get());
    
    mWindowParam = std::make_unique<NonAutoParameterChoice> (ParameterID[kParameter_Window],
                                                             ParameterLabel[kParameter_Window],
                                                             WindowTypeItemsUI,
                                                             SpectralSubtractor<float>::kWindowTypeHann);
    mNonAutoParams.emplace (mWindowParam->getParameterID(), mWindowParam.get());
}

void SpectralSubtractorAudioProcessor::attachSubTrees()
{
    apvts.state.appendChild (mFFTSizeParam->getValueTree(), nullptr);
    apvts.state.appendChild (mWindowOverlapParam->getValueTree(), nullptr);
    apvts.state.appendChild (mWindowParam->getValueTree(), nullptr);
    apvts.state.appendChild (mAudioDataTree, nullptr);
}

NonAutoParameterChoice& SpectralSubtractorAudioProcessor::getNonAutoParameterWithID (const String& parameterID)
{
    // TODO: add this line once the Projucer supports C++20: jassert (mNonAutoParams.contains (parameterID));
    return *(mNonAutoParams[parameterID]);
}

//==============================================================================
const String SpectralSubtractorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectralSubtractorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectralSubtractorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectralSubtractorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectralSubtractorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectralSubtractorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectralSubtractorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectralSubtractorAudioProcessor::setCurrentProgram (int index)
{
}

const String SpectralSubtractorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SpectralSubtractorAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void SpectralSubtractorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    prepareAndResetSpectralSubtractor();
}

void SpectralSubtractorAudioProcessor::prepareAndResetSpectralSubtractor()
{
    mSpectralSubtractor.prepare (getTotalNumInputChannels(),
                                 FFTSize[mFFTSizeParam->getIndex()],
                                 WindowOverlap[mWindowOverlapParam->getIndex()],
                                 mWindowParam->getIndex());
    wakeUpBackgroundThread();
}

void SpectralSubtractorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralSubtractorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if c input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SpectralSubtractorAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    mSpectralSubtractor.process (buffer);
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

//==============================================================================
bool SpectralSubtractorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SpectralSubtractorAudioProcessor::createEditor()
{
    return new SpectralSubtractorAudioProcessorEditor (*this);
}

//==============================================================================
// Store the plugin's state in an XML object
void SpectralSubtractorAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    juce::File xmlFile = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile ("SpectralSubtractor.xml");
    
    apvts.state.getChildWithName (IDs::AudioData).setProperty (IDs::NoiseSpectrum, mSpectralSubtractor.getNoiseSpectrumAsString(), nullptr);
    
    juce::ValueTree state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
    
    xml->writeTo (xmlFile, XmlElement::TextFormat());
}

// Restore the plugin's state from an XML object
void SpectralSubtractorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            juce::ValueTree savedState = juce::ValueTree::fromXml (*xmlState);
            
            int numChildren = apvts.state.getNumChildren();
            for (int i = 0; i < numChildren; ++i)
            {
                juce::ValueTree stateChild = apvts.state.getChild (i);
                juce::ValueTree savedStateChild = savedState.getChild (i);
                
                jassert (savedStateChild.hasType (stateChild.getType()));
                stateChild.copyPropertiesFrom (savedStateChild, nullptr);
            }
    
            if (auto noiseSpectrumAsString = apvts.state.getChildWithName (IDs::AudioData).getProperty (IDs::NoiseSpectrum).toString();
                !noiseSpectrumAsString.isEmpty())
                mSpectralSubtractor.loadNoiseSpectrumFromString (noiseSpectrumAsString);
        }
    }
}

//==============================================================================
// Noise spectrum processing thread functions

void SpectralSubtractorAudioProcessor::run()
{
    while (true)
    {
        if (mRequiresUpdate.load()) updateBackgroundThread();
        
        suspendProcessing (true);
        
        if (threadShouldExit()) return;         // must check this as often as possible, because this is how we know if the user's pressed 'cancel'
        if (mRequiresUpdate.load()) continue;   // if the FFT settings change while the background thread is doing work, start over with the most up to date settings
        
        if (mNoiseBuffer->getNumChannels() != 0 && mNoiseBuffer->getNumSamples() != 0)
        {
            // Compute spectrogram of noise signal
            Spectrogram<float> noiseSpectrogram;
            makeSpectrogram (noiseSpectrogram, mNoiseBuffer.get(), *(mBG_FFT.get()), mBG_HopSize, *(mBG_Window.get()));
            
            if (threadShouldExit()) return;
            if (mRequiresUpdate.load()) continue;

            // Compute noise spectrum
            computeAverageSpectrum (mSpectralSubtractor.getNoiseSpectrum(), noiseSpectrogram, mBG_FFT->getSize());
            
            if (threadShouldExit()) return;
            if (mRequiresUpdate.load()) continue;
            
            juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                               .withIconType (MessageBoxIconType::InfoIcon)
                                               .withMessage ("Successfully loaded noise spectrum!"),
                                               nullptr);
        }
        else
            mSpectralSubtractor.reset (FFTSize[mFFTSizeParam->getIndex()]);
        
        suspendProcessing (false);
        
        if (mRequiresUpdate.load()) continue;
        
        wait (-1);
    }
}

void SpectralSubtractorAudioProcessor::wakeUpBackgroundThread()
{
    mRequiresUpdate.store (true);
    notify();
}

void SpectralSubtractorAudioProcessor::updateBackgroundThread()
{
    int fftSize = FFTSize[mFFTSizeParam->getIndex()];
    int overlap = WindowOverlap[mWindowOverlapParam->getIndex()];
    
    mBG_FFT.reset (new juce::dsp::FFT (static_cast<int> (std::log2 (fftSize))));
    if (overlap != 0) mBG_HopSize = fftSize / overlap;
    mBG_Window.reset (new juce::dsp::WindowingFunction<float> (static_cast<size_t> (fftSize + 1), juce::dsp::WindowingFunction<float>::hann, false));
    
    DBG ("Background thread FFT size: " << mBG_FFT->getSize());
    DBG ("Background thread hop size: " << mBG_HopSize);
    DBG ("");
    
    mRequiresUpdate.store (false);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralSubtractorAudioProcessor();
}
