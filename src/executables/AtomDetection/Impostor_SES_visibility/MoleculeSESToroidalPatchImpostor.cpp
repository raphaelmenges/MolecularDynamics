#include "MoleculeSESToroidalPatchImpostor.h"


MoleculeSESToroidalPatchImpostor::MoleculeSESToroidalPatchImpostor(std::shared_ptr<Protein> protein)
{
    mode = GL_POINTS;

    this->protein = protein;
    numberOfToroidalPatches = protein->frames.at(0).toroidalPatches.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfToroidalPatches * sizeof(Protein::ToroidalPatch), &this->protein->frames.at(0).toroidalPatches.front(), GL_STATIC_DRAW);
}

MoleculeSESToroidalPatchImpostor::~MoleculeSESToroidalPatchImpostor()
{
    glDeleteBuffers(1, &vertexBufferObjectHandle);
    glDeleteVertexArrays(1, &vertexArrayObjectHandle);
}

void MoleculeSESToroidalPatchImpostor::draw()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);
    glDrawArrays(mode, 0, numberOfToroidalPatches);
}

void MoleculeSESToroidalPatchImpostor::enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap)
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    if (inputMap.find("torus_center") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("torus_center")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), 0);
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("torus_radius") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("torus_radius")->second;
        glVertexAttribPointer(info.location, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("tangent1_center") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("tangent1_center")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("tangent1_radius") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("tangent1_radius")->second;
        glVertexAttribPointer(info.location, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("tangent2_center") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("tangent2_center")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("tangent2_radius") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("tangent2_radius")->second;
        glVertexAttribPointer(info.location, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(11 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }
}

void MoleculeSESToroidalPatchImpostor::updateToroidalPatches()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    numberOfToroidalPatches = protein->frames.at(0).toroidalPatches.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfToroidalPatches * sizeof(Protein::ToroidalPatch), &this->protein->frames.at(0).toroidalPatches.front(), GL_STATIC_DRAW);

}
