// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Atom.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <limits>
#include <set>
#include <glm/ext.hpp>

/**
 * @file Protein.h
 * @brief class for a protein
 */
class Atom;
class Protein
{
public:
    Protein(std::vector<std::string> &names,
        std::vector<std::string> &elementNames, std::vector<std::string> &residueNames,
        std::vector<int> &indices, std::vector<std::string> &bonds, std::vector<std::vector<glm::vec3> > &positions,
        std::string name, int numAtoms, std::vector<std::string> &distinctResidue, std::vector<float> &radii);
	~Protein();

	/**
	@brief name of Protein e.g. E2IH
	@param [out] string name of Protein
	*/
	std::string getName();

	/**
	@brief get the ith Atom of the Protein
	@param [out] Atom on given position i from the Protein
	*/
	Atom* getAtomAt(int i);

	std::vector<Atom*>* getAtoms();
	std::vector<Atom*>* getAtomsFromAmino(std::string name);    

	std::vector<std::string>* getAminoNames();
	std::vector<std::string>* getDiffAminos();

    glm::vec3 getMin();
    glm::vec3 getMax();

	void minMax();
	const static int DIFF_AMINOS = 24;


    std::vector<float> getRadii() const;
    float getRadiusAt(int i);

private:

    int numAtoms_;
    int numFrames_;
    glm::vec3 min_;
    glm::vec3 max_;

	std::string name_; //name of the protein

	std::vector<Atom*> atoms_; //list of all atoms
    std::vector<float> radii_; // list of all atom radii
	std::vector<std::string> aminoNames_; //is the entire sequence of the Protein
	std::vector<std::string> diffAminos_; //is the unique vector of aminoacids, maximum 23


	std::set<std::string> aminoAcids_;

	//key "distinctResidue", value "vector of its Atoms"
	std::unordered_map<std::string, std::vector<Atom*>> aminoAndItsAtoms_;

	void setBonds(std::vector<std::string> bonds);

};
