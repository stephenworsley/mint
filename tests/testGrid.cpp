#define _USE_MATH_DEFINES // M_PI for Visual Studio
#include <mntGrid.h>
#include <mntLogger.h>
#undef NDEBUG // turn on asserts
#include <cassert>


// not currently working
void testIrisGrid() {
    Grid_t* grd;
    mnt_grid_new(&grd);
    mnt_grid_loadFromUgrid2DFile(&grd, "${CMAKE_SOURCE_DIR}/data/gridlike_mesh.nc$example_mesh");
    mnt_grid_del(&grd);
}

void testDisplacedUgrid() {
    Grid_t* grd;
    mnt_grid_new(&grd);
    int fixLonAcrossDateline = 1;
    int averageLonAtPole = 1;
    int degrees = 1;
    mnt_grid_setFlags(&grd, fixLonAcrossDateline, averageLonAtPole, degrees);
    mnt_grid_loadFromUgrid2DFile(&grd, "${CMAKE_SOURCE_DIR}/data/cs8_wind.nc$cs");
    std::size_t numBadCells;
    int ier = mnt_grid_check(&grd, &numBadCells);
    mnt_printLogMessages();
    std::cerr << "number of bad cells: " << numBadCells << '\n';
    assert(numBadCells == 0);
    mnt_grid_del(&grd);
   
}

void testVTK() {
    Grid_t* grd;
    mnt_grid_new(&grd);
    mnt_grid_load(&grd, "${CMAKE_SOURCE_DIR}/data/cs.vtk");
    mnt_grid_del(&grd);
}

void testUgridData() {

    int ier;
    Grid_t* grd = NULL;
    ier = mnt_grid_new(&grd);
    assert(ier == 0);

    /* a single cell
     3....>2....2
     :          :
     v          ^
     1          0
     :          :
     0....<3....1
    */

    std::size_t ncells = 1;
    std::size_t nedges = 4;
    std::size_t npoints = 4;
    std::vector<double> xyz{0.,0.,0.,
                            1.,0.,0.,
                            1.,1.,0.,
                            0.,1.,0.};
    std::vector<std::size_t> face2nodes{0, 1, 2, 3};
    std::vector<std::size_t> edge2nodes{1, 2,
                                        3, 0,
                                        3, 2,
                                        1, 0};
    ier = mnt_grid_loadFromUgrid2DData(&grd, ncells, nedges, npoints, &xyz[0], &face2nodes[0], &edge2nodes[0]);

    std::vector<vtkIdType> edgeNodeIds(MNT_NUM_VERTS_PER_EDGE);

    ier = mnt_grid_getNodeIds(&grd, 0, 0, &edgeNodeIds[0]);
    assert(edgeNodeIds[0] == 0);
    assert(edgeNodeIds[1] == 1);

    ier = mnt_grid_getNodeIds(&grd, 0, 1, &edgeNodeIds[0]);
    assert(edgeNodeIds[0] == 1);
    assert(edgeNodeIds[1] == 2);

    ier = mnt_grid_getNodeIds(&grd, 0, 2, &edgeNodeIds[0]);
    assert(edgeNodeIds[0] == 3);
    assert(edgeNodeIds[1] == 2);

    ier = mnt_grid_getNodeIds(&grd, 0, 3, &edgeNodeIds[0]);
    assert(edgeNodeIds[0] == 0);
    assert(edgeNodeIds[1] == 3);

    ier = mnt_grid_del(&grd);
    assert(ier == 0);
}


void testLFRic() {
    int ier;
    Grid_t* grd;
    ier = mnt_grid_new(&grd);
    assert(ier == 0);
    // filename$meshname
    ier = mnt_grid_loadFromUgrid2DFile(&grd, "${CMAKE_SOURCE_DIR}/data/lfric_diag_wind.nc$Mesh2d");
    assert(ier == 0);
    std::size_t numBadCells = 0;
    ier = mnt_grid_check(&grd, &numBadCells);
    assert(numBadCells == 0);
    double* verts;
    ier = mnt_grid_getPointsPtr(&grd, &verts);
    assert(ier == 0);
    std::size_t numCells;
    ier = mnt_grid_getNumberOfCells(&grd, &numCells);
    for (auto cellId = 0; cellId < numCells; ++cellId) {
        for (auto vIndex = 0; vIndex < 4; ++vIndex) {
            auto k = 3*(cellId*4 + vIndex);
            std::cout << "testLFRic: cell " << cellId << " node " << vIndex << " coords " << 
                         verts[k + 0] << ',' << verts[k + 1] << ',' << verts[k + 2] << '\n';
        }
    }
    ier = mnt_grid_del(&grd);
    assert(ier == 0);
}

