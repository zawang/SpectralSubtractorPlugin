/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IDs.h"

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
       parameters(*this, nullptr, juce::Identifier("SpectralSubtractor"),
                  {
                      std::make_unique<juce::AudioParameterFloat> (ParameterID[kParameter_SubtractionStrength],      // parameterID
                                                                   ParameterLabel[kParameter_SubtractionStrength],   // parameter name
                                                                   0.0f,                                             // minimum value
                                                                   5.0f,                                             // maximum value
                                                                   0.0f)                                             // default value
                  })
#endif
{
    initializeDSP();
    
    // Set up the ValueTree that holds mNoiseSpectrum
    ValueTree child{IDs::audioData};                                                        // Create a node
    parameters.state.addChild(child, 0, nullptr);                                           // Add node to root ValueTree
    
    mSubtractionStrengthParameter = parameters.getRawParameterValue(ParameterID[kParameter_SubtractionStrength]);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
}

SpectralSubtractorAudioProcessor::~SpectralSubtractorAudioProcessor()
{
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
    
    mFilter.processBlock(buffer, mNoiseSpectrum, *mSubtractionStrengthParameter);
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
//    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
//        float* channelData = buffer.getWritePointer(channel);
//        mFilters[channel]->processBlock(channelData, channelData, buffer.getNumSamples());
//    }
}

// Replaces the old noise spectrum with a new noise spectrum.
void SpectralSubtractorAudioProcessor::loadNewNoiseSpectrum(HeapBlock<float>& tempNoiseSpectrum)
{
    jassert (isSuspended());    // playback must be suspended!
    
    // TODO: use memcpy instead?
    for (int i = 0; i < globalFFTSize; ++i)
        mNoiseSpectrum[i] = tempNoiseSpectrum[i];
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    File file("/Users/zach/Desktop/toDAW.xml");
    
    if (mNoiseSpectrum != nullptr)
    {
        Array<var> temp;
        heapBlockToArray(mNoiseSpectrum, temp);
        var mNoiseSpectrumAsString = var(varArrayToDelimitedString(temp));
        parameters.state.getChild(0).setProperty(IDs::noiseSpectrumID, mNoiseSpectrumAsString, nullptr);
    }
    
    ValueTree state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());      // Creates an XmlElement with a tag name of "SpectralSubtractor" that holds a complete image of state and all its children
    copyXmlToBinary (*xml, destData);
    
    xml->writeTo(file, XmlElement::TextFormat());
//    if (xml->writeTo(file, XmlElement::TextFormat()))
//        DBG("toDAW written");
//    else
//        DBG("toDAW not written");
}

// Restore the plugin's state from an XML object
void SpectralSubtractorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    File file("/Users/zach/Desktop/fromDAW.xml");
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
    
            mNoiseSpectrum.realloc(globalFFTSize);
            mNoiseSpectrum.clear(globalFFTSize);
            Array<var> mNoiseSpectrumAsArray = delimitedStringToVarArray(parameters.state.getChild(0).getProperty(IDs::noiseSpectrumID).toString());
            arrayToHeapBlock(mNoiseSpectrumAsArray, mNoiseSpectrum);
    
            xmlState->writeTo(file, XmlElement::TextFormat());
//            if (xmlState->writeTo(file, XmlElement::TextFormat()))
//                DBG("fromDAW written");
//            else
//                DBG("fromDAW not written");
        }
    }
}

void SpectralSubtractorAudioProcessor::initializeDSP()
{
    // Initialize filter
    mFilter.setup(getTotalNumInputChannels());
    mFilter.updateParameters(globalFFTSize,
                             globalFFTSize / globalHopSize,
                             globalWindow);
    
    mNoiseSpectrum.realloc(globalFFTSize);
    mNoiseSpectrum.clear(globalFFTSize);
}

void SpectralSubtractorAudioProcessor::heapBlockToArray(HeapBlock<float>& heapBlock, Array<var>& array)
{
    for (int i = 0; i < globalFFTSize; i++)
        array.add (heapBlock[i]);
}

void SpectralSubtractorAudioProcessor::arrayToHeapBlock(Array<var>& array, HeapBlock<float>& heapBlock)
{
    for (int i = 0; i < globalFFTSize; i++)
        heapBlock[i] = array[i];
}

String SpectralSubtractorAudioProcessor::varArrayToDelimitedString (const Array<var>& input)
{
    // if you are trying to control a var that is an array then you need to
    // set a delimiter string that will be used when writing to XML!
    StringArray elements;

    for (auto& v : input)
        elements.add (v.toString());

    return elements.joinIntoString (",");
}

Array<var> SpectralSubtractorAudioProcessor::delimitedStringToVarArray (StringRef input)
{
    Array<var> arr;
    
    for (auto t : StringArray::fromTokens (input, ",", {}))
        arr.add (t);
    
    return arr;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralSubtractorAudioProcessor();
}
