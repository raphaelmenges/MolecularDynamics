#ifndef MOLECULE_SES_TOROIDAL_PATCH_IMPOSTOR_H
#define MOLECULE_SES_TOROIDAL_PATCH_IMPOSTOR_H

#include <vector>
#include <memory>

#include "ShaderTools/ShaderProgram.h"

#include "DynamicVertexArrayObject.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"


class MoleculeSESToroidalPatchImpostor : public DynamicVertexArrayObject
{
public:
    MoleculeSESToroidalPatchImpostor(std::shared_ptr<Protein> protein);
    virtual ~MoleculeSESToroidalPatchImpostor();

    virtual void draw();
    virtual void enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap);

protected:
    std::size_t numberOfToroidalPatches;
    std::shared_ptr<Protein> protein;
};

#endif // MOLECULE_SES_TOROIDAL_PATCH_IMPOSTOR_H
