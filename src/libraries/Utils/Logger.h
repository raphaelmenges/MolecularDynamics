//
// Created by ubundrian on 21.06.16.
//

#ifndef OPENGL_FRAMEWORK_LOGGER_H
#define OPENGL_FRAMEWORK_LOGGER_H

#include <string>
#include <iostream>

class Logger {
public:
    static Logger& instance()
    {
        static Logger _instance;
        return _instance;
    }
    ~Logger() {}

    enum Mode
    {
        LOG     = (1u << 0),
        WARNING = (1u << 1),
        ERROR   = (1u << 2)
    };

    void print(std::string text, Mode mode = LOG);
    void tabIn();
    void tabOut();
    void changeTab(std::string newTab);
private:
    Logger()
    {
        filter = LOG | WARNING | ERROR;
    }                                       // disable creating an object of this class
    Logger( const Logger& );                // disable copy constructor
    Logger & operator = (const Logger &);   // disable new instance by copy

    void calcTab();

    int indent = 0;
    std::string tab = "    ";
    std::string tabs = "";
    bool printToFile = false;
    unsigned int filter;
};


#endif //OPENGL_FRAMEWORK_LOGGER_H
