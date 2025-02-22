#define _USE_MATH_DEFINES // M_PI for Visual Studio
#include <mntRegridEdges.h>
#include <mntLogger.h>
#include <cmath>
#undef NDEBUG // turn on asserts
#include <cassert>

double streamFunc(const double p[]) {
    double lon = p[0];
    double lat = p[1];
    return cos(M_PI*lat/180.0) * sin(M_PI*lon/180.);
}

void test1() {

    int ier;
    std::string srcFile = "@CMAKE_SOURCE_DIR@/data/vel_cs_16.nc$physics";
    std::string dstFile = "@CMAKE_SOURCE_DIR@/data/cs_4.nc$physics";
    std::string outputFile = "weights.nc";

    RegridEdges_t* rg;

    ier = mnt_regridedges_new(&rg);
    assert(ier == 0);

    ier = mnt_regridedges_loadSrcGrid(&rg, srcFile.c_str(), (int) srcFile.size());
    assert(ier == 0);

    ier = mnt_regridedges_loadDstGrid(&rg, dstFile.c_str(), (int) dstFile.size());
    assert(ier == 0);

    std::size_t numSrcEdges, numDstEdges;

    ier = mnt_regridedges_getNumSrcEdges(&rg, &numSrcEdges);
    assert(ier == 0);

    ier = mnt_regridedges_getNumDstEdges(&rg, &numDstEdges);
    assert(ier == 0);

    int numCellsPerBucket = 8;
    double period = 0.;
    int enableFolding = 0;
    ier = mnt_regridedges_buildLocator(&rg, numCellsPerBucket, period, enableFolding);
    assert(ier == 0);

    int debug = 1;
    ier = mnt_regridedges_computeWeights(&rg, debug);
    assert(ier == 0);

    ier = mnt_regridedges_dumpWeights(&rg, outputFile.c_str(), (int) outputFile.size());
    assert(ier == 0);

    ier = mnt_regridedges_del(&rg);
    assert(ier == 0);

    // read the weights and interpolate

    ier = mnt_regridedges_new(&rg);
    assert(ier == 0);

    ier = mnt_regridedges_loadSrcGrid(&rg, srcFile.c_str(), (int) srcFile.size());
    assert(ier == 0);

    ier = mnt_regridedges_loadDstGrid(&rg, dstFile.c_str(), (int) dstFile.size());
    assert(ier == 0);

    ier = mnt_regridedges_loadWeights(&rg, outputFile.c_str(), (int) outputFile.size());
    assert(ier == 0);

    std::vector<double> srcData(numSrcEdges);
    std::vector<double> dstData(numDstEdges);

    std::string resFile = "regridded_line_integrated_velocity.nc";

    // initialize the slices by reading the metcdf metadata from file
    std::string fieldName = "line_integrated_velocity";
    std::string dstFieldFile = "line_integrated_velocity.nc";
    int append = 0; // new file
    std::size_t numSlices;
    ier = mnt_regridedges_initSliceIter(&rg, 
                                        srcFile.c_str(), (int) srcFile.size(),
                                        dstFieldFile.c_str(), (int) dstFieldFile.size(),
                                        append,
                                        fieldName.c_str(), (int) fieldName.size(),
                                        &numSlices);
    assert(ier == 0);

    for (std::size_t i = 0; i < numSlices; ++i) {
        ier = mnt_regridedges_loadSrcSlice(&rg, &srcData[0]);
        assert(ier == 0);

        int placement = MNT_UNIQUE_EDGE_DATA;
        ier = mnt_regridedges_apply(&rg, &srcData[0], &dstData[0], placement);
        assert(ier == 0);

        ier = mnt_regridedges_dumpDstSlice(&rg, &dstData[0]);
        assert(ier == 0);

        ier = mnt_regridedges_nextSlice(&rg);
        assert(ier == 0);
    }

    ier = mnt_regridedges_del(&rg);
    assert(ier == 0);

}

