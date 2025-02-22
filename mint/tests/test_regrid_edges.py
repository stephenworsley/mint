from mint import RegridEdges, Grid, UNIQUE_EDGE_DATA
import numpy
from pathlib import Path


DATA_DIR = Path(__file__).absolute().parent.parent.parent / Path('data')


def test_loadFromUgrid2DData():

    # src grid
    sg = Grid()
    sg.setFlags(0, 0) # lon-lat
    filename = str(DATA_DIR / Path('lonlatzt_100x50x3x2.nc'))
    sg.loadFromUgrid2DFile(f'{filename}$mesh2d')

    # dst grid
    dg = Grid()
    dg.setFlags(0, 0)

    # 2 cells/faces
    #
    # 3..1<...2...>5..5
    # :       :       :
    # v       v       ^
    # 6   0   3   1   4
    # :       :       :
    # 0..>2...1...>0..4
    xyz = numpy.array([(0.,0.,0.),
                       (1.,0.,0.),
                       (1.,1.,0.),
                       (0.,1.,0.),
                       (2.,0.,0.),
                       (2.,1.,0.),])
    face2nodes = numpy.array([(0,1,2,3), (2,1,4,5)], numpy.uint64)
    edge2nodes = numpy.array([(1,4), (2,3), (0,1), (2,1), (4,5), (2,5), (3,0)], numpy.uint64)
    dg.loadFromUgrid2DData(xyz, face2nodes, edge2nodes)

    # create a regridder
    rg = RegridEdges()
    rg.setSrcGrid(sg)
    rg.setDstGrid(dg)
    rg.buildLocator(numCellsPerBucket=128, periodX=0., enableFolding=False)
    rg.computeWeights()


def test_set_grids():

    # create and load src grid
    sg = Grid()
    sg.setFlags(0, 0) # lon-lat
    filename = str(DATA_DIR / Path('lonlatzt_100x50x3x2.nc'))
    sg.loadFromUgrid2DFile(f'{filename}$mesh2d')

    # create and load dst grid
    dg = Grid()
    dg.setFlags(1, 1) # cubed-sphere
    filename = str(DATA_DIR / Path('lfric_diag_wind.nc'))
    dg.loadFromUgrid2DFile(f'{filename}$Mesh2d')

    # create a regridder
    rg = RegridEdges()
    rg.setSrcGrid(sg)
    rg.setDstGrid(dg)
    rg.buildLocator(numCellsPerBucket=128, periodX=360., enableFolding=True)
    rg.computeWeights()


def test_compute_weights():

    # create a regridder
    rg = RegridEdges()

    # src is lon-lat grid so set the flags to 0, 0. This will
    # not apply any transformations to the grid
    rg.setSrcGridFlags(0, 0)

    # src grid file
    src_file = str(DATA_DIR / Path('latlon4x2.nc'))
    # load the src grid, latlon is the name of the mesh stored in
    # the netcdf file
    rg.loadSrcGrid(f'{src_file}$latlon')

    # dst is cubed-sphere. Cells at the poles need to be fixed to
    # make them as compact as possible. Use flags 1, 1
    rg.setDstGridFlags(1, 1)

    # dst grid file
    dst_file = str(DATA_DIR / Path('cs_4.nc'))
    # load the dst grid, physics is the name of the mesh stored in
    # the netcdf file
    rg.loadDstGrid(f'{dst_file}$physics')

    # build the locator. numCellsPerBucket is used to
    # accelerate the cell search. The smaller numCellPerBucket the
    # faster the search. However, there are edge cases where the
    # search fails when numCellsPerBucket is too small. periodX is
    # the periodicity length to add/subtract to make the cells well
    # behaved (periodX can be 0 if a regional model). Parameter 
    # enableFolding allows for |latitude| > 90.
    rg.buildLocator(numCellsPerBucket=128, periodX=360., enableFolding=False)

    # compute the regridding weights. debug = 2 saves the bad
    # edges in a VTK file
    rg.computeWeights(debug=2)

    # save the weights in a netCDF file
    rg.dumpWeights('test_regrid_edges_py.nc')


