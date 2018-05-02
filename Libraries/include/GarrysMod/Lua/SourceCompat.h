#ifndef GARRYSMOD_LUA_SOURCECOMPAT_H
#define GARRYSMOD_LUA_SOURCECOMPAT_H

#ifdef GMOD_USE_SOURCESDK
    #include "mathlib/vector.h"
#else
    struct Vector
    {
        float x, y, z;
    };

    struct QAngle
    {
        float x, y, z;
    };
#endif

#endif