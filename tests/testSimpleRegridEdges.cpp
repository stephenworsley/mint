#include <limits> // required by vtkUnstructuredGrid
#include "mntRegridEdges.h"
#include <vtkDoubleArray.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkIdList.h>
#include <iostream>
#include <string>
#undef NDEBUG // turn on asserts
#include <cassert>


int main() {

    std::string srcFile = "@CMAKE_SOURCE_DIR@/data/tiny1x1.nc$physics";
    std::string dstFile = "@CMAKE_SOURCE_DIR@/data/tiny1x1.nc$physics";


    RegridEdges_t* rg;
    int ier;

    ier = mnt_regridedges_new(&rg);
    assert(ier == 0);

    ier = mnt_regridedges_loadSrcGrid(&rg, srcFile.c_str(), (int) srcFile.size());
    assert(ier == 0);

    ier = mnt_regridedges_loadDstGrid(&rg, dstFile.c_str(), (int) dstFile.size());
    assert(ier == 0);

    int num_cells_per_bucket = 1;
    double periodX = 0.;
    int enableFolding = 0;
    ier = mnt_regridedges_buildLocator(&rg, num_cells_per_bucket, periodX, enableFolding);
    assert(ier == 0);

    int debug = 2;
    ier = mnt_regridedges_computeWeights(&rg, debug);
    assert(ier == 0);

    ier = mnt_regridedges_print(&rg);
    assert(ier == 0);
    
    std::string outputFilename = "simpleRegridEdgesWeights.nc";
    ier = mnt_regridedges_dumpWeights(&rg, outputFilename.c_str(), (int) outputFilename.size());
    assert(ier == 0);

    // edges point in the positive direction
    //
    //     3
    //  o--<---o
    //  |      |
    //4 ^      V 2
    //  |      |
    //  o-->---o
    //     1
    double srcData[] = {1., -2., -3., 4.};
    double dstData[4];

    ier = mnt_regridedges_apply(&rg, srcData, dstData, MNT_UNIQUE_EDGE_DATA);
    assert(ier == 0);

    double srcCellByCellData[] = {1., -2., -3., 4.};
    double dstCellByCellData[4];

    ier = mnt_regridedges_apply(&rg, srcCellByCellData, dstCellByCellData, MNT_CELL_BY_CELL_DATA);
    assert(ier == 0);

    ier = mnt_regridedges_del(&rg);
    assert(ier == 0);

    for (size_t i = 0; i < 4; ++i) {
        std::cout << "src edge " << i << " data value " << srcData[i] << ' ' << srcCellByCellData[i] << '\n';
    }
    for (size_t i = 0; i < 4; ++i) {
        std::cout << "dst edge " << i << " data value " << dstData[i] << ' ' << dstCellByCellData[i] << '\n';
        // expect the same value as srcData
        assert(std::abs(dstData[i] - srcData[i]) < 1.e-12);
        assert(std::abs(srcCellByCellData[i] - dstCellByCellData[i]) < 1.e-12);
    }

}
