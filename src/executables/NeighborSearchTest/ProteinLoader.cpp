//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#include "ProteinLoader.h"

ProteinLoader::ProteinLoader()
{
    m_currentProteinIdx = 0;
}

ProteinLoader::~ProteinLoader()
{
    for (int i = 0; i < m_proteins.size(); i++) {
        delete m_proteins.at(i);
    }
    m_proteins.clear();
}

int ProteinLoader::getNumberOfProteins()
{
    return m_proteins.size();
}

std::vector<SimpleProtein*> ProteinLoader::getProteins()
{
    return m_proteins;
}

SimpleProtein* ProteinLoader::getProteinAt(int i)
{
    return m_proteins.at(i);
}

void ProteinLoader::updateAtoms()
{
    m_allAtoms.clear();
    for (int i = 0; i < getNumberOfProteins(); i++) {
        SimpleProtein* protein = getProteinAt(i);
        m_allAtoms.insert(std::end(m_allAtoms), std::begin(protein->atoms), std::end(protein->atoms));
    }
}

std::vector<SimpleAtom> ProteinLoader::getAllAtoms()
{
    updateAtoms();
    return m_allAtoms;
}

int ProteinLoader::getNumberOfAllAtoms()
{
    return m_allAtoms.size();
}

/*
 * LOADING PROTEIN
 */
SimpleProtein* ProteinLoader::loadProtein(std::string fileName)
{
    /*
     * extracting protein name from file name
     */
    std::string proteinName = fileName;
    int lastSlash = proteinName.find_last_of("/");
    if (lastSlash >= 0) {
        proteinName = proteinName.substr(lastSlash+1, proteinName.size());
    }
    int lastDot = proteinName.find_last_of(".");
    if (lastDot >= 0) {
        proteinName = proteinName.substr(0, lastDot);
    }



    /*
     * concatenate full path
     */
    std::string subfolder = "/molecules/";
    std::string filePath = RESOURCES_PATH + subfolder + fileName;



    /*
     * load protein from pdb file
     */
    SimpleProtein* protein = new SimpleProtein;
    protein->name = proteinName;
    loadPDB(filePath, *protein, protein->bbMin, protein->bbMax);
    m_proteins.push_back(protein);

    return m_proteins.at(m_proteins.size()-1);
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
    float maxRadius = 0;

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
        double radius = PyFloat_AsDouble(atom_radius_py) * 10;
        radii.push_back(radius);

        /*
         * find the atom with the biggest radius
         */
        maxRadius = std::max((float)radius, maxRadius);

        Py_DECREF(name_py);
        Py_DECREF(element_py);
        Py_DECREF(element_name_py);
        Py_DECREF(atom_radius_py);

        atom = PyIter_Next(atom_iterator_py);
    }
    Py_DECREF(atom_iterator_py);


    /*
     * extent the bounding box by the radius of the biggest atom
     */
    minPosition -= maxRadius;
    maxPosition += maxRadius;


    /*
     * Create atoms and add them to the protein
     * and to the allAtoms vector
     */
    if ((positions.size() == names.size()) && (positions.size() == elementNames.size()) && (positions.size() == radii.size())) {
        for (int i = 0; i < names.size(); i++) {
            std::string name = names.at(i);
            std::string elementName = elementNames.at(i);
            glm::vec3 position = positions.at(i);
            float radius = radii.at(i);

            /*
             * create atom
             */
            SimpleAtom atom;
            atom.pos = position;
            atom.radius = radius;
            atom.proteinID = glm::vec4(m_currentProteinIdx, m_currentProteinIdx, m_currentProteinIdx, m_currentProteinIdx);

            /*
             * add atom to both protein and all atoms
             */
            protein.atoms.push_back(atom);
            m_allAtoms.push_back(atom);
        }
    } else {
        std::cerr << "Size of atom positions " << positions.size() << " and properties " << names.size() << ", " << elementNames.size() << ", " << radii.size() << " dont match" << std::endl;
    }


    /*
     * increment protein idx
     */
    m_currentProteinIdx++;
}

void ProteinLoader::getBoundingBoxAroundProteins(glm::vec3& min, glm::vec3& max)
{
    if (m_proteins.size() > 0) {
        min = glm::vec3(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX);
        max = glm::vec3(FLOAT_MIN, FLOAT_MIN, FLOAT_MIN);
        for (int i = 0; i < m_proteins.size(); i++)
        {
            SimpleProtein* protein = m_proteins.at(i);
            min.x = glm::min(min.x, protein->bbMin.x);
            min.y = glm::min(min.y, protein->bbMin.y);
            min.z = glm::min(min.z, protein->bbMin.z);
            max.x = glm::max(max.x, protein->bbMax.x);
            max.y = glm::max(max.y, protein->bbMax.y);
            max.z = glm::max(max.z, protein->bbMax.z);
        }
    } else {
        Logger::instance().print("No proteins there to calculate bounding box!", Logger::Mode::WARNING);
        min = glm::vec3(0,0,0);
        max = glm::vec3(0,0,0);
    }
}

void ProteinLoader::getCenteredBoundingBoxAroundProteins(glm::vec3& min, glm::vec3& max)
{
    getBoundingBoxAroundProteins(min, max);
    glm::vec3 extent = max - min;
    glm::vec3 center = (max + min) / 2;
    float longestSideHalf = std::max(std::max(extent.x, extent.y), extent.z) / 2;
    min = center - longestSideHalf;
    max = center + longestSideHalf;
}
