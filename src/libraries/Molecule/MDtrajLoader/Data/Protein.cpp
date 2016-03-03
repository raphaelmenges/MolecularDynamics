// Fill out your copyright notice in the Description page of Project Settings.

//#include "PharmaCV.h"
#include "Protein.h"
#include <iostream>

Protein::Protein(std::vector<std::string> &names,
                 std::vector<std::string> &elementNames, std::vector<std::string> &residueNames,
                 std::vector<int> &indices, std::vector<std::string> &bonds, std::vector<std::vector<glm::vec3>> &positions,
                 std::string name, int numAtoms, std::vector<std::string> &distinctResidueNames, std::vector<float> &radii)
{
    radii_ = radii;
    std::string old = distinctResidueNames.at(0);
    std::string newer = distinctResidueNames.at(0);
    std::vector<Atom*> atomVector;
    aminoNames_.push_back(old);


    min_[0] = std::numeric_limits<float>::max();
    min_[1] = std::numeric_limits<float>::max();
    min_[2] = std::numeric_limits<float>::max();

    aminoAndItsAtoms_.insert(make_pair(old, atomVector));

    max_[0] = std::numeric_limits<float>::min();
    max_[1] = std::numeric_limits<float>::min();
    max_[2] = std::numeric_limits<float>::min();

    minMax();

    //UE_LOG(LogTemp, Warning, TEXT("maxx vorher %f"), max_.X);
    //UE_LOG(LogTemp, Warning, TEXT("maxy %f"), max_.Y);
    //UE_LOG(LogTemp, Warning, TEXT("maxz %f"), max_.Z);

    //UE_LOG(LogTemp, Warning, TEXT("minx %f"), min_.X);
    //UE_LOG(LogTemp, Warning, TEXT("miny %f"), min_.Y);
    //UE_LOG(LogTemp, Warning, TEXT("minz %f"), min_.Z);


    for (int i = 0; i < names.size(); i++) {
        distinctResidueNames.at(i);
        std::string tmpstr = distinctResidueNames.at(i);
        std::string amino = tmpstr.substr(0, 3);
        Atom* a = new Atom(names.at(i), elementNames.at(i), indices.at(i), positions[0][i], distinctResidueNames.at(i), amino, this);

        Protein::atoms_.push_back(a);
        newer = distinctResidueNames.at(i);
        //if not the same
        if (old.compare(newer)!=0) {
            std::vector<Atom*> b;
            aminoAndItsAtoms_.insert(make_pair(newer, b));
            old = newer;
            aminoNames_.push_back(old);

        }

        aminoAndItsAtoms_[old].push_back(a);
    }
    //get number of different amino acids and put them into a vector
    std::set<std::string> aminoAcids;
    for (int i = 0; i < residueNames.size(); i++) {
        aminoAcids.insert(residueNames.at(i));
    }
    //UE_LOG(LogTemp, Warning, TEXT("size : %d"), positions.size());

    // positions per frame
    for (int i = 0; i < positions[0].size(); i++) {
        for (int j = 1; j < positions.size();j++) {
            atoms_[i]->setNextPosition(positions[j][i]);
        }
    }

    diffAminos_.assign(aminoAcids.begin(), aminoAcids.end());

    //setBonds(bonds);

}

Protein::~Protein()
{

    int delete_counter = 0;
    for (auto it = atoms_.begin(); it != atoms_.end(); ) {
        delete *it;
        it = atoms_.erase(it);
        delete_counter++;
    }
    //UE_LOG(LogTemp, Warning, TEXT("Destroyed %i Atoms of Protein %s."),delete_counter, UTF8_TO_TCHAR(name_.c_str()));
}

std::string Protein::getName() {
    return name_;
}

Atom* Protein::getAtomAt(int i) {
    return atoms_.at(i);
}

std::vector<Atom*>* Protein::getAtoms() {
    return &atoms_;
}

std::vector<Atom*>* Protein::getAtomsFromAmino(std::string name) {
    return &aminoAndItsAtoms_[name] ;
}


std::vector<std::string>* Protein::getAminoNames() {
    return &aminoNames_;
}

std::vector<std::string>* Protein::getDiffAminos() {
    return &diffAminos_;
}

glm::vec3 Protein::getMin() {
    return min_;
}

glm::vec3 Protein::getMax() {
    return max_;
}

void Protein::minMax() {

    //get minimum and maximum
    min_[0] = std::numeric_limits<float>::max();
    min_[1] = std::numeric_limits<float>::max();
    min_[2] = std::numeric_limits<float>::max();

    max_[0] = std::numeric_limits<float>::min();
    max_[1] = std::numeric_limits<float>::min();
    max_[2] = std::numeric_limits<float>::min();

    for (int j = 0; j < atoms_.size(); j++) {

        glm::vec3 position = atoms_.at(j)->getPosition();
        for (int i = 0; i < 3; i++) {
            if (position[i] < min_[i]) {
                min_[i] = position[i];
            }
            if (position[i] > max_[i]) {
                max_[i] = position[i];
            }
            min_[i] = floor(min_[i]);									//now uses round (important!)
            max_[i] = ceil(max_[i]);
        }
    }

}

