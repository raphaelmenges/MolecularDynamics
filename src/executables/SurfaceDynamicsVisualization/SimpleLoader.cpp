//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "SimpleLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

// Lookup table for radii
std::map<std::string, float> radiiLookup =
{
{"H", 1.2f}, {"N", 1.55f}, {"C", 1.7f}, {"O", 1.52f}, {"S", 1.8f}, {"P", 1.8f}
};

std::unique_ptr<GPUProtein> parseSimplePDB(std::string filepath, glm::vec3& rMinExtent, glm::vec3& rMaxExtent)
{
    // Initialize min / max extent values
    rMinExtent.x = std::numeric_limits<float>::max();
    rMinExtent.y = std::numeric_limits<float>::max();
    rMinExtent.z = std::numeric_limits<float>::max();
    rMaxExtent.x = std::numeric_limits<float>::min();
    rMaxExtent.y = std::numeric_limits<float>::min();
    rMaxExtent.z = std::numeric_limits<float>::min();

    // Read file
    std::ifstream in(filepath);

    // Create vector to save values
    std::vector<glm::vec4> atoms;

    // Check whether file was found
    if (!in)
    {
        std::cout << "Could not open file: " << filepath << std::endl;
    }
    else
    {
        // Convert input file to string
        std::stringstream strStream;
        strStream << in.rdbuf();
        std::string content = strStream.str();

        // Close file
        in.close();

        // Add line in the end to catch all lines
        content += "\n";

        // Some values for iteration
        std::string lineDelimiter = "\n";
        size_t linePos = 0;
        std::string line;

        // Iterate through lines
        while ((linePos = content.find(lineDelimiter)) != std::string::npos)
        {
            // Extract line
            line = content.substr(0, linePos);
            content.erase(0, linePos + lineDelimiter.length());

            // Check for empty line
            if(line.empty())
            {
                continue;
            }

            // Split by space
            std::string valueDelimiter = " ";
            size_t valuePos = 0;
            std::string value;
            int valueCount = 0;

            // Prepare vec3 for center
            glm::vec3 center;

            // Find values in a line (quite hacky but works for well formatted files)
            while ((valuePos = line.find(valueDelimiter)) != std::string::npos)
            {
                // Extract value
                value = line.substr(0, valuePos);
                line.erase(0, valuePos + valueDelimiter.length());

                // Forget empty values
                if(value.empty())
                {
                    continue;
                }

                // Has to be component of center
                if(valueCount < 3)
                {
                    // Just guess that there are floats
                    center[valueCount] = std::stof(value);
                }
                valueCount++;
            }

            // Decide about min / max
            rMinExtent = glm::min(rMinExtent, center);
            rMaxExtent = glm::max(rMaxExtent, center);

            // Radius
            float radius = radiiLookup.at(line);

            // Add found atom to vectors
            atoms.push_back(glm::vec4(center, radius));
        }
    }

    // Return protein
    return std::move(std::unique_ptr<GPUProtein>(new GPUProtein(atoms)));
}
