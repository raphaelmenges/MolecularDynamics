#include "SimpleLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

// TODO: very unstable :D

std::map<std::string, float> radiiLookup =
{
{"H", 1.2f}, {"N", 1.55f}, {"C", 1.7f}, {"O", 1.52f}, {"S", 1.8f}
};

std::vector<AtomStruct> parseSimplePDB(std::string filepath)
{
    // Read file
    std::ifstream in(filepath);

    // Check whether file was found
    if (!in)
    {
        std::cout << "Could not open file: " << filepath << std::endl;
    }

    // Convert input file to string
    std::stringstream strStream;
    strStream << in.rdbuf();
    std::string content = strStream.str();

    // Close file
    in.close();

    // Some values for iteration
    std::string lineDelimiter = "\n";
    size_t linePos = 0;
    std::string line;

    // Create vector
    std::vector<AtomStruct> atoms;

    // Iterate through lines
    while ((linePos = content.find(lineDelimiter)) != std::string::npos)
    {
        // Extract line
        line = content.substr(0, linePos);
        content.erase(0, linePos + lineDelimiter.length());

        // Split by space
        std::string valueDelimiter = " ";
        size_t valuePos = 0;
        std::string value;
        int valueCount = 0;
        glm::vec3 center;
        while ((valuePos = line.find(valueDelimiter)) != std::string::npos)
        {
            // Extract value
            value = line.substr(0, valuePos);
            line.erase(0, valuePos + valueDelimiter.length());

            // Forgot empty values
            if(value.empty())
            {
                continue;
            }

            // Has to be component of center
            if(valueCount < 3)
            {
                center[valueCount] = std::stof(value);
            }
            valueCount++;
        }
        float radius = radiiLookup.at(line);

        // Add found atom to vector
        atoms.push_back(AtomStruct(center, radius));
    }

    // Return protein
    return atoms;
}
