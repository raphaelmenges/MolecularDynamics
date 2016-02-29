#include "AtomLUT.h"

AtomLUT::radiiMap AtomLUT::vdW_radii_picometer = {
    {"aluminium", 184}, {"antimony",206}, {"argon",188 }, { "arsenic",185},
    { "astatine",202}, { "barium",268}, { "beryllium",153}, { "bismuth",207},
    { "boron",192}, { "bromine",185}, {"cadmium",158 }, {"caesium",343},
    { "calcium",231}, {"carbon",170 }, {"chlorine",175 }, {"copper",140 },
    {"fluorine",147 }, {"francium",348 }, {"gallium",187 }, {"germanium",211 },
    {"gold",166 }, {"helium",140 }, {"hydrogen",120 }, {"indium",193 },
    {"iodine",198 }, {"krypton",202 }, {"lead",202 }, {"magnesium",173 },
    {"mercury",155 }, {"neon",154 }, {"nickel",163 }, {"nitrogen",155 },
    {"oxygen",152 }, {"palladium",163 }, {"phosphorus",180 }, {"platinum",175 },
    { "polonium",197}, {"potassium",275 }, {"radium",283 }, {"radon",220 },
    {"rubidium",303 }, {"scandium",211 }, {"selenium",190 }, {"silicon",210 },
    {"silver",172 }, {"sodium",227 }, {"strontium",249 }, {"sulfur",180 },
    {"tellurium",206 }, {"thallium",196 }, {"tin",217 }, {"uranium",186 },
    {"xenon",216 }, {"zinc",139 }
};

AtomLUT::colorMap AtomLUT::cpk_colorcode = {
    {"hydrogen", AtomLUT::color{1,1,1}}, {"carbon", AtomLUT::color{0,0,0}},
    {"nitrogen", AtomLUT::color{0.5,0.8,1}}, {"oxygen", AtomLUT::color{1,0,0}},
    {"fluorine ", AtomLUT::color{0,1,0}}, {"chlorine", AtomLUT::color{0,1,0}},
    {"bromine", AtomLUT::color{.6,.2,.2}}, {"iodine", AtomLUT::color{.7,.2,.9}},
    {"phosphorus", AtomLUT::color{1,.5,0}}, {"sulfur", AtomLUT::color{1,1,0}},
    {"boron", AtomLUT::color{1,.9,.7}}, {"titanium", AtomLUT::color{.7,.7,.7}},
    {"iron", AtomLUT::color{.8,0.4,.1}}, {"other", AtomLUT::color{.9,.5,.9}}
};


