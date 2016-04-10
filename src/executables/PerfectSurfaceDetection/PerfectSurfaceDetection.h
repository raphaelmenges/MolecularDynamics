#ifndef PERFECT_SURFACE_DETECTION_H_
#define PERFECT_SURFACE_DETECTION_H_

#include <memory>

// Forward declaration instead of including (saved compile time)
class Protein;

class PerfectSurfaceDetection
{
public:

    // Constructor
    PerfectSurfaceDetection();

private:

    // Members
    std::unique_ptr<Protein> mupProtein;
};

#endif // PERFECT_SURFACE_DETECTION_H_
