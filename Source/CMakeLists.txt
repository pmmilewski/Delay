
# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.
target_sources(${PROJECT_NAME}
        PRIVATE
        PluginEditor.h
        PluginEditor.cpp
        PluginProcessor.h
        PluginProcessor.cpp
		DelayLine.cpp
		DelayLine.h
		DSP.h
		LevelMeter.cpp
		LevelMeter.h
		Parameters.cpp
		Parameters.h
		ProtectYourEars.h
        LookAndFeel.cpp
        LookAndFeel.h
        RotaryKnob.cpp
        RotaryKnob.h
		Tempo.cpp
		Tempo.h
		Measurement.h
		Defines.h
        )

