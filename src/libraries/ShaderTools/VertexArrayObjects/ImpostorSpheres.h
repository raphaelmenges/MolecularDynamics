#ifndef ImpostorSpheres_H
#define ImpostorSpheres_H

#include "../VertexArrayObject.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"

class ImpostorSpheres : public VertexArrayObject {
public:
    ImpostorSpheres(bool prepareWAttribDivisor, bool frontFacesOnly);
    void init();
    void draw();
    void doOcclusionQuery();
    void drawInstanced(int countInstances);

    void setProteinData(Protein* prot);

    Protein* prot;
    bool useProteinData;
    void copyProteinData();

    GLuint occlusionQuery;
    GLuint positionBuffer;

    std::vector<GLint> visibilityMap;
    GLuint visibilityBufferOffset;

    void updateVisibilityMap(std::vector<GLint> map);

    int num_balls = 2000;//16384;

    void prepareWithAttribDivisor();
    void prepareWithoutAttribDivisor();

    std::vector<glm::vec4> instance_colors;
    std::vector<glm::vec4> instance_positions;

    struct instance_colors_t
    {
        std::vector<glm::vec4> instance_colors;
    };
    struct instance_positions_t
    {
        std::vector<glm::vec4> instance_positions;
    };

    instance_colors_t instance_colors_s;
    instance_positions_t instance_positions_s;

    unsigned int instancesToRender;

    bool prepareWAttribDivisor;
    bool frontFacesOnly;

    void surfaceDetectionTestSet();
};

#endif // ImpostorSpheres_H
