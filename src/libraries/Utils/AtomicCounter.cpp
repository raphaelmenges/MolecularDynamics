#include "AtomicCounter.h"
#include <cstring>

AtomicCounter::AtomicCounter()
{
    glGenBuffers(1, &mBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Initial reset
    reset();
}

AtomicCounter::~AtomicCounter()
{
    glDeleteBuffers(1, &mBuffer);
}

void AtomicCounter::bind(GLuint slot) const
{
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, slot, mBuffer);
}

GLuint AtomicCounter::read() const
{
    // Read atomic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mBuffer);

    GLuint *mapping = (GLuint*)glMapBufferRange(
        GL_ATOMIC_COUNTER_BUFFER,
        0,
        sizeof(GLuint),
        GL_MAP_READ_BIT);

    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    return mapping[0];
}

void AtomicCounter::reset() const
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, mBuffer);

    // Map the buffer
    GLuint* mapping = (GLuint*)glMapBufferRange(
        GL_ATOMIC_COUNTER_BUFFER,
        0 ,
        sizeof(GLuint),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

    // Set memory to new value
    memset(mapping, 0, sizeof(GLuint));

    // Unmap the buffer
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}