std::vector<float> Protein::getRadii() const
{
    return radii_;
}

float Protein::getRadiusAt(int i)
{
    return radii_.at(i);
}

//---------------------------------------------private

void Protein::setBonds(std::vector<std::string> bonds) {

    for (int b = 0; b < bonds.size(); b++) {
        //bonds cutting the atom names and amino names
        std::string bond = bonds[b];
        //get both bond partners
        int posKomma = bond.find(",");
        std::string bond1 = bond.substr(1, posKomma - 1);
        std::string bond2 = bond.substr(posKomma + 2);
        //get amino name and atom name from both partners
        int posStrich = bond1.find("-");
        std::string bond1Amino = bond1.substr(0, posStrich);
        std::string bond1Atom = bond1.erase(0, posStrich + 1);

        posStrich = bond2.find("-");
        std::string bond2Amino = bond2.substr(0, posStrich);
        //FString happyStringBond2Amino = bond2Amino.c_str();
        bond2.erase(0, posStrich + 1);
        int posKlammer = bond2.find(")");
        std::string bond2Atom = bond2.substr(0, posKlammer);


        //search for the two appropriate atoms with distinct aminos from the bond
        //find all atoms_, that have distinct amino from bond 1 and 2
        std::vector<Atom*> atomsBond1Amino;
        for (int i = 0; i < atoms_.size(); i++) {
            if (atoms_[i]->getDistinctResidue() == bond1Amino) {
                atomsBond1Amino.push_back(atoms_[i]);
            }
        }
        std::vector<Atom*> atomsBond2Amino;
        for (int i = 0; i < atoms_.size(); i++) {
            if (atoms_[i]->getDistinctResidue() == bond2Amino) {
                atomsBond2Amino.push_back(atoms_[i]);
            }
        }

        //find the specific atom with atom name from bond 1 and 2
        Atom* atomBond1 = NULL;
        for (int i = 0; i < atomsBond1Amino.size(); i++) {
            if (atomsBond1Amino[i]->getName() == bond1Atom) {
                atomBond1 = atomsBond1Amino[i];
            }
        }
        Atom* atomBond2 = NULL;
        for (int i = 0; i < atomsBond2Amino.size(); i++) {
            if (atomsBond2Amino[i]->getName() == bond2Atom) {
                atomBond2 = atomsBond2Amino[i];
            }
        }

        //add each other to the bond list of the other
        if (atomBond1 != NULL && atomBond2 != NULL) {
            atomBond1->addBondPartner(atomBond2);
            atomBond2->addBondPartner(atomBond1);
        }
        else {
            //UE_LOG(LogTemp, Error, TEXT("Toll gemacht, bond wurde verkackt!"));
        }

    }

    numAtoms_ = atoms_.size();
    //UE_LOG(LogTemp, Warning, TEXT("numAtoms_ : %d"), numAtoms_);

}

void Protein::recenter()
{
    glm::vec3 sum = glm::vec3(0.0f);
    for (auto const& atom : atoms_) sum += atom->getPosition();
    sum /= atoms_.size();
    for (auto & atom : atoms_) atom->setXYZ(atom->getPosition() - sum);
}

void Protein::setupSimpleAtoms()
{
    Frame frame;
    for (int i = 0; i < atoms_.size(); i++)
    {
        SimpleAtom sAtom;
        sAtom.position = atoms_.at(i)->getPosition();
        sAtom.radius = this->getRadiusAt(i);
        frame.atoms.push_back(sAtom);
    }
    frames.push_back(frame);
}

void Protein::setupSimpleAtoms(unsigned int *collectedVisibleIDs)
{
    Frame frame;
    for (int i = 0; i < atoms_.size(); i++)
    {
        if (collectedVisibleIDs[i] != 1)
            continue;
        SimpleAtom sAtom;
        sAtom.position = atoms_.at(i)->getPosition();
        sAtom.radius = this->getRadiusAt(i);
        frame.atoms.push_back(sAtom);
    }
    frames.clear();
    frames.push_back(frame);
    //std::cout << "visible atom count:\t" << frame.atoms.size() << std::endl;
}



