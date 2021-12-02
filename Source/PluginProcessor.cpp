/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
                       )
#endif
{
    setParams();
    
    initializeDSP();
    
    juce::ValueTree audioDataNode {IDs::AudioData};
    apvts.state.appendChild (audioDataNode, nullptr);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    
#if RUN_UNIT_TESTS == 1
    std::cout << "Running unit tests..." << std::endl;
    mUnitTestRunner.runAllTests();
#endif
}

SpectralSubtractorAudioProcessor::~SpectralSubtractorAudioProcessor()
{
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
    
    params.push_back (std::make_unique<PluginParameterChoice> (ParameterID[kParameter_FFTSize],
                                                               ParameterLabel[kParameter_FFTSize],
                                                               FFTSizeItems,
                                                               kFFTSize2048,
                                                               juce::String(),
                                                               nullptr,
                                                               nullptr,
                                                               [this] (int newValue)
                                                               {
                                                                    DBG ("FFT size: " << FFTSize[newValue]);
                                                                    initializeDSP();
                                                               }));
    
    return { params.begin(), params.end() };
}

void SpectralSubtractorAudioProcessor::setParams()
{
    mSubtractionStrengthParam = apvts.getRawParameterValue (ParameterID[kParameter_SubtractionStrength]);
    jassert (mSubtractionStrengthParam);
    mSpectralSubtractor.setSubtractionStrength (mSubtractionStrengthParam);
    
    mFFTSizeParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ParameterID[kParameter_FFTSize]));
    jassert (mFFTSizeParam);
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    initializeDSP();
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
    
    mSpectralSubtractor.processBlock (buffer);
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

// Replaces the old noise spectrum with a new noise spectrum.
void SpectralSubtractorAudioProcessor::loadNewNoiseSpectrum (HeapBlock<float>& tempNoiseSpectrum)
{
    jassert (isSuspended());    // playback must be suspended!
    
    // TODO: add a saftey check to ensure mNoiseSpectrum and tempNoiseSpectrum have the same element type?
    
    std::memcpy (mNoiseSpectrum.get(), tempNoiseSpectrum, mNoiseSpectrum.size() * sizeof (float));
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
    
    if (mNoiseSpectrum.get().get() != nullptr)
        apvts.state.getChildWithName (IDs::AudioData).setProperty (IDs::NoiseSpectrum, mNoiseSpectrum.toString(), nullptr);
    
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
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    
            if (auto noiseSpectrumAsString = apvts.state.getChildWithName (IDs::AudioData).getProperty (IDs::NoiseSpectrum).toString();
                !noiseSpectrumAsString.isEmpty())
                mNoiseSpectrum.allocateFromString (noiseSpectrumAsString);
        }
    }
}

void SpectralSubtractorAudioProcessor::initializeDSP()
{
    // Initialize filter
    mSpectralSubtractor.setup (getTotalNumInputChannels());
    mSpectralSubtractor.updateParameters (FFTSize[mFFTSizeParam->getIndex()],
                                          FFTSize[mFFTSizeParam->getIndex()] / mHopSize,
                                          mWindow);
    
    mNoiseSpectrum.realloc (FFTSize[mFFTSizeParam->getIndex()]);
    mNoiseSpectrum.clear (FFTSize[mFFTSizeParam->getIndex()]);
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralSubtractorAudioProcessor();
}
