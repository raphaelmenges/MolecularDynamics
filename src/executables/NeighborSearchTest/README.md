# Fast neighbor search
By Adrian Derstroff

## Acknowledgement
Fast fixed radius neighbor search is based on *fluids v3* from *Rama C. Hoetzlein*. 
Only the neighbor search has been extracted from *fluids v3*, while the code itself has been rewritten to use compute shaders instead of CUDA. For more information visit [fluids3.com](http://fluids3.com).

## Algorithm
### Overview
Basically the neighborhood search consists of three stages.
1. Insertion of the particles into the grid
2. Sorting the particles based on their cell
3. Search of neighbor aka usage of the built up structures

### Insertion
Insertion takes place on the GPU. Only the position of the particle is required in this stage. The relative position of the particle to the origin of the grid (lower left corner) is calculated with respect to the size and resolution of the grid.

Result of the insertion are three buffers. The first buffer stores the mapping from each particle to its voxel cell. The second buffer saves the position where the particle has been inserted to the cell. So if the cell had been empty before than the first cell will be inserted at the first position, the next particle that will be inserted in this cell will be inserted at the second position within this cell and so on. The third buffer counts the number of elements for every cell. 

In addition with the given fixed search radius and the size of the grid cells, the number of gridcells that had to be searched for are calculated by *(2searchRadius/cellSize + 1)^3*.

### Sorting
Counting sort is utilized for sorting the particles based on their cell index.
The advantage of using counting sort is a time consumption of *O(n + k)*, where *n* is the number of particles and *k* is the number of cells, also it is a stable sort, which means that particles inside a cell don't change their position after the counting sort.

The core of counting sort is a prefix sum or scan. It consists of two sums that can be parallelized on the gpu. For every cell index the prefix sum adds up the number of all elements in those cells up to this cell. We basically end up with the offset of every cell.

Together with the particle's cell index and the cell offsets the sort index can be calculated with:

```
uint sortedIndex = cellOffset[cell[i]] + cellIndex[i];
```

We then sort all particle informations and additionally save the previous index for every sorted particle index.

### Search
With the sorted particles for every particle the corresponding cell is determined and all neighboring grid cells within the search radius are checked.
For every cell all particles then need to be checked if they are inside the search radius of the corresponding particle.

## Usage
In the main code should look like the following.

```C++
Neighborhoodsearch search;

// initialization
float searchRadius = 20.f;
glm::vec3 gridResolution = glm::vec3(10.f, 10.f, 10.f);
glm::vec3 gridMin = glm::vec3(0.f, 0.f, 0.f);
glm::vec3 gridMax = glm::vec3(100.f, 100.f, 100.f);
int numberOfParticles = 1000;

search.init(numberOfParticles, gridMin, gridMax, gridResolution, searchRadius);

// sorting
GLuint* atomPositionsSSBO;
... (fill atom positions ssbo)
Neighborhood neighborhood;

search.run(atomPositionsSSBO, neighborhood);
```

The filled neighborhood structure stores all important information.

```C++
struct Neighborhood {
	// GPU
	GLuint* dp_particleOriginalIndex;   // uint     particles original index before the counting sort
	GLuint* dp_particleCell;            // uint     cell index the particle is in
	GLuint* dp_particleCellIndex;       // uint     insertion index of the particle inside the cell
	GLuint* dp_grid;                    // uint     index of the particle after sorting
	GLuint* dp_gridCellCounts;          // int      number of particles that are in the respective cell
	GLuint* dp_gridCellOffsets;         // int      total offset of the starting point of the respective cell

	// CPU
	int*     p_searchCellOffsets;       // int[]    stores the offsets for all cells that have to be searched
	int        startCellOffset;         // int      since  the particle is always in the center of the search cells
		                              	//          we need the offset of the cell with the lowest index within those
		                              	//          search cells
	int        numberOfSearchCells;     // int      number of cells within the search radius
	float      searchRadius;            // float    adjusted search radius
};
```

With the neighborhood one can implement the actual neighborhood search. This had not been encapsulated inside the neighborhood search class to make it more flexible by the developer.

```GLSL
#version 430
#define GRID_UNDEF 4294967295

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct AtomStruct
{
    vec4 center;
};

// SSBOs
layout(std430, binding = 0) buffer AtomsBuffer               { AtomStruct atoms[];};
layout(std430, binding = 1) buffer ParticlecellsBuffer       { uint pcell[];      };
layout(std430, binding = 2) buffer ParticleCellIndicesBuffer { uint gndx[];       };
layout(std430, binding = 3) buffer GridCellCountBuffer       { int  gridCellCnt[];};
layout(std430, binding = 4) buffer GridCellOffsetBuffer      { int  gridCellOff[];};
layout(std430, binding = 5) buffer GridBuffer                { uint grid[];       };
layout(std430, binding = 6) buffer OriginalIndexBuffer       { uint ondx[];       };

uniform int     numberOfParticles;
uniform float   radiusSquared;          // radius^2
uniform ivec3   gridResolution;
uniform int     numberOfSearchcells;    // (2*searchRadius/cellSize + 1)^3
uniform int     firstSearchCellOffset;  // offset from particle cell to first search cell
uniform int     searchCellsOffset[216]; // offset from the first search cell

void checkIfParticlesAreInRadius(uint currentCell, vec4 position)
{
  // proceed only if cell contains particles
  if (gridCellCnt[currentCell] != 0) {
    uint firstParticle = gridCellOff[currentCell];
    uint lastParticle  = firstParticle + gridCellCnt[currentCell];

    // iterate over all particles
    for (uint pIndex = firstParticle; pIndex < lastParticle; pIndex++) {
      uint oIndex2 = ondx[grid[pIndex]];   // get the original index for the current particle
      AtomStruct atom2 = atoms[oIndex2];
      vec3 position2 = atom2.center;

      // is current particle within search radius?
      vec4 distance = position - position2;
      float d2 = (distance.x * distance.x) + (distance.y * distance.y) + (distance.z * distance.z);
      if (d2 < radiusSquared) {
          ... // do something with the particle inside the search radius
      }
    }
  }
}

void main() {
  // get particle index
  uint i = gl_GlobalInvocationID.x;
  if (i >= numberOfParticles) return; 

  // get unsorted index
  uint oidx = uondx[i];

  // get cell for the corresponding atom
  uint cell = pcell[i];
  if (icell == GRID_UNDEF) return; // particle is outside the grid

  // get position of the atom
  AtomStruct atom = atoms[uidx];
  vec4 position = atom.center;

  // iterate over all search cells
  uint startCell = cell - firstSearchCellOffset;
  for (int cellIndex = 0; cellIndex < numberOfSearchcells; cellIndex++) {
		uint currentCell = startCell + searchCellsOffset[cellIndex];
		checkIfParticlesAreInRadius(currentCell, position);
  }
}
```
