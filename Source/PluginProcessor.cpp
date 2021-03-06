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
ExperimentalFilterAudioProcessor::ExperimentalFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters(*this, nullptr, juce::Identifier("ExperimentalFilter"),
                  {
                      std::make_unique<juce::AudioParameterFloat> (ParameterID[kParameter_SubtractionStrength],      // parameterID
                                                                   ParameterLabel[kParameter_SubtractionStrength],   // parameter name
                                                                   0.0f,                                             // minimum value
                                                                   5.0f,                                             // maximum value
                                                                   0.0f)                                             // default value
                  }),
       mSpectrogramMaker(globalFFTSize, globalHopSize),
       mFFT(std::log2(globalFFTSize))                    // For testing purposes... Delete later
#endif
{
    initializeDSP();
    
    // Set up the ValueTree that holds mNoiseSpectrum
    ValueTree child{IDs::audioData};                                                        // Create a node
    parameters.state.addChild(child, 0, nullptr);                                           // Add node to root ValueTree
    
    mSubtractionStrengthParameter = parameters.getRawParameterValue(ParameterID[kParameter_SubtractionStrength]);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    mNoiseBuffer = std::make_unique<AudioSampleBuffer>(0, 0);
    mNoiseBuffer->clear();
}

ExperimentalFilterAudioProcessor::~ExperimentalFilterAudioProcessor()
{
}

//==============================================================================
const String ExperimentalFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ExperimentalFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ExperimentalFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ExperimentalFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ExperimentalFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ExperimentalFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ExperimentalFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ExperimentalFilterAudioProcessor::setCurrentProgram (int index)
{
}

const String ExperimentalFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void ExperimentalFilterAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void ExperimentalFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    initializeDSP();
}

void ExperimentalFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    mNoiseBuffer->clear();
    mNoiseBuffer->setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ExperimentalFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ExperimentalFilterAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
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

void ExperimentalFilterAudioProcessor::storeNoiseSpectrum(const AudioSampleBuffer& noiseSignal)
{
    Spectrogram spectrogram;
    mSpectrogramMaker.perform(noiseSignal, spectrogram);
    averageSpectrum(spectrogram, mNoiseSpectrum, globalFFTSize);
}

// Tests performFrequencyOnlyForwardTransform()
/**void ExperimentalFilterAudioProcessor::testFFT(const AudioSampleBuffer& noiseSignal) {
    const float* timeDomainBuffer = noiseSignal.getReadPointer(0);
    ptrdiff_t fftSize = mFFT.getSize();
    std::vector<float> frequencyDomainBuffer(fftSize * 2UL);
    std::memcpy(frequencyDomainBuffer.data(), timeDomainBuffer, fftSize * sizeof(float));
    mFFT.performFrequencyOnlyForwardTransform(frequencyDomainBuffer.data());
}*/

// Tests perform()
void ExperimentalFilterAudioProcessor::testFFT(const AudioSampleBuffer& noiseSignal) {
    ptrdiff_t fftSize = mFFT.getSize();
    
    HeapBlock<dsp::Complex<float>> timeDomainBuffer;
    timeDomainBuffer.realloc (fftSize);
    timeDomainBuffer.clear (fftSize);
    
    HeapBlock<dsp::Complex<float>> frequencyDomainBuffer;
    frequencyDomainBuffer.realloc (fftSize);
    frequencyDomainBuffer.clear (fftSize);
    
    for (int index = 0; index < fftSize; ++index)
    {
        timeDomainBuffer[index].real (noiseSignal.getSample (0, index));
        timeDomainBuffer[index].imag (0.0f);
    }
    
    mFFT.perform(timeDomainBuffer, frequencyDomainBuffer, false);
}

//==============================================================================
bool ExperimentalFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* ExperimentalFilterAudioProcessor::createEditor()
{
    return new ExperimentalFilterAudioProcessorEditor (*this);
}

//==============================================================================
// Store the plugin's state in an XML object
void ExperimentalFilterAudioProcessor::getStateInformation (MemoryBlock& destData)
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
    std::unique_ptr<juce::XmlElement> xml (state.createXml());      // Creates an XmlElement with a tag name of "ExperimentalFilter" that holds a complete image of state and all its children
    copyXmlToBinary (*xml, destData);
    
    if (xml->writeTo(file, XmlElement::TextFormat())) DBG("toDAW written");
    else DBG("toDAW not written");
}

// Restore the plugin's state from an XML object
void ExperimentalFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    
            if (xmlState->writeTo(file, XmlElement::TextFormat())) DBG("fromDAW written");
            else DBG("fromDAW not written");
        }
    }
}

void ExperimentalFilterAudioProcessor::initializeDSP()
{
    // Initialize filter
    mFilter.setup(getTotalNumInputChannels());
    mFilter.updateParameters(globalFFTSize,
                             globalFFTSize / globalHopSize,
                             globalWindow);
    
    mNoiseSpectrum.realloc(globalFFTSize);
    mNoiseSpectrum.clear(globalFFTSize);
}

void ExperimentalFilterAudioProcessor::heapBlockToArray(HeapBlock<float>& heapBlock, Array<var>& array)
{
    for (int i = 0; i < globalFFTSize; i++)
    {
        array.add (heapBlock[i]);
    }
}

void ExperimentalFilterAudioProcessor::arrayToHeapBlock(Array<var>& array, HeapBlock<float>& heapBlock)
{
    for (int i = 0; i < globalFFTSize; i++)
    {
        heapBlock[i] = array[i];
    }
}

String ExperimentalFilterAudioProcessor::varArrayToDelimitedString (const Array<var>& input)
{
    // if you are trying to control a var that is an array then you need to
    // set a delimiter string that will be used when writing to XML!
    StringArray elements;

    for (auto& v : input)
        elements.add (v.toString());

    return elements.joinIntoString (",");
}

Array<var> ExperimentalFilterAudioProcessor::delimitedStringToVarArray (StringRef input)
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
    return new ExperimentalFilterAudioProcessor();
}
