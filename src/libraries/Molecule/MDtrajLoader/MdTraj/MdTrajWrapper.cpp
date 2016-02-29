// Fill out your copyright notice in the Description page of Project Settings.
//#include "PharmaCV.h"
#include "MdTrajWrapper.h"

MdTrajWrapper::MdTrajWrapper()
{

    /*	FString ue_path = FPaths::GameDir();
   //UE_LOG(LogTemp, Warning, TEXT("Path in MDTrajWrapper: %s"), *ue_path);
    std::string updated_path(TCHAR_TO_UTF8(*ue_path));
    updated_path += "Content/Include/mdtraj";

    Py_Initialize();
    PyObject* sys = PyImport_ImportModule("sys");

    PyObject* path= PyObject_GetAttrString(sys, "path");
    Py_DECREF(sys);
    PyList_Append(path, PyUnicode_FromString(updated_path.c_str()));
    Py_DECREF(path);
    this->importMDTraj();*/

    Py_Initialize();
    PySys_SetArgvEx(0, NULL, 0);
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    //std::string s = "/home/nlichtenberg/miniconda/lib/python2.7/site-packages";
    std::string s = MDTRAJ_PATH;
    PyList_Append(path, PyUnicode_FromString(s.c_str())); // add location of the MDTraj package to python path
    Py_DECREF(path);
    this->importMDTraj();

    //    mdtraj = PyImport_ImportModule("mdtraj");
    //    mdtraj_load = PyObject_GetAttrString(mdtraj, "load");

    //    import_array();
}

MdTrajWrapper::~MdTrajWrapper()
{
    //UE_LOG(LogTemp, Warning, TEXT("MdTrajWrapper: delete"));
    //Py_Finalize();

}



/*
* @param vector of paths to xtc and/or pdb
* if xtc then first argument in vector is path to pdb and second path to xtc
*/
Protein* MdTrajWrapper::load(std::vector<std::string> paths)
{
    std::vector<std::string> names;
    std::vector<std::string> bonds;
    std::vector<int> indices;
    std::vector<std::string> residueNames;
    std::vector<std::string> elementNames;
    std::vector<std::string> distinctResidueNames;
    std::vector<std::vector<glm::vec3>> positions;
    int numAtoms;
    std::string pathTmp = paths[0].substr(paths[0].find_last_of("\\/")+1);
    std::string proteinName = pathTmp.substr(0, pathTmp.size()-4);
    getAllAtomProperties(paths, names,
                         elementNames, residueNames,
                         indices, bonds, distinctResidueNames, positions, numAtoms);

    //	pDq->spawnProtein(names, elementNames, residueNames,
    //		indices, bonds, positions, proteinName, numAtoms, distinctResidueNames);


    Protein* prot = new Protein(names,
        elementNames, residueNames,
        indices, bonds, positions, proteinName, numAtoms, distinctResidueNames);
   // proteins_.push_back(prot);
    return prot;
}


//----------------------------------------------------------------------------------------------

void MdTrajWrapper::importMDTraj()
{
    PyObject* MDTrajString = PyUnicode_FromString((char*)"mdtraj");
    PyObject* MDTraj = PyImport_Import(MDTrajString);

    Py_DECREF(MDTrajString);

    if (MDTraj == nullptr)
    {
        std::cerr << "Python Error. Printing Python Stacktrace..." << std::endl;
        PyErr_Print();
        std::exit(1);
    }

    // get access to different functions
    function_loadPDB = PyObject_GetAttrString(MDTraj, (char*)"load_pdb");
    function_loadXTC = PyObject_GetAttrString(MDTraj, (char*)"load_xtc");

    Py_DECREF(MDTraj);
}

PyObject* MdTrajWrapper::loadFilePDB(std::string path)
{
    PyObject* args = PyTuple_Pack(1, PyUnicode_FromString(path.c_str()));
    PyObject* file = PyObject_CallObject(function_loadPDB, args);

    Py_DECREF(args);
    return file;
}

PyObject* MdTrajWrapper::loadFileXTC(std::string pathXTC, std::string pathPDB) {

    PyObject* args = PyTuple_Pack(1, PyUnicode_FromString(pathXTC.c_str()));
    PyObject *kwargs = Py_BuildValue("{s:O}", "top", PyUnicode_FromString(pathPDB.c_str()));
    PyObject* file = PyObject_Call(function_loadXTC, args, kwargs);

    Py_DECREF(args);
    Py_DECREF(kwargs);
    //Optional TODO find out how many atoms in pdb is too much

    return file;

}

