// Fill out your copyright notice in the Description page of Project Settings.

//#include "PharmaCV.h"
#include "Protein.h"

Protein::Protein(std::vector<std::string> &names,
                 std::vector<std::string> &elementNames, std::vector<std::string> &residueNames,
                 std::vector<int> &indices, std::vector<std::string> &bonds, std::vector<std::vector<glm::vec3>> &positions,
                 std::string name, int numAtoms, std::vector<std::string> &distinctResidueNames)
{
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


    glm::vec3 sum = glm::vec3(0.0f);
    for (auto const& atom : atoms_) sum += atom->getPosition();
    sum /= atoms_.size();
    for (auto & atom : atoms_) atom->setXYZ(atom->getPosition() - sum);


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

