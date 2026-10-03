//
//  RVideo.h
//  raylib-miniscript video plugin
//
//  The player lives in RVideo.cpp; plugin.cpp wires it into the plugin system.
//

#ifndef RVIDEO_H
#define RVIDEO_H

#include "miniscript.h"

// Adds the `video` module's functions to the map.
void AddRVideoMethods(MiniScript::ValueDict& videoModule);

// Tear down players whose video map was garbage collected (call once per frame).
void VideoDrainPending();

// Release every player script never unloaded (call at shutdown).
void VideoShutdownAll();

#endif // RVIDEO_H
