#ifndef MOLECULE_SES_SPHERE_PATCH_IMPOSTOR_H
#define MOLECULE_SES_SPHERE_PATCH_IMPOSTOR_H

#include <vector>
#include <memory>

#include "ShaderTools/ShaderProgram.h"

#include "DynamicVertexArrayObject.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"


class MoleculeSESSpherePatchImpostor : public DynamicVertexArrayObject
{
public:
    MoleculeSESSpherePatchImpostor(std::shared_ptr<Protein> protein);
    virtual ~MoleculeSESSpherePatchImpostor();

    virtual void draw();
    virtual void enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap);

protected:
    std::size_t numberOfSpherePatches;
    std::shared_ptr<Protein> protein;
};

#endif // MOLECULE_SES_SPHERE_PATCH_IMPOSTOR_H
