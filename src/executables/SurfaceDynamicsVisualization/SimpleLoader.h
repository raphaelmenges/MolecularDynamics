#ifndef SIMPLE_LOADER_H
#define SIMPLE_LOADER_H

#include <string>
#include <vector>
#include "SurfaceExtraction/GPUProtein.h"

std::unique_ptr<GPUProtein> parseSimplePDB(std::string filepath, glm::vec3& rMinExtent, glm::vec3& rMaxExtent);

#endif // SIMPLE_LOADER_H