void regridEdgeFieldTest(const std::string& testName, const std::string& srcFile, const std::string& dstFile) {

    int ier;
    std::string outputFile = testName + "Weights.nc";

    RegridEdges_t* rg;

    ier = mnt_regridedges_new(&rg);
    assert(ier == 0);

    ier = mnt_regridedges_loadSrcGrid(&rg, srcFile.c_str(), (int) srcFile.size());
    assert(ier == 0);
    std::cerr << testName << ": loadSrc...OK\n";

    ier = mnt_regridedges_loadDstGrid(&rg, dstFile.c_str(), (int) dstFile.size());
    assert(ier == 0);
    std::cerr << testName << ": loadDst...OK\n";

    int numCellsPerBucket = 8;
    
    assert(ier == 0);
    double period = 0.;
    int enableFolding = 0;
    ier = mnt_regridedges_buildLocator(&rg, numCellsPerBucket, period, enableFolding);
    std::cerr << testName << ": build...OK\n";
    assert(ier == 0);

    int debug = 1;
    ier = mnt_regridedges_computeWeights(&rg, debug);
    std::cerr << testName << ": computeWeights...OK\n";
    assert(ier == 0);

    std::string weightFile = testName + "Weights.nc";
    ier = mnt_regridedges_dumpWeights(&rg, weightFile.c_str(), (int) weightFile.size());
    assert(ier == 0);
    std::cerr << testName << ": dump weights...OK\n";

    double p0[3];
    double p1[3];

    std::size_t numSrcCells, numSrcEdges, srcEdgeId;
    int srcEdgeSign;

    // set the source data
    ier = mnt_grid_getNumberOfCells(&rg->srcGridObj, &numSrcCells);
    assert(ier == 0);
    ier = mnt_grid_getNumberOfEdges(&rg->srcGridObj, &numSrcEdges);
    assert(ier == 0);
    std::cerr << testName << ": src num cells = " << numSrcCells << " num edges = " << numSrcEdges << "...OK\n";

    std::vector<double> srcData(numSrcEdges);
    std::vector<double> srcCellByCellData(numSrcCells * MNT_NUM_EDGES_PER_QUAD);
    double* srcPoints;
    double* pBeg;
    double* pEnd;
    ier = mnt_grid_getPointsPtr(&rg->srcGridObj, &srcPoints);
    assert(ier == 0);

    for (std::size_t srcCellId = 0; srcCellId < numSrcCells; ++srcCellId) {
        for (int ie = 0; ie < MNT_NUM_EDGES_PER_QUAD; ++ie) {

            ier = mnt_grid_getEdgeId(&rg->srcGridObj, srcCellId, ie, &srcEdgeId, &srcEdgeSign);
            assert(ier == 0);

            ier = mnt_grid_getPoints(&rg->srcGridObj, srcCellId, ie, p0, p1);
            assert(ier == 0);

            srcData[srcEdgeId] = srcEdgeSign * (streamFunc(p1) - streamFunc(p0));

            // first two edges are anticlockwise, last two are clockwise
            int edgeSign = 1;
            if (ie > 1) edgeSign = -1;
            int i0 = ie;
            int i1 = (i0 + 1) % MNT_NUM_VERTS_PER_QUAD;
            pBeg = &srcPoints[srcCellId*MNT_NUM_VERTS_PER_QUAD*3 + i0*3];
            pEnd = &srcPoints[srcCellId*MNT_NUM_VERTS_PER_QUAD*3 + i1*3];
            srcCellByCellData[srcCellId*MNT_NUM_EDGES_PER_QUAD + ie] = edgeSign*(streamFunc(pEnd) - streamFunc(pBeg));
        }
    }
    std::cerr << testName << ": src data set...OK\n";

    std::size_t numDstCells, numDstEdges, dstEdgeId;
    int dstEdgeSign;
    ier = mnt_grid_getNumberOfCells(&rg->dstGridObj, &numDstCells);
    assert(ier == 0);
    ier = mnt_grid_getNumberOfEdges(&rg->dstGridObj, &numDstEdges);
    assert(ier == 0);

    std::vector<double> dstData(numDstEdges);
    std::vector<double> dstCellByCellData(numDstCells * MNT_NUM_EDGES_PER_QUAD);

    // regrid
    ier = mnt_regridedges_apply(&rg, &srcData[0], &dstData[0], MNT_UNIQUE_EDGE_DATA);
    assert(ier == 0);

    ier = mnt_regridedges_apply(&rg, &srcCellByCellData[0], &dstCellByCellData[0], MNT_CELL_BY_CELL_DATA);
    assert(ier == 0);


    // check
    printf("%s\n dstCellId         edgeIndex        edgeId      interpVal      exact        error               p0               p1\n", testName.c_str());
    double totError = 0;
    for (std::size_t dstCellId = 0; dstCellId < numDstCells; ++dstCellId) {
        for (int ie = 0; ie < MNT_NUM_EDGES_PER_QUAD; ++ie) {

            ier = mnt_grid_getEdgeId(&rg->dstGridObj, dstCellId, ie, &dstEdgeId, &dstEdgeSign);
            assert(ier == 0);

            ier = mnt_grid_getPoints(&rg->dstGridObj, dstCellId, ie, p0, p1);
            assert(ier == 0);

            double exact = dstEdgeSign * (streamFunc(p1) - streamFunc(p0));

            double interpVal = dstData[dstEdgeId];

            assert(std::abs(dstEdgeSign*dstData[dstEdgeId] - dstCellByCellData[dstCellId*MNT_NUM_EDGES_PER_QUAD + ie]) < 1.e-8);

            double error = interpVal - exact;

            if (std::abs(error) > 1.e-6) {
                printf("%10ld           %1d         %9ld      %10.6lf   %10.6lf    %12.5lg     %5.1lf,%5.1lf      %5.1lf,%5.1lf\n", 
                    dstCellId, ie, dstEdgeId, interpVal, exact, error, p0[0], p0[1], p1[0], p1[1]);
            }
            totError += std::abs(error);
        }
    }

    std::cout << testName << ": total interpolation |error|: " << totError << '\n';
    assert(totError < 1.e-8);

    // clean up
    ier = mnt_regridedges_del(&rg);
    assert(ier == 0);
}


