# Molecular Dynamics Visualization

Framework for molecular dynamics visualization developed by Computer Graphics institute at University of Koblenz. 

## How to
Does only work on Ubuntu for now. Follwing packages are required to be installed via apt-get:
```
sudo apt-get install glew-utils libglew-dev libassimp-dev libdevil-dev python-numpy libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev
```

In addition, a Miniconda installation is required. Please download it from [**here**](miniconda3: http://conda.pydata.org/miniconda.html) and install it as advised. After installation, add required packages by following command:

```
conda install -c omnia mdtraj
```
    
## Projects
Multiple projects base on this framework. One can find them within the subfolders.

* [Surface Dynamics Visualization](src/executables/SurfaceDynamicsVisualization)
* [Fast Neighbor Search](src/executables/NeighborSearchTest)

