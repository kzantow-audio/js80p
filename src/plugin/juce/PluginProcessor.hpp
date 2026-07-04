/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JS80P__PLUGIN__JUCE__PLUGIN_PROCESSOR_HPP
#define JS80P__PLUGIN__JUCE__PLUGIN_PROCESSOR_HPP

#include <juce_audio_processors/juce_audio_processors.h>

#include "js80p.hpp"

#include "bank.hpp"
#include "renderer.hpp"
#include "synth.hpp"


namespace JS80P
{

/**
 * \brief JUCE wrapper around the (unchanged) JS80P \c Synth. Owns the DSP; the
 *        editor talks to the live \c Synth through the existing lock-free
 *        message queue + atomic getters.
 */
class JS80PProcessor : public juce::AudioProcessor
{
    public:
        JS80PProcessor();
        ~JS80PProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;

        bool isBusesLayoutSupported(BusesLayout const& layouts) const override;

        void processBlock(
            juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi
        ) override;

        void processBlock(
            juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi
        ) override;

        bool supportsDoublePrecisionProcessing() const override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override;

        juce::String const getName() const override;

        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;
        double getTailLengthSeconds() const override;

        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int index) override;
        juce::String const getProgramName(int index) override;
        void changeProgramName(int index, juce::String const& new_name) override;

        void getStateInformation(juce::MemoryBlock& dest) override;
        void setStateInformation(void const* data, int size_in_bytes) override;

        Synth& get_synth() noexcept;

    private:
        template<typename FloatType>
        void render(juce::AudioBuffer<FloatType>& buffer, juce::MidiBuffer& midi);

        void update_bpm();
        void dispatch_midi(juce::MidiBuffer& midi);

        Synth synth;
        Renderer renderer;
        Bank bank;
        int current_program;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JS80PProcessor)
};

}

#endif