int main() {

    // CURRENTLY FAILS
    // regridEdgeFieldTest("lfric_diag_wind", "@CMAKE_SOURCE_DIR@/data/lfric_diag_wind.nc$Mesh2d", "@CMAKE_SOURCE_DIR@/data/cs_4.nc$physics");

    // crashes when building the cell locator
    //regridTest("tiny1x2_1x1", "@CMAKE_SOURCE_DIR@/data/tiny1x2.nc$physics", "@CMAKE_SOURCE_DIR@/data/tiny1x1.nc$physics");
    //regridTest("tiny1x1_1x2", "@CMAKE_SOURCE_DIR@/data/tiny1x1.nc$physics", "@CMAKE_SOURCE_DIR@/data/tiny1x2.nc$physics");


    regridEdgeFieldTest("edgeField_4_4", "@CMAKE_SOURCE_DIR@/data/cs_4.nc$physics", "@CMAKE_SOURCE_DIR@/data/cs_4.nc$physics");
    regridEdgeFieldTest("edgeField_16_4", "@CMAKE_SOURCE_DIR@/data/cs_16.nc$physics", "@CMAKE_SOURCE_DIR@/data/cs_4.nc$physics"); 
    regridEdgeFieldTest("edgeField_16_16", "@CMAKE_SOURCE_DIR@/data/cs_16.nc$physics", "@CMAKE_SOURCE_DIR@/data/cs_16.nc$physics");

    test1();

    return 0;
}   
