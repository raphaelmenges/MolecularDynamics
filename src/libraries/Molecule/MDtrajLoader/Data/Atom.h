// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vector>
#include <string>
#include <glm/ext.hpp>

class Protein;
class Atom
{
public:
    Atom(std::string name, std::string element, int index, glm::vec3 position, std::string aminoDistinctAcid, std::string amino, Protein* parent);
    ~Atom();

    /**
    @brief
    @param [out]
    */
    int getIndex();

    /**
    @brief returns the letter of the Atom e.g. N
    @param [out] string element letter of Atom
    */
    std::string getName();

    /**
    @brief returns the element e.g. Nitrogen
    @param [out] string element name of Atom
    */
    std::string getElement() const;

    /**
    @brief returns just the aminoacid e.g. MET
    @param [out] string aminoacid the Atom belongs to
    */
    std::string getAmino();

    /**
    @brief returns unique aminoacid e.g. MET1
    @param [out] string distinct aminoacid Atom belongs to
    */
    std::string getDistinctResidue();

    /**
    @brief returns the x position of pdb
    @param [out] string returns x position of pdb
    */
    float getX();
    /**
    @brief returns the y position of pdb
    @param [out] string returns y position of pdb
    */
    float getY();
    /**
    @brief returns the y position of pdb
    @param [out] string returns y position of pdb
    */
    float getZ();

    //get atom on frame i
    /**
    @brief returns the x position of the atom on frame i, i = 1 gives frame 1
    @param [out] string returns x position of atom on frame i
    */
    float getXAtFrame(int i);
    //get atom on frame i
    /**
    @brief returns the y position of the atom on frame i, i = 1 gives frame 1
    @param [out] string returns y position of atom on frame i
    */
    float getYAtFrame(int i);
    /**
    @brief returns the z position of the atom on frame i, i = 1 gives frame 1
    @param [out] string returns z position of atom on frame i
    */
    float getZAtFrame(int i);

    /**
    @brief returns the position of the pdb, position [0] is always the pdb position
    @param [out] glm::vec3 position of the pdb
    */
    glm::vec3 getPosition() const;

    Protein* getProteinParent();

    /**
    @brief returns the position of xtc at given frame number i
    @param [in] frame of which the position is wanted, i=1 gives the first frame
    @param [out] glm::vec3 position of atom on frame i
    */
    glm::vec3 getPositionAtFrame(int i);

    /**
    @brief returns list of all Atom ptrs to which current Atom is bounded
    @param [out] vector<Atom*> all Atoms to which current Atom is bounded
    */
    std::vector<Atom*> getBondPartners();

    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setXYZ(glm::vec3 xyz);
    void setXYZat(int frame, glm::vec3 xyz);

    void setNextPosition(glm::vec3 pos);

    //void setActor(AAtom_Actor* a);

    void addBondPartner(Atom*);

private:
    int index_;

    std::string name_; //the letter e.g. N
    std::string element_; //the whole name e.g. Nitrogen
    std::string amino_; //the amino where it belongs to e.g. MET
    std::string aminoDistinctAcid_; // the disting amino where it belongs to e.g. MET1

    std::vector<glm::vec3> positions_; //every position per frame, [0] is pdb and [1] starts first frame

    std::vector<Atom*> bonds_; //all bonded partners
    //AAtom_Actor* pActor_;
    Protein* pParent_;
};
