#include "MoleculeSESSpherePatchImpostor.h"


MoleculeSESSpherePatchImpostor::MoleculeSESSpherePatchImpostor(std::shared_ptr<Protein> protein)
{
    mode = GL_POINTS;

    this->protein = protein;
    numberOfSpherePatches = protein->frames.at(0).spherePatches.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfSpherePatches * sizeof(Protein::SpherePatch), &this->protein->frames.at(0).spherePatches.front(), GL_STATIC_DRAW);
}

MoleculeSESSpherePatchImpostor::~MoleculeSESSpherePatchImpostor()
{
}

void MoleculeSESSpherePatchImpostor::draw()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);
    glDrawArrays(mode, 0, numberOfSpherePatches);
}

void MoleculeSESSpherePatchImpostor::enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap)
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    if (inputMap.find("probe_position") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("probe_position")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), 0);
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("atom1_position") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("atom1_position")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("atom2_position") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("atom2_position")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("atom3_position") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("atom3_position")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }
}

void MoleculeSESSpherePatchImpostor::updateSpherePatches()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    numberOfSpherePatches = protein->frames.at(0).spherePatches.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfSpherePatches * sizeof(Protein::SpherePatch), &this->protein->frames.at(0).spherePatches.front(), GL_STATIC_DRAW);

}