void Protein::calculatePatches(float probeRadius)
{

    int T = 0; // timestep

    std::cout << "overall atom count:\t" << atoms_.size() << std::endl;

    frames[T].spherePatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 2; i++)
    {
        for (std::size_t j = i + 1; j < atoms_.size() - 1; j++)
        {
            for (std::size_t k = j + 1; k < atoms_.size(); k++)
            {
                glm::vec3 atom1_pos = atoms_[i]->getPosition();
                float     atom1_r   = this->getRadiusAt(i) + probeRadius;
                float     atom1_rr  = atom1_r * atom1_r;
                glm::vec3 atom2_pos = atoms_[j]->getPosition();
                float     atom2_r   = this->getRadiusAt(j) + probeRadius;
                float     atom2_rr  = atom2_r * atom2_r;
                glm::vec3 atom3_pos = atoms_[k]->getPosition();
                float     atom3_r   = this->getRadiusAt(k) + probeRadius;
                float     atom3_rr  = atom3_r * atom3_r;

                float     d_a1_a2   = glm::distance(atom1_pos, atom2_pos);
                float     d_a1_a3   = glm::distance(atom1_pos, atom3_pos);
                float     d_a2_a3   = glm::distance(atom2_pos, atom3_pos);


                if (d_a1_a2 < atom1_r + atom2_r && d_a1_a2 + atom1_r > atom2_r && d_a1_a2 + atom2_r > atom1_r &&
                        d_a1_a3 < atom1_r + atom3_r && d_a1_a3 + atom1_r > atom3_r && d_a1_a3 + atom3_r > atom1_r &&
                        d_a2_a3 < atom2_r + atom3_r && d_a2_a3 + atom2_r > atom3_r && d_a2_a3 + atom3_r > atom2_r)
                {
                    glm::vec3 omega   = atom1_pos;
                    glm::vec3 omega_x = glm::normalize(atom2_pos - atom1_pos);
                    float     i       = glm::dot(omega_x, atom3_pos - atom1_pos);
                    glm::vec3 omega_y = glm::normalize(atom3_pos - atom1_pos - i * omega_x);
                    glm::vec3 omega_z = glm::normalize(glm::cross(omega_x, omega_y));
                    float     d       = glm::length(atom2_pos - atom1_pos);
                    float     j       = glm::dot(omega_y, atom3_pos - atom1_pos);

                    float     x = (atom1_rr - atom2_rr + d * d) / (2 * d);

                    float     radius_help_sphere = glm::sqrt(glm::abs(atom1_rr - x * x));
                    float     distance_help_sphere_i = i - x;

                    if (glm::abs(distance_help_sphere_i) < atom3_r)
                    {
                        float     atom3_r_new = glm::sqrt(glm::abs(atom3_rr - distance_help_sphere_i * distance_help_sphere_i));

                        if (radius_help_sphere + atom3_r_new > j && radius_help_sphere + j > atom3_r_new && atom3_r_new + j > radius_help_sphere)
                        {
                            float y = (radius_help_sphere * radius_help_sphere - atom3_r_new * atom3_r_new + j * j) / (2 * j);
                            float z_abs = glm::sqrt(glm::abs(radius_help_sphere * radius_help_sphere - y * y));

                            glm::vec3 probe1_pos = omega + x * omega_x + y * omega_y + z_abs * omega_z;
                            glm::vec3 probe2_pos = omega + x * omega_x + y * omega_y - z_abs * omega_z;

                            SpherePatch sp1;
                            sp1.probe_position = probe1_pos;
                            sp1.atom1_position = atom1_pos;
                            sp1.atom2_position = atom2_pos;
                            sp1.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp1);

                            SpherePatch sp2;
                            sp2.probe_position = probe2_pos;
                            sp2.atom1_position = atom2_pos; // atom1 and atom2 are switched to keep the same winding order inside the geometry shader
                            sp2.atom2_position = atom1_pos;
                            sp2.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp2);


                            float d1 =glm::length(probe1_pos - atom1_pos);
                            float d2 =glm::length(probe1_pos - atom2_pos);
                            float d3 =glm::length(probe1_pos - atom3_pos);

                            if (glm::abs(d1 - atom1_r) > 0.001f || glm::abs(d2 - atom2_r) > 0.001f || glm::abs(d3 - atom3_r) > 0.001f)
                            {
                                //std::cout << "atom id: " << i << j << k << std::endl;
                                std::cout << "new" << std::endl;
                                std::cout << "\t" << "dist all: " << glm::abs(d1 - atom1_r) << "  " <<  glm::abs(d2 - atom2_r) << "  " <<  glm::abs(d3 - atom3_r)<< std::endl;
                                std::cout << "\t" << "dist a1: " << d1 << "  " <<  atom1_r << std::endl;
                                std::cout << "\t" << "dist a2: " << d2 << "  " <<  atom2_r << std::endl;
                                std::cout << "\t" << "dist a3: " << d3 << "  " <<  atom3_r << std::endl;
                                /*
                                std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << "\t" << atom1_r << std::endl;
                                std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << "\t" << atom2_r << std::endl;
                                std::cout << "\t" << "a3: " << atom3_pos.x << "\t" << atom3_pos.y << "\t" << atom3_pos.z << "\t" << atom3_r << std::endl;

                                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                                std::cout << "\t" << "d_a1_i1:\t" << d_a1_i1 << std::endl;
                                std::cout << "\t" << "r_i1:\t" << r_i1 << std::endl;
                                std::cout << "\t" << "omega_x:\t" << omega_x.x << "\t" << omega_x.y << "\t" << omega_x.z << std::endl;
                                std::cout << "\t" << "d_a1_o:\t" << d_a1_o << std::endl;
                                std::cout << "\t" << "v_a1_o:\t" << v_a1_o.x << "\t" << v_a1_o.y << "\t" << v_a1_o.z << std::endl;
                                std::cout << "\t" << "omega:\t" << omega.x << "\t" << omega.y << "\t" << omega.z << std::endl;
                                std::cout << "\t" << "v_o_a3:\t" << v_o_a3.x << "\t" << v_o_a3.y << "\t" << v_o_a3.z << std::endl;
                                std::cout << "\t" << "d_o_a3:\t" << d_o_a3 << std::endl;
                                std::cout << "\t" << "omega_y:\t" << omega_y.x << "\t" << omega_y.y << "\t" << omega_y.z << std::endl;
                                std::cout << "\t" << "omega_z:\t" << omega_z.x << "\t" << omega_z.y << "\t" << omega_z.z << std::endl;
                                std::cout << "\t" << "d_i1_o:\t" << d_i1_o << std::endl;
                                std::cout << "\t" << "r_a3_i1:\t" << r_a3_i1 << std::endl;
                                std::cout << "\t" << "d_i1_i2:\t" << d_i1_i2 << std::endl;
                                std::cout << "\t" << "r_i2:\t" << r_i2 << std::endl;
    */
                            }

                            //std::cout << "\t" << "p1: " << probe1_pos.x << "\t" << probe1_pos.y << "\t" << probe1_pos.z << std::endl;
                            //std::cout << "\t" << "p2: " << probe2_pos.x << "\t" << probe2_pos.y << "\t" << probe2_pos.z << std::endl;
                            //std::cout << "\t" << "dist: " << glm::length(probe2_pos - atom1_pos) << "\t" <<  glm::length(probe2_pos - atom2_pos) << "\t" << glm::length(probe2_pos - atom3_pos) << std::endl;
                        }
                    }
                }
            }
        }
    }

    std::cout << "sphere patch count:\t" << frames[T].spherePatches.size() << std::endl;


    frames[T].toroidalPatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 1; i++)
    {
        for (std::size_t j = i + 1; j < atoms_.size(); j++)
        {
            glm::vec3 atom1_pos = atoms_[i]->getPosition();
            float     atom1_r   = this->getRadiusAt(i) + probeRadius;
            float     atom1_rr  = atom1_r * atom1_r;
            glm::vec3 atom2_pos = atoms_[j]->getPosition();
            float     atom2_r   = this->getRadiusAt(j) + probeRadius;
            float     atom2_rr  = atom2_r * atom2_r;

            /*
            std::cout << "i: " << i << " j: " << j << std::endl;

            std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << std::endl;
            std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << std::endl;
            std::cout << "\t" << "a: " << glm::distance(atom1_pos, atom2_pos) << "\t" << atom1_r << "\t" << atom2_r << "\t" << std::endl;
*/

            if (glm::distance(atom1_pos, atom2_pos) < atom1_r + atom2_r)
            {
                //std::cout << "YES!   i: " << i << " j: " << j << std::endl;

                glm::vec3 v_a1_a2   = atom2_pos - atom1_pos;
                glm::vec3 v_a1_a2_n = glm::normalize(v_a1_a2);
                float     d_a1_a2   = glm::length(v_a1_a2);
                float     d_a1_i    = (d_a1_a2 * d_a1_a2 - atom2_rr + atom1_rr) / (2 * d_a1_a2);
                float     r_i       = glm::sqrt(glm::abs(atom1_rr - d_a1_i * d_a1_i));

                glm::vec3 v_u;
                if (v_a1_a2_n.z != 0 && v_a1_a2_n.y != 0)
                {
                    v_u             = glm::normalize(glm::vec3(0.0f, -v_a1_a2_n.z, v_a1_a2_n.y));
                }
                else
                {
                    v_u             = glm::normalize(glm::vec3(v_a1_a2_n.y, -v_a1_a2_n.x, 0.0f));
                }

                glm::vec3 v_v       = glm::normalize(glm::cross(v_a1_a2_n, v_u));
                glm::vec3 p_i_rand  = atom1_pos + d_a1_i * v_a1_a2_n + r_i * v_u;

                glm::vec3 v_a1_i    = p_i_rand - atom1_pos;
                glm::vec3 v_a1_i_n  = glm::normalize(v_a1_i);

                glm::vec3 v_a1_t1   = this->getRadiusAt(i) * v_a1_i_n;
                glm::vec3 p_t1c     = atom1_pos + glm::dot(v_a1_t1, v_a1_a2_n) * v_a1_a2_n;
                float     d_a1_t1c  = glm::length(p_t1c - atom1_pos);
                glm::vec3 v_a2_i    = p_i_rand - atom2_pos;
                glm::vec3 v_a2_i_n  = glm::normalize(v_a2_i);
                glm::vec3 v_a2_t2   = this->getRadiusAt(j) * v_a2_i_n;
                glm::vec3 p_t2c     = atom2_pos + glm::dot(v_a2_t2, -v_a1_a2_n) * -v_a1_a2_n;
                float     d_a2_t2c  = glm::length(p_t2c - atom2_pos);

                /*
                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                std::cout << "\t" << "v_a1_a2_n:\t" << v_a1_a2_n.x << "\t" << v_a1_a2_n.y << "\t" << v_a1_a2_n.z << std::endl;
                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                std::cout << "\t" << "d_a1_i:\t" << d_a1_i << std::endl;
                std::cout << "\t" << "r_i:\t" << r_i << std::endl;
                std::cout << "\t" << "v_u:\t" << v_u.x << "\t" << v_u.y << "\t" << v_u.z << std::endl;
                std::cout << "\t" << "v_v:\t" << v_v.x << "\t" << v_v.y << "\t" << v_v.z << std::endl;
                std::cout << "\t" << "p_i_rand:\t" << p_i_rand.x << "\t" << p_i_rand.y << "\t" << p_i_rand.z << std::endl;
                std::cout << "\t" << "v_a1_i:\t" << v_a1_i.x << "\t" << v_a1_i.y << "\t" << v_a1_i.z << std::endl;
                std::cout << "\t" << "v_a1_i_n:\t" << v_a1_i_n.x << "\t" << v_a1_i_n.y << "\t" << v_a1_i_n.z << std::endl;
                std::cout << "\t" << "v_a1_t1:\t" << v_a1_t1.x << "\t" << v_a1_t1.y << "\t" << v_a1_t1.z << std::endl;
                std::cout << "\t" << "p_t1c:\t" << p_t1c.x << "\t" << p_t1c.y << "\t" << p_t1c.z << std::endl;
                std::cout << "\t" << "d_a1_t1c:\t" << d_a1_t1c << std::endl;
                std::cout << "\t" << "v_a2_i:\t" << v_a2_i.x << "\t" << v_a2_i.y << "\t" << v_a2_i.z << std::endl;
                std::cout << "\t" << "v_a2_i_n:\t" << v_a2_i_n.x << "\t" << v_a2_i_n.y << "\t" << v_a2_i_n.z << std::endl;
                std::cout << "\t" << "v_a2_t2:\t" << v_a2_t2.x << "\t" << v_a2_t2.y << "\t" << v_a2_t2.z << std::endl;
                std::cout << "\t" << "p_t2c:\t" << p_t2c.x << "\t" << p_t2c.y << "\t" << p_t2c.z << std::endl;
                std::cout << "\t" << "d_a2_t2c:\t" << d_a2_t2c << std::endl;
                */

                ToroidalPatch tp;
                tp.torus_center     = atom1_pos + d_a1_i * v_a1_a2_n;
                tp.torus_radius     = r_i;
                tp.tangent1_center = p_t1c;
                tp.tangent1_radius = glm::sqrt(this->getRadiusAt(i) * this->getRadiusAt(i) - d_a1_t1c * d_a1_t1c);
                tp.tangent2_center = p_t2c;
                tp.tangent2_radius = glm::sqrt(this->getRadiusAt(j) * this->getRadiusAt(j) - d_a2_t2c * d_a2_t2c);
                frames[T].toroidalPatches.push_back(tp);

                /*
                std::cout << "\t" << "torus_center: " << tp.torus_center.x << "\t" << tp.torus_center.y << "\t" << tp.torus_center.z << std::endl;
                std::cout << "\t" << "torus_radius: " << tp.torus_radius << std::endl;
                std::cout << "\t" << "tangent1_center: " << tp.tangent1_center.x << "\t" << tp.tangent1_center.y << "\t" << tp.tangent1_center.z << std::endl;
                std::cout << "\t" << "tangent1_radius: " << tp.tangent1_radius << std::endl;
                std::cout << "\t" << "tangent2_center: " << tp.tangent2_center.x << "\t" << tp.tangent2_center.y << "\t" << tp.tangent2_center.z << std::endl;
                std::cout << "\t" << "tangent2_radius: " << tp.tangent2_radius << std::endl;
                */
            }
        }
    }

    //std::cout << "toroidal patch count:\t" << frames[T].toroidalPatches.size() << std::endl;
}

