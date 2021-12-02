/*
  ==============================================================================

    SerializableHeapBlock.h
    Created: 20 Nov 2021 3:18:30pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "HelperFunctions.h"

/** Simple wrapper of juce::HeapBlock that can convert to/from a juce::String and has a variable to keep track of the number of elements allocated.  */
template <class ElementType, bool throwOnFailure = false>
class HeapBlockWrapper
{
public:
    HeapBlockWrapper ()
    {}
    
    template <typename SizeType>
    HeapBlockWrapper (SizeType numElements)
        : mHeapBlock (numElements),
          mNumElements (numElements)
    {}
    
    template <typename SizeType>
    HeapBlockWrapper (SizeType numElements, bool initialiseToZero)
        : mHeapBlock (numElements, initialiseToZero),
          mNumElements (numElements)
    {}
    
    juce::HeapBlock<ElementType, throwOnFailure>& get() noexcept { return mHeapBlock; }
    
    size_t size() const noexcept { return mNumElements; }
    
    template <typename IndexType>
    ElementType& operator[] (IndexType index) const noexcept { return mHeapBlock[index]; }
    
    void allocateFromString (const juce::String& input)
    {
        juce::Array<juce::var> array = delimitedStringToVarArray (input);
        
        realloc (array.size());
        clear (array.size());
        
        arrayToHeapBlock (array);
    }
    
    template <typename SizeType>
    void malloc (SizeType newNumElements, size_t elementSize = sizeof (ElementType))
    {
        mHeapBlock.malloc (newNumElements, elementSize);
        mNumElements = newNumElements;
    }
    
    template <typename SizeType>
    void calloc (SizeType newNumElements, size_t elementSize = sizeof (ElementType))
    {
        mHeapBlock.calloc (newNumElements, elementSize);
        mNumElements = newNumElements;
    }
    
    template <typename SizeType>
    void realloc (SizeType newNumElements, size_t elementSize = sizeof (ElementType))
    {
        mHeapBlock.realloc (newNumElements, elementSize);
        mNumElements = newNumElements;
    }
    
    template <typename SizeType>
    void clear (SizeType numElements) noexcept
    {
        mHeapBlock.clear (numElements);
    }
    
    const juce::String toString()
    {
        if (mHeapBlock.get() == nullptr)
            return juce::String{};
        
        juce::Array<juce::var> array;
        toArray (array);
        return varArrayToDelimitedString (array);
    }
    
private:
    juce::HeapBlock<ElementType, throwOnFailure> mHeapBlock;
    size_t mNumElements;
    
    juce::String mDelimiter { "," };
    
    void toArray (juce::Array<juce::var>& array)
    {
        for (size_t i = 0; i < mNumElements; ++i)
            array.add (mHeapBlock[i]);
    }

    void arrayToHeapBlock (const juce::Array<juce::var>& array)
    {
        for (size_t i = 0; i < mNumElements; ++i)
            mHeapBlock[i] = array[i];
    }
    
    // See ValueWithDefault::varArrayToDelimitedString()
    juce::String varArrayToDelimitedString (const juce::Array<juce::var>& input) const noexcept
    {
        // if you are trying to control a var that is an array then you need to
        // set a delimiter string that will be used when writing to XML!
        jassert (mDelimiter.isNotEmpty());

        StringArray elements;

        for (auto& v : input)
            elements.add (v.toString());

        return elements.joinIntoString (mDelimiter);
    }

    // See ValueWithDefault::delimitedStringToVarArray()
    juce::Array<juce::var> delimitedStringToVarArray (juce::StringRef input) const noexcept
    {
        juce::Array<juce::var> arr;

        for (auto t : juce::StringArray::fromTokens (input, mDelimiter, {}))
            arr.add (t);

        return arr;
    }

};
