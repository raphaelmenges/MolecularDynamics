#include "ImpostorSpheres.h"
#include "Molecule/MDtrajLoader/Data/Atom.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

float r_equ(float size) {
    return size * 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - size;
}

float r_pos(float size) {
    return size * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

ImpostorSpheres::ImpostorSpheres(bool prepareWAttribDivisor, bool frontFacesOnly)
    :instancesToRender(num_balls),
      frontFacesOnly(frontFacesOnly),
      prepareWAttribDivisor(prepareWAttribDivisor),
      useProteinData(false)
{
}

void ImpostorSpheres::init()
{
    mode = GL_TRIANGLE_STRIP;

    if (prepareWAttribDivisor)
        this->prepareWithAttribDivisor();
    else
        prepareWithoutAttribDivisor();
}

void ImpostorSpheres::draw() {
    this->drawInstanced(instancesToRender);
}

void ImpostorSpheres::doOcclusionQuery()
{
}

void ImpostorSpheres::drawInstanced(int countInstances)
{
    glBindVertexArray(vertexArrayObjectHandle);
    if(frontFacesOnly)
        glDrawArraysInstanced(mode, 0, 6, countInstances);
    else
        glDrawArraysInstanced(mode, 0, 36, countInstances);

}

void ImpostorSpheres::setProteinData(Protein *prot)
{
    this->prot = prot;
    useProteinData = true;
}

void ImpostorSpheres::copyProteinData()
{
    num_balls = prot->getAtoms()->size();
    for( int i = 0; i < num_balls; i++)
    {
        Atom* a = prot->getAtomAt(i);
        AtomLUT::colorMap::iterator it = AtomLUT::cpk_colorcode.find(a->getElement());
        if(it != AtomLUT::cpk_colorcode.end())
            instance_colors.push_back(glm::vec4(it->second.r,it->second.g,it->second.b,1));
        else
            instance_colors.push_back(glm::vec4(AtomLUT::cpk_colorcode.find("other")->second.r,AtomLUT::cpk_colorcode.find("other")->second.g,AtomLUT::cpk_colorcode.find("other")->second.b,1));

        instance_positions.push_back(glm::vec4(a->getPosition(),AtomLUT::vdW_radii_picometer.find(a->getElement())->second/100.0 + 1.4 )); // pico to angstrom + proberadius
    }

}

void ImpostorSpheres::updateVisibilityMap(std::vector<GLint> map)
{
    visibilityMap = map;
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, visibilityBufferOffset, sizeof(GLfloat) * visibilityMap.size(), &visibilityMap[0]);
    glVertexAttribPointer(3, 1, GL_INT, GL_FALSE, 0, (GLvoid*)(visibilityBufferOffset));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3,1);
}

void ImpostorSpheres::prepareWithAttribDivisor()
{
    // initially all instances are visible
    visibilityMap.resize(num_balls);
    std::fill(visibilityMap.begin(), visibilityMap.end(), 1);

    glGenQueries(1, &occlusionQuery);

    glGenVertexArrays(1, &vertexArrayObjectHandle);
    glBindVertexArray(vertexArrayObjectHandle);


    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    //    GLfloat positions[] = {
    //        -1.0f, -1.0f,
    //        -1.0f, 1.0f,
    //        1.0f, -1.0f,
    //        1.0f, 1.0f
    //    };

    float size = 1;
    GLfloat positions[] = {
        -size,-size,size, size,-size,size, size,size,size,
        size,size,size, -size,size,size, -size,-size,size,
        // Right face
        size,-size,size, size,-size,-size, size,size,-size,
        size,size,-size, size,size,size, size,-size,size,
        // Back face
        -size,-size,-size, size,-size,-size, size,size,-size,
        size,size,-size, -size,size,-size, -size,-size,-size,
        // Left face
        -size,-size,size, -size,-size,-size, -size,size,-size,
        -size,size,-size, -size,size,size, -size,-size,size,
        // Bottom face
        -size,-size,size, size,-size,size, size,-size,-size,
        size,-size,-size, -size,-size,-size, -size,-size,size,
        // Top Face
        -size,size,size, size,size,size, size,size,-size,
        size,size,-size, -size,size,-size, -size,size,size,
    };

    std::vector<GLfloat> instance_colors;
    std::vector<GLfloat> instance_positions;

    for (int i = 0; i < num_balls*4; i+=4) {
        instance_colors.push_back(r_pos(1.0));
        instance_colors.push_back(r_pos(1.0));
        instance_colors.push_back(r_pos(1.0));
        instance_colors.push_back(1);

        instance_positions.push_back(r_equ(30));
        instance_positions.push_back(r_equ(30));
        instance_positions.push_back(r_equ(30));
        instance_positions.push_back(1 + r_equ(0.5));
    }

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(positions) +
                 sizeof(GLfloat) * instance_colors.size() +
                 sizeof(GLfloat) * instance_positions.size() +
                 sizeof(GLfloat) * visibilityMap.size(), NULL, GL_STATIC_DRAW);

    GLuint offset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(positions), positions);
    offset += sizeof(positions);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(GLfloat) * instance_colors.size(), &instance_colors[0]);
    offset += sizeof(GLfloat) * instance_colors.size();
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(GLfloat) * instance_positions.size(), &instance_positions[0]);
    offset += sizeof(GLfloat) * instance_positions.size();
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(GLfloat) * visibilityMap.size(), &visibilityMap[0]);
    visibilityBufferOffset = offset; // need that for update of the buffer

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeof(positions));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(positions) + sizeof(GLfloat) * instance_colors.size()));
    glVertexAttribPointer(3, 1, GL_INT, GL_FALSE, 0, (GLvoid*)(sizeof(positions) + sizeof(GLfloat) * instance_colors.size() + sizeof(GLfloat) * instance_positions.size()));

    //    glVertexAttribPointer(m_positionAttr, 2, GL_FLOAT, GL_FALSE, 0, &m_uniformQuad[0]);
    //    glVertexAttribPointer(m_colorAttr, 4, GL_FLOAT, GL_FALSE, 0, &m_instance_colors[m_currentFrame][0]);
    //    glVertexAttribPointer(m_instancePositionAttr, 4, GL_FLOAT, GL_FALSE, 0, &m_instance_positions[m_currentFrame][0]);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glVertexAttribDivisor(0,0);
    glVertexAttribDivisor(1,1);
    glVertexAttribDivisor(2,1);
    glVertexAttribDivisor(3,1);
}

