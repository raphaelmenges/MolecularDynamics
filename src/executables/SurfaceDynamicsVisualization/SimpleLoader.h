#ifndef SIMPLE_LOADER_H
#define SIMPLE_LOADER_H

#include <string>
#include <vector>
#include "AtomStruct.h"

std::vector<AtomStruct> parseSimplePDB(std::string filepath, glm::vec3& rMinExtent, glm::vec3& rMaxExtent);

#endif // SIMPLE_LOADER_H
