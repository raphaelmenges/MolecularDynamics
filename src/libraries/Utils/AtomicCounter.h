//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// OpenGL atomic counter.

#ifndef ATOMIC_COUNTER_H
#define ATOMIC_COUNTER_H

#include <GL/glew.h>

class AtomicCounter
{
public:

    // Constructor
    AtomicCounter();

    // Destructor
    virtual ~AtomicCounter();

    // Bind
    void bind(GLuint slot) const;

    // Read
    GLuint read() const;

    // Reset
    void reset() const;

private:

    // Buffer handle
    GLuint mBuffer;
};

#endif // ATOMIC_COUNTER_H
