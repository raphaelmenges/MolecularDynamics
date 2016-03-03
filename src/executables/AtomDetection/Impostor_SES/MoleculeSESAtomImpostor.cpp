#include "MoleculeSESAtomImpostor.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"


MoleculeSESAtomImpostor::MoleculeSESAtomImpostor(std::shared_ptr<Protein> protein)
{
    mode = GL_POINTS;

    this->protein = protein;
    numberOfAtoms = protein->frames.at(0).atoms.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfAtoms * sizeof(Protein::SimpleAtom), &this->protein->frames.at(0).atoms.front(), GL_STATIC_DRAW);

}

MoleculeSESAtomImpostor::~MoleculeSESAtomImpostor()
{
}

void MoleculeSESAtomImpostor::draw()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);
    glDrawArrays(mode, 0, numberOfAtoms);
}

void MoleculeSESAtomImpostor::enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap)
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    if (inputMap.find("atom_position") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("atom_position")->second;
        glVertexAttribPointer(info.location, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glEnableVertexAttribArray(info.location);
    }

    if (inputMap.find("atom_radius") != inputMap.end())
    {
        ShaderProgram::Info info = inputMap.find("atom_radius")->second;
        glVertexAttribPointer(info.location, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(info.location);
    }
}

void MoleculeSESAtomImpostor::updateAtoms()
{
    glBindVertexArray(vertexArrayObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);

    numberOfAtoms = protein->frames.at(0).atoms.size();

    glBufferData(GL_ARRAY_BUFFER, numberOfAtoms * sizeof(Protein::SimpleAtom), &this->protein->frames.at(0).atoms.front(), GL_STATIC_DRAW);
}