def test_apply_weights():

    # create a regridder
    rg = RegridEdges()

    # src is lon-lat grid
    rg.setSrcGridFlags(0, 0)
    rg.loadSrcGrid(f'{DATA_DIR}/latlon4x2.nc$latlon')

    # dst is cubed-sphere
    rg.setDstGridFlags(1, 1)
    rg.loadDstGrid(f'{DATA_DIR}/cs_4.nc$physics')

    # load the weights
    rg.loadWeights('test_regrid_edges_py.nc')

    num_src_edges = rg.getNumSrcEdges()
    num_dst_edges = rg.getNumDstEdges()
    print(f'number of edges (src/dst): {num_src_edges}/{num_dst_edges}')

    # create some mock field
    src_data = numpy.array(range(0, num_src_edges), numpy.float64)

    # allocate some space to receive the data
    dst_data = numpy.empty((num_dst_edges,), numpy.float64)

    # apply the weights to the src field, will fill in dst_data
    rg.apply(src_data, dst_data, placement=UNIQUE_EDGE_DATA)

    check_sum = numpy.abs(dst_data).sum()
    assert(abs(check_sum - 515.8441563902018) < 1.e-8)
    print(f'check sum test passsed: {check_sum}')


def test_identity():

    srcGrid = Grid()
    srcGrid.setFlags(fixLonAcrossDateline=0, averageLonAtPole=0, degrees=True) # uniform lat-lon
    filename = str(DATA_DIR / Path('latlon4x2.nc'))
    meshname = 'latlon'
    srcGrid.loadFromUgrid2DFile(f'{filename}${meshname}')

    # destination and source grids are the same
    dstGrid = Grid()
    dstGrid.setFlags(fixLonAcrossDateline=0, averageLonAtPole=0, degrees=True) # uniform lat-lon
    filename = str(DATA_DIR / Path('latlon4x2.nc'))
    meshname = 'latlon'
    dstGrid.loadFromUgrid2DFile(f'{filename}${meshname}')

    regridder = RegridEdges()

    # even though the grids are the same, we still need to create two 
    # grid instances
    regridder.setSrcGrid(srcGrid)
    regridder.setDstGrid(dstGrid)

    # compute the weights
    regridder.buildLocator(numCellsPerBucket=128, periodX=360., enableFolding=False)
    regridder.computeWeights(debug=2)

    # create a mock field
    num_edges = srcGrid.getNumberOfEdges()
    src_data = numpy.array(range(0, num_edges), numpy.float64)

    # allocate space to receive the interpolated data
    dst_data = numpy.empty((num_edges,), numpy.float64)

    # apply the interpolation weights
    regridder.apply(src_data, dst_data, placement=UNIQUE_EDGE_DATA)

    diff = src_data - dst_data
    for i in range(num_edges):
        print(f'{i} {diff[i]}')
    print(f'src data = {src_data}')
    print(f'dst data = {dst_data}')

    # check that we recover the original field
    error = numpy.fabs(diff).sum()
    print(f'error = {error}')
    assert error < 1.e-6


def test_identity2():

    regridder = RegridEdges()

    # lat-lon grid
    fixLonAcrossDateline = 0
    averageLonAtPole = 0
    regridder.setSrcGridFlags(fixLonAcrossDateline, averageLonAtPole)
    regridder.setDstGridFlags(fixLonAcrossDateline, averageLonAtPole)

    filename = str(DATA_DIR / Path('latlon4x2.nc'))
    meshname = 'latlon'
    # destination and source grids are the same
    regridder.loadSrcGrid(f'{filename}${meshname}')
    regridder.loadDstGrid(f'{filename}${meshname}')

    # compute the weights
    regridder.buildLocator(numCellsPerBucket=128, periodX=360., enableFolding=False)
    regridder.computeWeights(debug=2)

    # create a mock field
    num_edges = regridder.getNumSrcEdges()
    src_data = numpy.array(range(0, num_edges), numpy.float64)

    # allocate space to receive the interpolated data
    dst_data = numpy.empty((num_edges,), numpy.float64)

    # apply the interpolation weights
    regridder.apply(src_data, dst_data, placement=UNIQUE_EDGE_DATA)

    diff = src_data - dst_data
    for i in range(num_edges):
        print(f'{i} {diff[i]}')
    print(f'src data = {src_data}')
    print(f'dst data = {dst_data}')

    # check that we recover the original field
    error = numpy.fabs(diff).sum()
    print(f'error = {error}')
    assert error < 1.e-6


if __name__ == '__main__':

    test_compute_weights()
    test_apply_weights()
