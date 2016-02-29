#ifndef ATOMLUT_H
#define ATOMLUT_H

#include <map>

/**
 * @brief The AtomLUT class
 * Holds atom radii and standard colors
 */
class AtomLUT{

public:

    struct color{
        float r;
        float g;
        float b;
    };

    typedef std::map< std::string, int> radiiMap;
    typedef std::map< std::string, color> colorMap;
    static radiiMap vdW_radii_picometer;
    static colorMap cpk_colorcode;
};


#endif // ATOMLUT_H
