#ifndef MOLECULE_SES_ATOM_IMPOSTOR_H
#define MOLECULE_SES_ATOM_IMPOSTOR_H

#include <vector>
#include <memory>

#include "ShaderTools/ShaderProgram.h"

#include "DynamicVertexArrayObject.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"


class MoleculeSESAtomImpostor : public DynamicVertexArrayObject
{
public:
    MoleculeSESAtomImpostor(std::shared_ptr<Protein> protein);
    virtual ~MoleculeSESAtomImpostor();

    virtual void draw();
    virtual void enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap);

protected:
    std::size_t numberOfAtoms;
    std::shared_ptr<Protein> protein;
};

#endif // MOLECULE_SES_ATOM_IMPOSTOR_H
