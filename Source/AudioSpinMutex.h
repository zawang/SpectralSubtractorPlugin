/*
  ==============================================================================

    AudioSpinMutex.h
    Created: 2 Dec 2021 4:26:47pm
    Author:  Zachary Wang

  ==============================================================================
*/

#pragma once

// Based off of Timur Doumler's audio spin lock implementation from his blog post https://timur.audio/using-locks-in-real-time-audio-processing-safely and his ADC20 talk "Using Locks in Real-Time Audio Processing, Safely".
// TODO: In Timur's ADC20 talk he mentioned that at some point he would publish his audio spin lock implementation on GitHub, so be on the lookout for that.

// TODO: Do ARM/Intel checking without relying on JUCE's macros.

#include "JuceHeader.h"
#include <array>
#include <thread>
#include <atomic>

#if JUCE_INTEL == 1
    #include <emmintrin.h>
#elif JUCE_ARM == 1
    #include <arm_acle.h>
#else
    #error "Only Intel and ARM architectures are supported!"
#endif

struct audio_spin_mutex {
#if JUCE_INTEL == 1
    void lock() noexcept {
        // approx. 5x5 ns (= 25 ns), 10x40 ns (= 400 ns), and 3000x350 ns
        // (~ 1 ms), respectively, when measured on a 2.9 GHz Intel i9
        constexpr std::array iterations = {5, 10, 3000};

        for (int i = 0; i < iterations[0]; ++i) {
            if (try_lock())
                return;
        }

        for (int i = 0; i < iterations[1]; ++i) {
            if (try_lock())
                return;

            _mm_pause();
        }

        while (true) {
            for (int i = 0; i < iterations[2]; ++i) {
                if (try_lock())
                    return;

                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
                _mm_pause();
            }

            // waiting longer than we should, let's give other threads
            // a chance to recover
            std::this_thread::yield();
        }
    }
#elif JUCE_ARM == 1
    void lock() noexcept {
        // approx. 20 ns and ~1 ms respectively, when measured on Timur's ARM machine
        constexpr std::array iterations = {2, 750};

        for (int i = 0; i < iterations[0]; ++i) {
            if (try_lock())
                return;
        }

        while (true) {
            for (int i = 0; i < iterations[1]; ++i) {
                if (try_lock())
                    return;

                __wfe();
            }

            std::this_thread::yield();
        }
    }
#endif

    bool try_lock() noexcept {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
