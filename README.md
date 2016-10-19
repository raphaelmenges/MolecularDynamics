# Molecular Dynamics Visualization

Framework for molecular dynamics visualization developed by Computer Graphics institute at University of Koblenz. 

## How to
Since some dependencies are integrated as git submodules, use _git clone --recursive_ to clone the repository to your local machine.

Compilation and execution only works on Ubuntu 15.10 or higher. Follwing packages are required to be installed via apt-get:
```
glew-utils libglew-dev libassimp-dev libdevil-dev python-numpy libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev
```

In addition, a Miniconda 3 installation is required. Please download it from [**here**](http://conda.pydata.org/miniconda.html) and install it as advised. After installation, add required packages using following command:

```
conda install -c omnia mdtraj
```
    
## Projects
Multiple projects base on this framework. One can find them within the subfolders of this repository.

* [Surface Dynamics Visualization](src/executables/SurfaceDynamicsVisualization)
* [Fast Neighbor Search](src/executables/NeighborSearchTest)
