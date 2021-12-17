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
    
    mSpectralSubtractor.updateParameters (FFTSize[mFFTSizeParam->getIndex()],
                                          WindowOverlap[mWindowOverlapParam->getIndex()],
                                          mWindowParam->getIndex());
    mSpectralSubtractor.reset (FFTSize[mFFTSizeParam->getIndex()]);
    
    mFormatManager = std::make_unique<AudioFormatManager>();
    mFormatManager->registerBasicFormats();
    
    // Ensure proper correspondence between the window combobox items and the Spectral Subtractor class window enums.
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeRectangular] == "Rectangular");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeBartlett] == "Bartlett");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHann] == "Hann");
    jassert (WindowTypeItemsUI[SpectralSubtractor<float>::kWindowTypeHamming] == "Hamming");

    startThread (8);    // set priority as high as possible without being realtime audio priority
    
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
    jassert (mNonAutoParams.contains (parameterID));
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
    mSpectralSubtractor.setNumChannels (getTotalNumInputChannels());
    mSpectralSubtractor.updateParameters (FFTSize[mFFTSizeParam->getIndex()],
                                          WindowOverlap[mWindowOverlapParam->getIndex()],
                                          mWindowParam->getIndex());
    mRequiresUpdate.store (true);
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
    std::unique_lock lock (mSpinMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return;
    
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
    DBG ("GET STATE INFORMATION");
    
    juce::File xmlFile = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile ("SpectralSubtractor.xml");
    
    if (mNoiseBuffer.getNumChannels() != 0 && mNoiseBuffer.getNumSamples() != 0)
    {
        juce::MemoryBlock memoryBlock;
        juce::WavAudioFormat format;
        {
            // MemoryOutputStream is owned, if the writer was created successfully
            std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (new MemoryOutputStream (memoryBlock, false),
                                                                                     mReader->sampleRate,
                                                                                     mNoiseBuffer.getNumChannels(),
                                                                                     mReader->bitsPerSample,
                                                                                     juce::StringPairArray(),
                                                                                     0));
            writer->writeFromAudioSampleBuffer (mNoiseBuffer, 0, mNoiseBuffer.getNumSamples());
            // End of scope flushes the writer
        }
        apvts.state.getChildWithName (IDs::AudioData).setProperty (IDs::NoiseBuffer,
                                                                   memoryBlock.toBase64Encoding(),
                                                                   nullptr);
    }
    
    juce::ValueTree state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
    
    xml->writeTo (xmlFile, XmlElement::TextFormat());
}

// Restore the plugin's state from an XML object
void SpectralSubtractorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    DBG ("SET STATE INFORMATION");
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            juce::ValueTree savedState = juce::ValueTree::fromXml (*xmlState);
            
            int numChildren = apvts.state.getNumChildren();
            for (int i = 0; i < numChildren; ++i)   // See NonAutoParameter for why plugin state loading is done like this
            {
                juce::ValueTree stateChild = apvts.state.getChild (i);
                juce::ValueTree savedStateChild = savedState.getChild (i);
                
                jassert (savedStateChild.hasType (stateChild.getType()));
                stateChild.copyPropertiesFrom (savedStateChild, nullptr);
            }
    
            juce::MemoryBlock memoryBlock;
            if (auto noiseBufferAsString = apvts.state.getChildWithName (IDs::AudioData).getProperty (IDs::NoiseBuffer).toString();
                memoryBlock.fromBase64Encoding (noiseBufferAsString))
            {
                juce::WavAudioFormat format;
                mReader.reset (format.createReaderFor (new MemoryInputStream (memoryBlock, false), false));
                mNoiseBuffer.setSize ((int) mReader->numChannels, (int) mReader->lengthInSamples, false, false, false);
                mReader->read (&mNoiseBuffer, 0, (int) mReader->lengthInSamples, 0, true, true);
                
                // Calculate the noise spectrum from the noise buffer that was just loaded
                mRequiresUpdate.store (true);
            }
        }
    }
}

void SpectralSubtractorAudioProcessor::loadNoiseBuffer (const juce::File& noiseFile)
{
    mReader.reset (mFormatManager->createReaderFor (noiseFile));
    if (mReader.get() != nullptr)
    {
        mNoiseBuffer.setSize ((int) mReader->numChannels,
                              (int) mReader->lengthInSamples,
                              false,
                              false,
                              false);
                                                  
        mReader->read (&mNoiseBuffer,
                       0,
                       (int) mReader->lengthInSamples,
                       0,
                       true,
                       true);
        
        mRequiresUpdate.store (true);
    }
    else
    {
        if (getActiveEditor() != nullptr)
            juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                               .withIconType (MessageBoxIconType::InfoIcon)
                                               .withMessage (juce::String("Unable to load ") + noiseFile.getFileName()),
                                               nullptr);
    }
}

//==============================================================================
// Noise spectrum processing thread functions

void SpectralSubtractorAudioProcessor::run()
{
    while (!threadShouldExit())
    {
        if (mRequiresUpdate.load())
        {
            updateBackgroundThread();
            
            if (threadShouldExit()) return;         // must check this as often as possible, because this is how we know if the user's pressed 'cancel'
            
            if (mNoiseBuffer.getNumChannels() != 0 && mNoiseBuffer.getNumSamples() != 0)
            {
                // Compute spectrogram of noise signal
                Spectrogram<float> noiseSpectrogram;
                makeSpectrogram (noiseSpectrogram, mNoiseBuffer, *(mBG_FFT.get()), mBG_HopSize, *(mBG_Window.get()));
                
                if (threadShouldExit()) return;

                // Compute noise spectrum
                computeAverageSpectrum (mTempNoiseSpectrum, noiseSpectrogram, mBG_FFT->getSize());
                
                if (threadShouldExit()) return;
                
                std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
                
                if (threadShouldExit()) return;
                
                // Update FFT settings
                mSpectralSubtractor.updateParameters (mBG_FFT->getSize(),
                                                      mBG_overlap,
                                                      mBG_WindowIndex);
                
                if (threadShouldExit()) return;
                
                // Load the new noise spectrum
                mSpectralSubtractor.loadNoiseSpectrum (mTempNoiseSpectrum);
                
                if (threadShouldExit()) return;
                
                if (getActiveEditor() != nullptr)
                    juce::NativeMessageBox::showAsync (MessageBoxOptions()
                                                       .withIconType (MessageBoxIconType::InfoIcon)
                                                       .withMessage ("Successfully loaded noise spectrum!"),
                                                       nullptr);
            }
            else
            {
                std::lock_guard<audio_spin_mutex> lock (mSpinMutex);
                
                if (threadShouldExit()) return;
                
                // Update FFT settings
                mSpectralSubtractor.updateParameters (mBG_FFT->getSize(),
                                                      mBG_overlap,
                                                      mBG_WindowIndex);
                
                if (threadShouldExit()) return;
                
                // Initialize empty noise spectrum
                mSpectralSubtractor.reset (mBG_FFT->getSize());
            }
        }
        
        if (threadShouldExit()) return;
        
        wait (500);
    }
}

void SpectralSubtractorAudioProcessor::updateBackgroundThread()
{
    int fftSize = FFTSize[mFFTSizeParam->getIndex()];
    mBG_overlap = WindowOverlap[mWindowOverlapParam->getIndex()];
    mBG_WindowIndex = mWindowParam->getIndex();
    
    mBG_FFT.reset (new juce::dsp::FFT (static_cast<int> (std::log2 (fftSize))));
    if (mBG_overlap != 0) mBG_HopSize = fftSize / mBG_overlap;
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