void ImpostorSpheres::prepareWithoutAttribDivisor()
{
    glGenVertexArrays(1, &vertexArrayObjectHandle);
    glBindVertexArray(vertexArrayObjectHandle);



    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);

    float size = 1;

    //    int positionsSize = 0;
    //    if (frontFacesOnly)
    //        positionsSize = 18;
    //    else
    //        positionsSize = 18*6;
    std::vector<GLfloat> positions;
    //    positions.reserve(positionsSize);
    if(frontFacesOnly){
        positions = {
            -size,-size,size, size,-size,size, size,size,size,
            size,size,size, -size,size,size, -size,-size,size  };

    }
    else
    {
        positions = {
            -size,-size,size, size,-size,size, size,size,size,
            size,size,size, -size,size,size, -size,-size,size,
            // Right face
            size,-size,size, size,-size,-size, size,size,-size,
            size,size,-size, size,size,size, size,-size,size,
            // Back face
            -size,-size,-size, size,-size,-size, size,size,-size,
            size,size,-size, -size,size,-size, -size,-size,-size,
            // Left face
            -size,-size,size, -size,-size,-size, -size,size,-size,
            -size,size,-size, -size,size,size, -size,-size,size,
            // Bottom face
            -size,-size,size, size,-size,size, size,-size,-size,
            size,-size,-size, -size,-size,-size, -size,-size,size,
            // Top Face
            -size,size,size, size,size,size, size,size,-size,
            size,size,-size, -size,size,-size, -size,size,size,
        };
    }


    instance_colors.clear();
    instance_positions.clear();
    if(!useProteinData)
        for (int i = 0; i < num_balls*4; i+=4) {
            instance_colors.push_back(glm::vec4(r_pos(1.0),r_pos(1.0),r_pos(1.0),1));
            instance_positions.push_back(glm::vec4(r_equ(20),r_equ(20),r_equ(20), 10 /*+ r_equ(0.5)*/));
        }
    else
    {
        this->copyProteinData();
    }
    //this->surfaceDetectionTestSet();

    instance_colors_s.instance_colors = instance_colors;
    instance_positions_s.instance_positions = instance_positions;

    //glBufferData(GL_ARRAY_BUFFER, sizeof(positions), NULL, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * positions.size(), NULL, GL_STATIC_DRAW);
    GLuint offset = 0;
    //glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(positions), &positions[0]);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(GLfloat) * positions.size(), &positions[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0,0);
}

void ImpostorSpheres::surfaceDetectionTestSet()
{
    instance_positions.clear();
    instance_colors.clear();

    for(int n = 0; n < num_balls; n++)
    {
        instance_colors.push_back(glm::vec4((n%3)/3.0,((n+1)%3)/3.0,((n+2)%3)/3.0,1));
    }
    //left bottom
    instance_positions.push_back(glm::vec4(-1,-1,-1,2));
    instance_positions.push_back(glm::vec4(-1,-1,1,2));
    instance_positions.push_back(glm::vec4(-1,-1,0,2));
    //left middle
    instance_positions.push_back(glm::vec4(-1,0,-1,2));
    instance_positions.push_back(glm::vec4(-1,0,1,2));
    instance_positions.push_back(glm::vec4(-1,0,0,2));
    //left top
    instance_positions.push_back(glm::vec4(-1,1,-1,2));
    instance_positions.push_back(glm::vec4(-1,1,1,2));
    instance_positions.push_back(glm::vec4(-1,1,0,2));


    //middle bottom
    instance_positions.push_back(glm::vec4(0,-1,-1,2));
    instance_positions.push_back(glm::vec4(0,-1,1,2));
    instance_positions.push_back(glm::vec4(0,-1,0,2));
    //middle middle
    instance_positions.push_back(glm::vec4(0,0,-1,2));
    instance_positions.push_back(glm::vec4(0,0,1,2));
    instance_positions.push_back(glm::vec4(0,0,0,2));
    //middle top
    instance_positions.push_back(glm::vec4(0,1,-1,2));
    instance_positions.push_back(glm::vec4(0,1,1,2));
    instance_positions.push_back(glm::vec4(0,1,0,2));


    //right bottom
    instance_positions.push_back(glm::vec4(1,-1,-1,2));
    instance_positions.push_back(glm::vec4(1,-1,1,2));
    instance_positions.push_back(glm::vec4(1,-1,0,2));
    //right middle
    instance_positions.push_back(glm::vec4(1,0,-1,2));
    instance_positions.push_back(glm::vec4(1,0,1,2));
    instance_positions.push_back(glm::vec4(1,0,0,2));
    //right top
    instance_positions.push_back(glm::vec4(1,1,-1,2));
    instance_positions.push_back(glm::vec4(1,1,1,2));
    instance_positions.push_back(glm::vec4(1,1,0,2));
}

