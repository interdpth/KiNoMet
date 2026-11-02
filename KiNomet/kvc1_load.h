#pragma once
#include "KiNoMet.h"

// KVC1-C container player -- the new-codec counterpart to LoadAVI() for
// KinoFileFormat::Kinopack (see KiNoMet.h; that enum value existed already,
// unused, before this codec filled it in). file/size is a complete .kvc1
// blob (see KinoMetGUI/kvc1_tools/kvc1_encoder.py's module docstring for the
// exact format); audiofile/audiofsize is the same AudioHeader-prefixed
// container LoadAVI already takes, unchanged.
void LoadKvc1(unsigned char* file,
              int size,
              unsigned char* audiofile,
              int audiofsize,
              KinometAviControl* options);
