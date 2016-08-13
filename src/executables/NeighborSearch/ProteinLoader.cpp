//
// Created by ubundrian on 09.06.16.
//

#include "ProteinLoader.h"

ProteinLoader::ProteinLoader()
{

}

ProteinLoader::~ProteinLoader()
{

}

void ProteinLoader::loadPDB(std::string filePath, SimpleProtein &protein, glm::vec3 &minPosition, glm::vec3 &maxPosition)
{

    /*
     * set start values of min and max position
     */
    double min = std::numeric_limits<double>::min();
    double max = std::numeric_limits<double>::max();
    minPosition = glm::vec3(max, max, max);
    maxPosition = glm::vec3(min, min, min);

    /*
     * the informations we need to create our atoms
     */
    std::vector<std::string> names;
    std::vector<std::string> elementNames;
    std::vector<glm::vec3> positions;
    std::vector<float> radii;


    /*
     * init mdtraj wrapper and load pdb file as pyobject
     */
    MdTrajWrapper mdTrajWrapper;
    PyObject* pdbFile = mdTrajWrapper.loadFilePDB(filePath);


    /*
     * Atom positions
     */
    PyObject* xyz_py = mdTrajWrapper.getXYZ(pdbFile);
    PyObject* topo = mdTrajWrapper.getTopology(pdbFile);
    Py_DECREF(pdbFile);
    PyArrayObject* xyz_pyarray = reinterpret_cast<PyArrayObject*>(xyz_py);

    //get number of frames, atoms and components
    long long numAtom{ PyArray_SHAPE(xyz_pyarray)[1] };
    long long numComponents{ PyArray_SHAPE(xyz_pyarray)[2] };
    int numAtoms = (int)numAtom;

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
        glm::vec3 position(positionX, positionY, positionZ);
        positions.push_back(position);

        /*
         * get min and max
         */
        minPosition.x = (positionX < minPosition.x) ? positionX : minPosition.x;
        minPosition.y = (positionY < minPosition.y) ? positionY : minPosition.y;
        minPosition.z = (positionZ < minPosition.z) ? positionZ : minPosition.z;

        maxPosition.x = (positionX > maxPosition.x) ? positionX : maxPosition.x;
        maxPosition.y = (positionY > maxPosition.y) ? positionY : maxPosition.y;
        maxPosition.z = (positionZ > maxPosition.z) ? positionZ : maxPosition.z;
    }
    Py_DECREF(xyz_py);


    /*
     * Atom properties
     */
    PyObject* atom_py = PyObject_GetAttrString(topo, "atoms");
    PyObject* atom_iterator_py = PyObject_GetIter(atom_py);
    Py_DECREF(atom_py);

    PyObject* atom;
    PyObject* name_py;
    PyObject* element_py;
    PyObject* element_name_py;
    PyObject* atom_radius_py;

    atom = PyIter_Next(atom_iterator_py);
    while ((atom != NULL)) {
        name_py = PyObject_GetAttrString(atom, "name");
        element_py = PyObject_GetAttrString(atom, "element");
        element_name_py = PyObject_GetAttrString(element_py, "name");
        atom_radius_py = PyObject_GetAttrString(element_py, "radius");

        names.push_back(PyUnicode_AsUTF8(name_py));
        elementNames.push_back(PyUnicode_AsUTF8(element_name_py));
        radii.push_back(PyFloat_AsDouble(atom_radius_py) * 10);

        Py_DECREF(name_py);
        Py_DECREF(element_py);
        Py_DECREF(element_name_py);
        Py_DECREF(atom_radius_py);

        atom = PyIter_Next(atom_iterator_py);
    }
    Py_DECREF(atom_iterator_py);


    /*
     * Create protein
     */
    if ((positions.size() == names.size()) && (positions.size() == elementNames.size()) && (positions.size() == radii.size())) {
        for (int i = 0; i < names.size(); i++) {
            std::string name = names.at(i);
            std::string elementName = elementNames.at(i);
            glm::vec3 position = positions.at(i);
            float radius = radii.at(i);

            //Logger::instance().print("Atom " + name + " (" + elementName + ") pos: (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ") rad: " + std::to_string(radius));

            protein.createAtom(name, position, radius);
        }
    } else {
        std::cerr << "Size of atom positions " << positions.size() << " and properties " << names.size() << ", " << elementNames.size() << ", " << radii.size() << " dont match" << std::endl;
    }
}