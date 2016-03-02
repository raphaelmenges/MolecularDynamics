#pragma once
#include "Protein.h"
#include "Atom.h"
#include <memory>

class SESPatchesProtein : public Protein
{
public:

    void calculatePatches(float probeRadius);
    void recenter();
};
