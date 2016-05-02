#ifndef SIMPLE_LOADER_H
#define SIMPLE_LOADER_H

#include <string>
#include <vector>
#include "AtomStruct.h"

std::vector<AtomStruct> parseSimplePDB(std::string filepath);

#endif // SIMPLE_LOADER_H
