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

#include "plugin/juce/PluginEditor.hpp"


namespace JS80P
{

JS80PEditor::JS80PEditor(JS80PProcessor& processor)
    : juce::AudioProcessorEditor(&processor),
    processor(processor),
    gui(nullptr),
    show_new_gui(true),
    in_resize(false)
{
    setResizable(true, true);

    juce::ComponentBoundsConstrainer* const constrainer = getConstrainer();

    if (constrainer != nullptr) {
        constrainer->setFixedAspectRatio(
            (double)JS80P::GUI::WIDTH / (double)JS80P::GUI::HEIGHT
        );
        constrainer->setSizeLimits(
            JS80P::GUI::WIDTH / 4,
            JS80P::GUI::HEIGHT / 4,
            JS80P::GUI::WIDTH,
            JS80P::GUI::HEIGHT
        );
    }

    /* Larger default than the legacy INIT_SCALE (0.48) so the new GUI's
     * per-oscillator pulse-width / harmonics section is visible without
     * resizing. Aspect ratio stays locked to the original layout. */
    setSize((int)(0.56 * JS80P::GUI::WIDTH_FLOAT), (int)(0.56 * JS80P::GUI::HEIGHT_FLOAT));

    /* Original GUI (JUCE Widget backend) — attached first, sits behind. */
    gui = new JS80P::GUI(
        JucePlugin_VersionString,
        nullptr,
        (JS80P::GUI::PlatformWidget)(juce::Component*)this,
        processor.get_synth(),
        true,
        this
    );
    gui->show();

    /* New simplified GUI — opaque, on top, shown by default. */
    new_gui = std::make_unique<NewGui>(processor.get_synth());
    addAndMakeVisible(*new_gui);

    /* Always-on-top switch between the two. */
    toggle_button.setWantsKeyboardFocus(false);
    toggle_button.onClick = [this]() {
        show_new_gui = !show_new_gui;
        new_gui->setVisible(show_new_gui);
        update_toggle();
    };
    addAndMakeVisible(toggle_button);
    update_toggle();

    startTimerHz((int)JS80P::GUI::REFRESH_RATE);

    resized();
}


JS80PEditor::~JS80PEditor()
{
    stopTimer();

    new_gui = nullptr;

    delete gui;
    gui = nullptr;
}


void JS80PEditor::update_toggle()
{
    toggle_button.setButtonText(show_new_gui ? "Detailed view" : "New view");
    toggle_button.toFront(false);
}


void JS80PEditor::resized()
{
    if (new_gui != nullptr) {
        new_gui->setBounds(getLocalBounds());
    }

    toggle_button.setBounds(getWidth() - 108, 12, 96, 22);
    toggle_button.toFront(false);

    if (gui == nullptr || in_resize) {
        return;
    }

    in_resize = true;
    gui->resize(getWidth(), getHeight());
    in_resize = false;
}


void JS80PEditor::timerCallback()
{
    if (gui != nullptr) {
        gui->idle();
    }
}


void JS80PEditor::handle_resize_request(
        int const new_width, int const new_height
) {
    setSize(new_width, new_height);
}

}
