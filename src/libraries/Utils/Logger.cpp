//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#include "Logger.h"

void Logger::print(std::string text, Mode mode)
{
    if (mode & LOG)
    {
        std::cout << tabs << text << std::endl;
    } else if (mode & WARNING)
    {
        std::cout << tabs << "WARNING: " << text << std::endl;
    } else if (mode & ERROR)
    {
        std::cerr << tabs << text << std::endl;
    }
}

void Logger::tabIn()
{
    indent++;
    calcTab();
}

void Logger::tabOut()
{
    indent = std::max(0, indent-1);
    calcTab();
}

void Logger::changeTab(std::string newTab)
{
    tab = newTab;
}

void Logger::calcTab()
{
    tabs = "";
    for (int i = 0; i < indent; i++)
    {
        tabs += tab;
    }
}