/**
* @brief extractXYZ from pdb
* @param file
* @return Returns the full trajectory of the given file
*/
PyObject *MdTrajWrapper::getXYZ(PyObject *file)
{
    return PyObject_GetAttrString(file, (char*)"xyz");
}

PyObject *MdTrajWrapper::getTopology(PyObject *file)
{
    return PyObject_GetAttrString(file, (char*)"topology");
}


void MdTrajWrapper::getAllAtomProperties(std::vector<std::string> &paths, std::vector<std::string> &names,
                                         std::vector<std::string> &elementNames, std::vector<std::string> &residueNames,
                                         std::vector<int> &indices, std::vector<std::string> &bonds, std::vector<std::string> &distinctResidue, std::vector<std::vector<glm::vec3>> &positions, int &numAtoms)
{
    if (paths.size() > 2) {
//       //UE_LOG(LogTemp, Error, TEXT("Anzahl der zu ladenden Dateien ist > 3"));
//       //UE_LOG(LogTemp, Error, TEXT("Einfach nein. Hier sollte man eig. bisher nicht hinkommen, weil .pdbs einzeln geladen werden und .xtc brauchen nur noch eine weitere pdb."));
        return;
    }

    //---------------------------------------------------load file-----------------------------------------------

    if (paths.size() == 1) {
        if (paths[0].substr((paths[0].length() - 3), 3) != "pdb") {
            //UE_LOG(LogTemp, Error, TEXT("BOAH MAN EY, wir können bisher nur .pdb und .xtc laden!!"));
            return;
        }
    }
    PyObject* pdbFile = loadFilePDB(paths[0]);

    PyObject* xyz_py = getXYZ(pdbFile);
    PyObject* topo = getTopology(pdbFile);
    Py_DECREF(pdbFile);


    PyArrayObject* xyz_pyarray = reinterpret_cast<PyArrayObject*>(xyz_py);

    //use iterator here for atom name and element :)
    PyObject* atom_py = PyObject_GetAttrString(topo, "atoms");

    PyObject* atom_iterator_py = PyObject_GetIter(atom_py);
    Py_DECREF(atom_py);
    if (atom_iterator_py == NULL) {
        //UE_LOG(LogTemp, Warning, TEXT("ITERATOR EMPTY"));
    }



    //-----------------------------------------AtomProperties-----------------------------------------------------------

    //get number of frames, atoms and components
    long long numFrames{ PyArray_SHAPE(xyz_pyarray)[0] };
    long long numAtom{ PyArray_SHAPE(xyz_pyarray)[1] };
    long long numComponents{ PyArray_SHAPE(xyz_pyarray)[2] };
    numAtoms = (int)numAtom;


    // cast the 3D numpy array to a 1D c array
    float* xyz_carray;
    xyz_carray = reinterpret_cast<float*>(PyArray_DATA(xyz_pyarray));

    float positionX;
    float positionY;
    float positionZ;

    std::vector<glm::vec3> frameHolder;
    for (int a = 0; a < numAtoms; a++)
    {
        int id = a * numComponents;
        positionX = xyz_carray[id] * 10;
        positionY = xyz_carray[id + 1] * 10;
        positionZ = xyz_carray[id + 2] * 10;
        glm::vec3 posi(positionX, positionY, positionZ);
        frameHolder.push_back(posi);
    }
    positions.push_back(frameHolder);
    frameHolder.clear();

    Py_DECREF(xyz_py);
    PyObject* atom;
    PyObject* name_py;
    PyObject* element_py;
    PyObject* element_name_py;
    PyObject* distinct_residue_name_py;
    PyObject* atom_radius_py;
    int i = 1;
    atom = PyIter_Next(atom_iterator_py);
    while ((atom != NULL)) {
        name_py = PyObject_GetAttrString(atom, "name");

        element_py = PyObject_GetAttrString(atom, "element");

        element_name_py = PyObject_GetAttrString(element_py, "name");
        Py_DECREF(element_py);
        distinct_residue_name_py=PyObject_GetAttrString(atom, "residue");

//        atom_radius_py = PyObject_GetAttrString(element_py, "radius");

//        float radius = PyFloat_AsDouble(atom_radius_py);


        PyObject* dist = PyObject_Str(distinct_residue_name_py);
        Py_DECREF(distinct_residue_name_py);
        names.push_back(PyUnicode_AsUTF8(name_py));
        Py_DECREF(name_py);
        elementNames.push_back(PyUnicode_AsUTF8(element_name_py));
        Py_DECREF(element_name_py);
        distinctResidue.push_back(PyUnicode_AsUTF8(dist));
        Py_DECREF(dist);
        indices.push_back(i);
        i++;

        atom = PyIter_Next(atom_iterator_py);
    }
    Py_DECREF(atom_iterator_py);

    //-----------------------------------------------------------
    //use iterator here for residue names
    PyObject* residue_py = PyObject_GetAttrString(topo, "residues");
    PyObject* residue_iterator_py = PyObject_GetIter(residue_py);
    Py_DECREF(residue_py);

    if (residue_iterator_py == NULL) {
        //UE_LOG(LogTemp, Warning, TEXT("ITERATOR EMPTY"));
    }

    PyObject* residue;
    PyObject* residue_name_py;

    residue = PyIter_Next(residue_iterator_py);
    while ((residue != NULL)) {
        residue_name_py = PyObject_GetAttrString(residue, "name");
        residueNames.push_back(PyUnicode_AsUTF8(residue_name_py));

        Py_DECREF(residue_name_py);
        residue = PyIter_Next(residue_iterator_py);
    }
    Py_DECREF(residue_iterator_py);



    PyObject* bonds_py = PyObject_GetAttrString(topo, "bonds");
    Py_DECREF(topo);
    PyObject* bonds_iterator_py = PyObject_GetIter(bonds_py);

    if (bonds_iterator_py == NULL) {
        //UE_LOG(LogTemp, Warning, TEXT("ITERATOR EMPTY"));
    }

    PyObject* bond;

    bond = PyIter_Next(bonds_iterator_py);
    while ((bond != NULL)) {

        PyObject* bondfi = PyObject_Str(bond);
        bonds.push_back(PyUnicode_AsUTF8(bondfi));
        Py_DECREF(bondfi);
        bond = PyIter_Next(bonds_iterator_py);

    }
    Py_DECREF(bonds_iterator_py);

    //-------------------------------------------------------load xtc if there was one
    if (paths.size() == 2) {
        std::string pathPDB = paths.at(0);
        std::string pathXTC = paths.at(1);
        if (pathXTC.substr((pathXTC.length() - 3),3) != "xtc") {
           //UE_LOG(LogTemp, Error, TEXT("BOAH MAN EY, wenn du ne .xtc laden willst muss zuerst die .pdb rein und dann die .xtc"));
           //UE_LOG(LogTemp, Error, TEXT("und wenn du mehrere .pdbs laden willst, musst du musst du die einzeln reinladen, MAN"));
            return;
        }
        else {
            PyObject* traj = loadFileXTC(pathXTC, pathPDB);
            if (traj == NULL) {
               //UE_LOG(LogTemp, Error, TEXT("Ok, die pdb passt nicht zur xtc, es wurde nur die .pdb geladen. (Wahrscheinlich stimmt die Atomanzahl nicht überein. Vllt Wasser raus schneiden?)"));
                return;
            }

            PyObject* xyz = getXYZ(traj);

            Py_DECREF(traj);
            xyz_pyarray = reinterpret_cast<PyArrayObject*>(xyz);
            xyz_carray = reinterpret_cast<float*>(PyArray_DATA(xyz_pyarray));

            //get number of frames, atoms and components
            numFrames = PyArray_SHAPE(xyz_pyarray)[0];
            numAtom = PyArray_SHAPE(xyz_pyarray)[1];
            numComponents = PyArray_SHAPE(xyz_pyarray)[2];
            numComponents = (int)numComponents;
            numFrames = (int)numFrames;
            numAtoms = (int)numAtom;

            //--------------------------read the atoms for each frame
            int c = 0;
            for (int b = 1; b < numFrames + 1; b++) {
                for (c; c < numAtoms*b; c++) {
                    int id = c * numComponents;
                    positionX = xyz_carray[id] * 10;
                    positionY = xyz_carray[id + 1] * 10;
                    positionZ = xyz_carray[id + 2] * 10;
                    glm::vec3 posi(positionX, positionY, positionZ);
                    frameHolder.push_back(posi);
                }
                positions.push_back(frameHolder);
                frameHolder.clear();
            }

            Py_DECREF(xyz);

        }
    }

}

/*
public string ModulePath ()
{
    return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
}

public string FullPath ()
{
    return Path.GetFullPath(Path.Combine(ModulePath, "../../"));
}
*/
