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
    {"hydrogen", AtomLUT::color{1.f, 1.f, 1.f}}, {"carbon", AtomLUT::color{200.f/255.f, 200.f/255.f, 200.f/255.f}},
    {"nitrogen", AtomLUT::color{143.f/255.f, 143.f/255.f, 1.f}}, {"oxygen", AtomLUT::color{240.f/255.f, 0.f, 0.f}},
    {"fluorine ", AtomLUT::color{0.f, 1.f, 0.f}}, {"chlorine", AtomLUT::color{0.f, 1.f, 0.f}},
    {"bromine", AtomLUT::color{165.f/255.f, 42.f/255.f, 42.f/255.f}}, {"iodine", AtomLUT::color{0.7f, 0.2f, 0.9f}},
    {"phosphorus", AtomLUT::color{1.f, 165.f/255.f, 0.f}}, {"sulfur", AtomLUT::color{1.f, 200.f/255.f, 50.f/255.f}},
    {"boron", AtomLUT::color{1.f, 0.9f, 0.7f}}, {"titanium", AtomLUT::color{0.7f, 0.7f, 0.7f}},
    {"iron", AtomLUT::color{1.f, 165.f/255.f, 0.f}}, {"other", AtomLUT::color{1.f, 20.f/255.f, 147.f/255.f}}
};

// Taken from here: http://acces.ens-lyon.fr/biotic/rastop/help/colour.htm
// If color not contained, use (0.745098039, 0.62745098, 0.431372549)
AtomLUT::colorMap AtomLUT::amino_colorcode = {
    {"ASP", AtomLUT::color{0.901960784,0.901960784,0.039215686}}, {"GLU", AtomLUT::color{0.901960784,0.901960784,0.039215686}},
    {"CYS", AtomLUT::color{0.901960784,0.901960784,0}}, {"MET", AtomLUT::color{0.901960784,0.901960784,0}},
    {"LYS", AtomLUT::color{0.078431373,0.352941176,1}}, {"ARG", AtomLUT::color{0.078431373,0.352941176,1}},
    {"SER", AtomLUT::color{0.980392157,0.588235294,0}}, {"THR", AtomLUT::color{0.980392157,0.588235294,0}},
    {"PHE", AtomLUT::color{0.196078431,0.196078431,0.666666667}}, {"TYR", AtomLUT::color{0.196078431,0.196078431,0.666666667}},
    {"ASN", AtomLUT::color{0,0.862745098,0.862745098}}, {"GLN", AtomLUT::color{0,0.862745098,0.862745098}},
    {"GLY", AtomLUT::color{0.921568627,0.921568627,0.921568627}}, {"LEU", AtomLUT::color{0.058823529,0.509803922,0.058823529}},
    {"VAL", AtomLUT::color{0.058823529,0.509803922,0.058823529}}, {"ILE", AtomLUT::color{0.058823529,0.509803922,0.058823529}},
    {"ALA", AtomLUT::color{0.784313725,0.784313725,0.784313725}}, {"TRP", AtomLUT::color{0.705882353,0.352941176,0.705882353}},
    {"HIS", AtomLUT::color{0.509803922,0.509803922,0.823529412}}, {"PRO", AtomLUT::color{0.862745098,0.588235294,0.509803922}},
};

AtomLUT::color AtomLUT::fetchAminoColor(std::string name)
{
    AtomLUT::colorMap::iterator it = AtomLUT::amino_colorcode.find(name);
    if(it != AtomLUT::amino_colorcode.end())
    {
       return it->second;
    }
    else
    {
        return AtomLUT::color{0.745098039, 0.62745098, 0.431372549};
    }
}

