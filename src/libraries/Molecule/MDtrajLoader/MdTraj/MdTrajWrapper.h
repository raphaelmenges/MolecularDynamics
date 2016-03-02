// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <memory>
#include <glm/ext.hpp>
#include "Molecule/MDtrajLoader/Data/Protein.h"
/**
*
*/
class MdTrajWrapper
{
public:
	MdTrajWrapper();
	~MdTrajWrapper();

    std::auto_ptr<Protein> load(std::vector<std::string> paths);

	void importMDTraj();

	PyObject* loadFilePDB(std::string path);
	PyObject* loadFileXTC(std::string pathPdb, std::string pathXTC);

	PyObject* getXYZ(PyObject* file);
	PyObject* getTopology(PyObject* file);

	void getAllAtomProperties(std::vector<std::string> &paths, std::vector<std::string> &names, std::vector<std::string> &elementNames
		, std::vector<std::string> &residueNames, std::vector<int> &indices
        , std::vector<std::string> &bonds, std::vector<std::string> &distinctResidueNames,
                              std::vector<std::vector<glm::vec3>> &positions, std::vector<float> &radii, int &numAtoms);


private:

	PyObject* function_loadPDB;
	PyObject* function_loadXTC;
};