void testUgrid() {
    int ier;
    Grid_t* grd;
    ier = mnt_grid_new(&grd);
    assert(ier == 0);
    ier = mnt_grid_loadFromUgrid2DFile(&grd, "${CMAKE_SOURCE_DIR}/data/cs_16.nc$physics");
    assert(ier == 0);
    ier = mnt_grid_print(&grd);
    assert(ier == 0);
    size_t numCells;
    ier = mnt_grid_getNumberOfCells(&grd, &numCells);
    assert(ier == 0);
    std::cout << numCells << " cells\n";

    std::vector<vtkIdType> cellIds{0, 1, 2, 3, 12, 456, 1535};

    vtkIdType nodeIds[2];
    size_t edgeId;
    int signEdge;
    for (auto cellId : cellIds) {
        for (int edgeIndex = 0; edgeIndex < 4; ++edgeIndex) {
            ier = mnt_grid_getNodeIds(&grd, cellId, edgeIndex, nodeIds);
            std::cout << " cell " << cellId << " edge index " << edgeIndex 
                      << " connects to nodes "  << nodeIds[0] << ',' << nodeIds[1] << '\n';
            assert(ier == 0);
            ier = mnt_grid_getEdgeId(&grd, cellId, edgeIndex, &edgeId, &signEdge);
            std::cout << " cell " << cellId << " edge index " << edgeIndex 
                      << " maps to edge Id = " << edgeId << " (direction " << signEdge << ")\n";
            assert(ier == 0);
        }
    }

    // check that each face a positive area
    std::size_t numBadCells = 0;
    ier = mnt_grid_check(&grd, &numBadCells);
    assert(ier == 0);
    assert(numBadCells == 0);

    // attach a field
    std::vector<double> cellByCellData(numCells * 4);
    for (size_t cellId = 0; cellId < numCells; ++cellId) {
        for (int ie = 0; ie < 4; ++ie) {
            ier = mnt_grid_getEdgeId(&grd, cellId, ie, &edgeId, &signEdge);
            assert(ier == 0);
            size_t k = cellId*4 + ie;
            cellByCellData[k] = (double) edgeId * (double) signEdge;        }
    }
    const char fieldName[] = "edge_ids";
    std::cout << "attaching edge field " << fieldName << " to grid\n";
    ier = mnt_grid_attach(&grd, fieldName, 4, &cellByCellData[0]);
    assert(ier == 0);

    ier = mnt_grid_computeEdgeArcLengths(&grd);
    assert(ier == 0);

    // check that we can retrieve the edge arc lengths
    for (auto cellId : cellIds) {
        for (int edgeIndex = 0; edgeIndex < 4; ++edgeIndex) {
            double arcLength;
            ier = mnt_grid_getEdgeArcLength(&grd, cellId, edgeIndex, &arcLength);
            assert(ier == 0);
            assert(arcLength >= 0);
            assert(arcLength <= M_PI);
        }
    }

    // should be able to compute the edge arcs multiple times
    ier = mnt_grid_computeEdgeArcLengths(&grd);
    assert(ier == 0);    
    
    ier = mnt_grid_dump(&grd, "cs_16.vtk");
    assert(ier == 0);

    ier = mnt_grid_del(&grd);
    assert(ier == 0);
}


int main(int argc, char** argv) {

    //testIrisGrid();
    testDisplacedUgrid();
    testUgridData();
    testLFRic();
    testUgrid();
    testVTK();

    mnt_printLogMessages();

    return 0;
}