void Protein::calculatePatches(float probeRadius, unsigned int *collectedVisibleIDs)
{
    int T = 0; // timestep



    frames[T].spherePatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 2; i++)
    {
        if(collectedVisibleIDs[i] != 1)
            continue;
        for (std::size_t j = i + 1; j < atoms_.size() - 1; j++)
        {
            if(collectedVisibleIDs[j] != 1)
                continue;
            for (std::size_t k = j + 1; k < atoms_.size(); k++)
            {

                if(collectedVisibleIDs[k] != 1)
                    continue;
                glm::vec3 atom1_pos = atoms_[i]->getPosition();
                float     atom1_r   = this->getRadiusAt(i) + probeRadius;
                float     atom1_rr  = atom1_r * atom1_r;
                glm::vec3 atom2_pos = atoms_[j]->getPosition();
                float     atom2_r   = this->getRadiusAt(j) + probeRadius;
                float     atom2_rr  = atom2_r * atom2_r;
                glm::vec3 atom3_pos = atoms_[k]->getPosition();
                float     atom3_r   = this->getRadiusAt(k) + probeRadius;
                float     atom3_rr  = atom3_r * atom3_r;

                float     d_a1_a2   = glm::distance(atom1_pos, atom2_pos);
                float     d_a1_a3   = glm::distance(atom1_pos, atom3_pos);
                float     d_a2_a3   = glm::distance(atom2_pos, atom3_pos);


                if (d_a1_a2 < atom1_r + atom2_r && d_a1_a2 + atom1_r > atom2_r && d_a1_a2 + atom2_r > atom1_r &&
                        d_a1_a3 < atom1_r + atom3_r && d_a1_a3 + atom1_r > atom3_r && d_a1_a3 + atom3_r > atom1_r &&
                        d_a2_a3 < atom2_r + atom3_r && d_a2_a3 + atom2_r > atom3_r && d_a2_a3 + atom3_r > atom2_r)
                {
                    glm::vec3 omega   = atom1_pos;
                    glm::vec3 omega_x = glm::normalize(atom2_pos - atom1_pos);
                    float     i       = glm::dot(omega_x, atom3_pos - atom1_pos);
                    glm::vec3 omega_y = glm::normalize(atom3_pos - atom1_pos - i * omega_x);
                    glm::vec3 omega_z = glm::normalize(glm::cross(omega_x, omega_y));
                    float     d       = glm::length(atom2_pos - atom1_pos);
                    float     j       = glm::dot(omega_y, atom3_pos - atom1_pos);

                    float     x = (atom1_rr - atom2_rr + d * d) / (2 * d);

                    float     radius_help_sphere = glm::sqrt(glm::abs(atom1_rr - x * x));
                    float     distance_help_sphere_i = i - x;

                    if (glm::abs(distance_help_sphere_i) < atom3_r)
                    {
                        float     atom3_r_new = glm::sqrt(glm::abs(atom3_rr - distance_help_sphere_i * distance_help_sphere_i));

                        if (radius_help_sphere + atom3_r_new > j && radius_help_sphere + j > atom3_r_new && atom3_r_new + j > radius_help_sphere)
                        {
                            float y = (radius_help_sphere * radius_help_sphere - atom3_r_new * atom3_r_new + j * j) / (2 * j);
                            float z_abs = glm::sqrt(glm::abs(radius_help_sphere * radius_help_sphere - y * y));

                            glm::vec3 probe1_pos = omega + x * omega_x + y * omega_y + z_abs * omega_z;
                            glm::vec3 probe2_pos = omega + x * omega_x + y * omega_y - z_abs * omega_z;

                            SpherePatch sp1;
                            sp1.probe_position = probe1_pos;
                            sp1.atom1_position = atom1_pos;
                            sp1.atom2_position = atom2_pos;
                            sp1.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp1);

                            SpherePatch sp2;
                            sp2.probe_position = probe2_pos;
                            sp2.atom1_position = atom2_pos; // atom1 and atom2 are switched to keep the same winding order inside the geometry shader
                            sp2.atom2_position = atom1_pos;
                            sp2.atom3_position = atom3_pos;
                            frames[T].spherePatches.push_back(sp2);


                            float d1 =glm::length(probe1_pos - atom1_pos);
                            float d2 =glm::length(probe1_pos - atom2_pos);
                            float d3 =glm::length(probe1_pos - atom3_pos);

                            if (glm::abs(d1 - atom1_r) > 0.001f || glm::abs(d2 - atom2_r) > 0.001f || glm::abs(d3 - atom3_r) > 0.001f)
                            {
                                //std::cout << "atom id: " << i << j << k << std::endl;
                                std::cout << "new" << std::endl;
                                std::cout << "\t" << "dist all: " << glm::abs(d1 - atom1_r) << "  " <<  glm::abs(d2 - atom2_r) << "  " <<  glm::abs(d3 - atom3_r)<< std::endl;
                                std::cout << "\t" << "dist a1: " << d1 << "  " <<  atom1_r << std::endl;
                                std::cout << "\t" << "dist a2: " << d2 << "  " <<  atom2_r << std::endl;
                                std::cout << "\t" << "dist a3: " << d3 << "  " <<  atom3_r << std::endl;
                                /*
                                std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << "\t" << atom1_r << std::endl;
                                std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << "\t" << atom2_r << std::endl;
                                std::cout << "\t" << "a3: " << atom3_pos.x << "\t" << atom3_pos.y << "\t" << atom3_pos.z << "\t" << atom3_r << std::endl;

                                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                                std::cout << "\t" << "d_a1_i1:\t" << d_a1_i1 << std::endl;
                                std::cout << "\t" << "r_i1:\t" << r_i1 << std::endl;
                                std::cout << "\t" << "omega_x:\t" << omega_x.x << "\t" << omega_x.y << "\t" << omega_x.z << std::endl;
                                std::cout << "\t" << "d_a1_o:\t" << d_a1_o << std::endl;
                                std::cout << "\t" << "v_a1_o:\t" << v_a1_o.x << "\t" << v_a1_o.y << "\t" << v_a1_o.z << std::endl;
                                std::cout << "\t" << "omega:\t" << omega.x << "\t" << omega.y << "\t" << omega.z << std::endl;
                                std::cout << "\t" << "v_o_a3:\t" << v_o_a3.x << "\t" << v_o_a3.y << "\t" << v_o_a3.z << std::endl;
                                std::cout << "\t" << "d_o_a3:\t" << d_o_a3 << std::endl;
                                std::cout << "\t" << "omega_y:\t" << omega_y.x << "\t" << omega_y.y << "\t" << omega_y.z << std::endl;
                                std::cout << "\t" << "omega_z:\t" << omega_z.x << "\t" << omega_z.y << "\t" << omega_z.z << std::endl;
                                std::cout << "\t" << "d_i1_o:\t" << d_i1_o << std::endl;
                                std::cout << "\t" << "r_a3_i1:\t" << r_a3_i1 << std::endl;
                                std::cout << "\t" << "d_i1_i2:\t" << d_i1_i2 << std::endl;
                                std::cout << "\t" << "r_i2:\t" << r_i2 << std::endl;
    */
                            }

                            //std::cout << "\t" << "p1: " << probe1_pos.x << "\t" << probe1_pos.y << "\t" << probe1_pos.z << std::endl;
                            //std::cout << "\t" << "p2: " << probe2_pos.x << "\t" << probe2_pos.y << "\t" << probe2_pos.z << std::endl;
                            //std::cout << "\t" << "dist: " << glm::length(probe2_pos - atom1_pos) << "\t" <<  glm::length(probe2_pos - atom2_pos) << "\t" << glm::length(probe2_pos - atom3_pos) << std::endl;
                        }
                    }
                }
            }
        }
    }

    //std::cout << "sphere patch count:\t" << frames[T].spherePatches.size() << std::endl;


    frames[T].toroidalPatches.clear();

    for (std::size_t i = 0; i < atoms_.size() - 1; i++)
    {
        if(collectedVisibleIDs[i] != 1)
            continue;
        for (std::size_t j = i + 1; j < atoms_.size(); j++)
        {
            if(collectedVisibleIDs[j] != 1)
                continue;
            glm::vec3 atom1_pos = atoms_[i]->getPosition();
            float     atom1_r   = this->getRadiusAt(i) + probeRadius;
            float     atom1_rr  = atom1_r * atom1_r;
            glm::vec3 atom2_pos = atoms_[j]->getPosition();
            float     atom2_r   = this->getRadiusAt(j) + probeRadius;
            float     atom2_rr  = atom2_r * atom2_r;

            /*
            std::cout << "i: " << i << " j: " << j << std::endl;

            std::cout << "\t" << "a1: " << atom1_pos.x << "\t" << atom1_pos.y << "\t" << atom1_pos.z << std::endl;
            std::cout << "\t" << "a2: " << atom2_pos.x << "\t" << atom2_pos.y << "\t" << atom2_pos.z << std::endl;
            std::cout << "\t" << "a: " << glm::distance(atom1_pos, atom2_pos) << "\t" << atom1_r << "\t" << atom2_r << "\t" << std::endl;
*/

            if (glm::distance(atom1_pos, atom2_pos) < atom1_r + atom2_r)
            {
                //std::cout << "YES!   i: " << i << " j: " << j << std::endl;

                glm::vec3 v_a1_a2   = atom2_pos - atom1_pos;
                glm::vec3 v_a1_a2_n = glm::normalize(v_a1_a2);
                float     d_a1_a2   = glm::length(v_a1_a2);
                float     d_a1_i    = (d_a1_a2 * d_a1_a2 - atom2_rr + atom1_rr) / (2 * d_a1_a2);
                float     r_i       = glm::sqrt(glm::abs(atom1_rr - d_a1_i * d_a1_i));

                glm::vec3 v_u;
                if (v_a1_a2_n.z != 0 && v_a1_a2_n.y != 0)
                {
                    v_u             = glm::normalize(glm::vec3(0.0f, -v_a1_a2_n.z, v_a1_a2_n.y));
                }
                else
                {
                    v_u             = glm::normalize(glm::vec3(v_a1_a2_n.y, -v_a1_a2_n.x, 0.0f));
                }

                glm::vec3 v_v       = glm::normalize(glm::cross(v_a1_a2_n, v_u));
                glm::vec3 p_i_rand  = atom1_pos + d_a1_i * v_a1_a2_n + r_i * v_u;

                glm::vec3 v_a1_i    = p_i_rand - atom1_pos;
                glm::vec3 v_a1_i_n  = glm::normalize(v_a1_i);

                glm::vec3 v_a1_t1   = this->getRadiusAt(i) * v_a1_i_n;
                glm::vec3 p_t1c     = atom1_pos + glm::dot(v_a1_t1, v_a1_a2_n) * v_a1_a2_n;
                float     d_a1_t1c  = glm::length(p_t1c - atom1_pos);
                glm::vec3 v_a2_i    = p_i_rand - atom2_pos;
                glm::vec3 v_a2_i_n  = glm::normalize(v_a2_i);
                glm::vec3 v_a2_t2   = this->getRadiusAt(j) * v_a2_i_n;
                glm::vec3 p_t2c     = atom2_pos + glm::dot(v_a2_t2, -v_a1_a2_n) * -v_a1_a2_n;
                float     d_a2_t2c  = glm::length(p_t2c - atom2_pos);

                /*
                std::cout << "\t" << "v_a1_a2:\t" << v_a1_a2.x << "\t" << v_a1_a2.y << "\t" << v_a1_a2.z << std::endl;
                std::cout << "\t" << "v_a1_a2_n:\t" << v_a1_a2_n.x << "\t" << v_a1_a2_n.y << "\t" << v_a1_a2_n.z << std::endl;
                std::cout << "\t" << "d_a1_a2:\t" << d_a1_a2 << std::endl;
                std::cout << "\t" << "d_a1_i:\t" << d_a1_i << std::endl;
                std::cout << "\t" << "r_i:\t" << r_i << std::endl;
                std::cout << "\t" << "v_u:\t" << v_u.x << "\t" << v_u.y << "\t" << v_u.z << std::endl;
                std::cout << "\t" << "v_v:\t" << v_v.x << "\t" << v_v.y << "\t" << v_v.z << std::endl;
                std::cout << "\t" << "p_i_rand:\t" << p_i_rand.x << "\t" << p_i_rand.y << "\t" << p_i_rand.z << std::endl;
                std::cout << "\t" << "v_a1_i:\t" << v_a1_i.x << "\t" << v_a1_i.y << "\t" << v_a1_i.z << std::endl;
                std::cout << "\t" << "v_a1_i_n:\t" << v_a1_i_n.x << "\t" << v_a1_i_n.y << "\t" << v_a1_i_n.z << std::endl;
                std::cout << "\t" << "v_a1_t1:\t" << v_a1_t1.x << "\t" << v_a1_t1.y << "\t" << v_a1_t1.z << std::endl;
                std::cout << "\t" << "p_t1c:\t" << p_t1c.x << "\t" << p_t1c.y << "\t" << p_t1c.z << std::endl;
                std::cout << "\t" << "d_a1_t1c:\t" << d_a1_t1c << std::endl;
                std::cout << "\t" << "v_a2_i:\t" << v_a2_i.x << "\t" << v_a2_i.y << "\t" << v_a2_i.z << std::endl;
                std::cout << "\t" << "v_a2_i_n:\t" << v_a2_i_n.x << "\t" << v_a2_i_n.y << "\t" << v_a2_i_n.z << std::endl;
                std::cout << "\t" << "v_a2_t2:\t" << v_a2_t2.x << "\t" << v_a2_t2.y << "\t" << v_a2_t2.z << std::endl;
                std::cout << "\t" << "p_t2c:\t" << p_t2c.x << "\t" << p_t2c.y << "\t" << p_t2c.z << std::endl;
                std::cout << "\t" << "d_a2_t2c:\t" << d_a2_t2c << std::endl;
                */

                ToroidalPatch tp;
                tp.torus_center     = atom1_pos + d_a1_i * v_a1_a2_n;
                tp.torus_radius     = r_i;
                tp.tangent1_center = p_t1c;
                tp.tangent1_radius = glm::sqrt(this->getRadiusAt(i) * this->getRadiusAt(i) - d_a1_t1c * d_a1_t1c);
                tp.tangent2_center = p_t2c;
                tp.tangent2_radius = glm::sqrt(this->getRadiusAt(j) * this->getRadiusAt(j) - d_a2_t2c * d_a2_t2c);
                frames[T].toroidalPatches.push_back(tp);

                /*
                std::cout << "\t" << "torus_center: " << tp.torus_center.x << "\t" << tp.torus_center.y << "\t" << tp.torus_center.z << std::endl;
                std::cout << "\t" << "torus_radius: " << tp.torus_radius << std::endl;
                std::cout << "\t" << "tangent1_center: " << tp.tangent1_center.x << "\t" << tp.tangent1_center.y << "\t" << tp.tangent1_center.z << std::endl;
                std::cout << "\t" << "tangent1_radius: " << tp.tangent1_radius << std::endl;
                std::cout << "\t" << "tangent2_center: " << tp.tangent2_center.x << "\t" << tp.tangent2_center.y << "\t" << tp.tangent2_center.z << std::endl;
                std::cout << "\t" << "tangent2_radius: " << tp.tangent2_radius << std::endl;
                */
            }
        }
    }

    //std::cout << "toroidal patch count:\t" << frames[T].toroidalPatches.size() << std::endl;
}

