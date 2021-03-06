#include "Grid.h"

Grid::Grid()
{
	// Initialize variables.
	cells = 0;
	nCols = 0;
	nRows = 0;
	cellSize = 0.0;
	xllCorner = 0.0;
	yllCorner = 0.0;
	urcorner[0] = urcorner[1] = 0.0;
	llcorner[0] = llcorner[1] = 0.0;
}

Grid::Grid(int nColsNew, int nRowsNew)
{
	// Initialize variables.
	nCols = nColsNew;
	nRows = nRowsNew;
	cellSize = 0.0;
	xllCorner = 0.0;
	yllCorner = 0.0;
	urcorner[0] = urcorner[1] = 0.0;
	llcorner[0] = llcorner[1] = 0.0;
	cells = new Cell[ nCols * nRows ];
}

Grid::Grid(Raster &landuseRaster, Raster &demRaster)
{
	// Initialize variables.
	cells = 0;
	nCols = 0;
	nRows = 0;
	cellSize = 0.0; // save cell size to cell objects
	xllCorner = 0.0;
	yllCorner = 0.0;
	urcorner[0] = urcorner[1] = 0.0;
	llcorner[0] = llcorner[1] = 0.0;

	// Create cells.
	int numberOfCellsX;
	int numberOfCellsY;
	double cellSizeMax = 8.0;
	numberOfCellsX = (int)((double)landuseRaster.nCols * landuseRaster.cellSize / cellSizeMax + 0.5);
	numberOfCellsY = (int)((double)landuseRaster.nRows * landuseRaster.cellSize / cellSizeMax + 0.5);
	double lowerLeftCornerX =  landuseRaster.xllCorner;
	double lowerLeftCornerY =  landuseRaster.yllCorner;
	std::vector<Cell> cellsAdaptive;

	for (int i = 0; i < numberOfCellsX * numberOfCellsY; i++)
	{
		int cellIndexL0X = i % numberOfCellsX;
		int cellIndexL0Y = i / numberOfCellsX;
		double lowerLeftCornerLocX = lowerLeftCornerX + cellIndexL0X * 8.0;
		double lowerLeftCornerLocY = lowerLeftCornerY + cellIndexL0Y * 8.0;

		if (isSingleLanduse(lowerLeftCornerLocX, lowerLeftCornerLocY, 8.0, landuseRaster))
		{
			Cell cell;
			cell.cellSize = 8.0;
			cell.centerCoordX = lowerLeftCornerLocX + cell.cellSize * 0.5;
			cell.centerCoordY = lowerLeftCornerLocY + cell.cellSize * 0.5;
			std::string landuse = landuseRaster.getPixelValue(cell.centerCoordX, cell.centerCoordY);
			cell.landuse = atoi(landuse.c_str());
			cellsAdaptive.push_back(cell);
		}
		else
		{
			for (int j = 0; j < 4; j++)
			{
				int cellIndexL1X = j % 2;
				int cellIndexL1Y = j / 2;
				double lowerLeftCornerLocX = lowerLeftCornerX + cellIndexL0X * 8.0 + cellIndexL1X * 4.0;
				double lowerLeftCornerLocY = lowerLeftCornerY + cellIndexL0Y * 8.0 + cellIndexL1Y * 4.0;

				if (isSingleLanduse(lowerLeftCornerLocX, lowerLeftCornerLocY, 8.0, landuseRaster))
				{
					Cell cell;
					cell.cellSize = 4.0;
					cell.centerCoordX = lowerLeftCornerLocX + cell.cellSize * 0.5;
					cell.centerCoordY = lowerLeftCornerLocY + cell.cellSize * 0.5;
					std::string landuse = landuseRaster.getPixelValue(cell.centerCoordX, cell.centerCoordY);
					cell.landuse = atoi(landuse.c_str());
					cellsAdaptive.push_back(cell);
				}
				else
				{
					for (int k = 0; k < 4; k++)
					{
						int cellIndexL2X = k % 2;
						int cellIndexL2Y = k / 2;
						double lowerLeftCornerLocX = lowerLeftCornerX + cellIndexL0X * 8.0 + cellIndexL1X * 4.0 + cellIndexL2X * 2.0;
						double lowerLeftCornerLocY = lowerLeftCornerY + cellIndexL0Y * 8.0 + cellIndexL1Y * 4.0 + cellIndexL2Y * 2.0;

						Cell cell;
						cell.cellSize = 2.0;
						cell.centerCoordX = lowerLeftCornerLocX + cell.cellSize * 0.5;
						cell.centerCoordY = lowerLeftCornerLocY + cell.cellSize * 0.5;
						std::string landuse = landuseRaster.getPixelValue(cell.centerCoordX, cell.centerCoordY);
						cell.landuse = atoi(landuse.c_str());
						cellsAdaptive.push_back(cell);
					}
				}
			}
		}
	}

	// Transfer adaptive cell data to the pointer array.
	nCols = (int)cellsAdaptive.size();
	nRows = 1;
	cells = new Cell[nRows * nCols];

	for (int i = 0; i < nRows * nCols; i++ )
	{
		cells[i].cellSize = cellsAdaptive[i].cellSize;
		cells[i].centerCoordX = cellsAdaptive[i].centerCoordX;
		cells[i].centerCoordY = cellsAdaptive[i].centerCoordY;
		cells[i].landuse = cellsAdaptive[i].landuse;
	}

	// Save elevation values to cells.
	for (int i = 0; i < nRows * nCols; i++ )
	{
		std::vector<std::string> catLabels;
		std::vector<int> catCount;
		computeHistogram(catLabels, catCount, cells[i].centerCoordX - 0.5 * cells[i].cellSize,
			cells[i].centerCoordY - 0.5 * cells[i].cellSize, cells[i].cellSize, demRaster);
		double elevation = 0;
		int count = 0;

		for (int j = 0; j < (int)catLabels.size(); j++)
		{
			elevation += atof( catLabels[j].c_str() ) * (double)catCount[j];
			count += catCount[j];
		}

		if (count > 0)
		{
			elevation /= (double)count;
			cells[i].elevation = elevation;
		}
		else
		{
			std::cout << "\n-> Error, no elevation data was found for the cell " << i;
		}
	}

	// Save neighbour cell indices to cells.
	double thresholdDistance = 0.5;

	for (int i = 0; i < nRows * nCols; i++ )
	{
		std::vector<int> neighbours;

		for (int j = 0; j < nRows * nCols; j++ )
		{
			//if (
			// cells[j].gridPos == cells[i].gridPos - 1 || cells[j].gridPos == cells[i].gridPos + 1 ||
			// (cells[j].gridPos >= cells[i].gridPos - cells[i].gridColumns - 1 && cells[j].gridPos <= cells[i].gridPos - cells[i].gridColumns + 1) ||
			// (cells[j].gridPos >= cells[i].gridPos + cells[i].gridColumns - 1 && cells[j].gridPos <= cells[i].gridPos + cells[i].gridColumns + 1)
			//{

			if
				(
				(
				fabs((cells[i].centerCoordX + cells[i].cellSize) - (cells[j].centerCoordX - cells[j].cellSize)) < thresholdDistance
				||
				fabs((cells[i].centerCoordX - cells[i].cellSize) - (cells[j].centerCoordX + cells[j].cellSize)) < thresholdDistance
				)
				&&
				(
				fabs((cells[i].centerCoordY + cells[i].cellSize) - (cells[j].centerCoordY - cells[j].cellSize)) < thresholdDistance
				||
				fabs((cells[i].centerCoordY - cells[i].cellSize) - (cells[j].centerCoordY + cells[j].cellSize)) < thresholdDistance
				)
				)
			{
				//if
				//	(
				//	fabs((cells[i].centerCoordY + cells[i].cellSize) - (cells[j].centerCoordY - cells[j].cellSize)) < thresholdDistance
				//	||
				//	fabs((cells[i].centerCoordY - cells[i].cellSize) - (cells[j].centerCoordY + cells[j].cellSize)) < thresholdDistance
				//	)
				//{
					if (i != j && cells[j].landuse != LANDUSE_NONE)
					{
						neighbours.push_back(j);
					}
				//}
			}

			//}

		}

		cells[i].neighCellIndices = neighbours;
	}
}

Grid::~Grid()
{
	delete [] cells;
}

void Grid::computeHistogram(std::vector<std::string> &catLabels, std::vector<int> &catCount,
	double lowerLeftCornerX, double lowerLeftCornerY, double cellSizeLoc, Raster &landuseRaster)
{
	catLabels.clear();
	catCount.clear();
	int indexMinX = (int)((lowerLeftCornerX - landuseRaster.xllCorner) / landuseRaster.cellSize + 0.5);
	int indexMaxX = (int)((lowerLeftCornerX + cellSizeLoc - landuseRaster.xllCorner) / landuseRaster.cellSize + 0.5);
	int indexMinY = (int)((lowerLeftCornerY - landuseRaster.yllCorner) / landuseRaster.cellSize + 0.5);
	int indexMaxY = (int)((lowerLeftCornerY + cellSizeLoc - landuseRaster.yllCorner) / landuseRaster.cellSize + 0.5);

	if (indexMinX < 0)
	{
		indexMinX = 0;
	}
	if (indexMinY < 0)
	{
		indexMinY = 0;
	}
	if (indexMaxX > landuseRaster.nCols)
	{
		indexMaxX = landuseRaster.nCols;
	}
	if (indexMaxY > landuseRaster.nRows)
	{
		indexMaxY = landuseRaster.nRows;
	}

	for (int j = indexMinY; j < indexMaxY; j++)
	{
		for (int i = indexMinX; i < indexMaxX; i++)
		{
			double coordinateX = landuseRaster.xllCorner + i * landuseRaster.cellSize + landuseRaster.cellSize * 0.5;
			double coordinateY = landuseRaster.yllCorner + j * landuseRaster.cellSize + landuseRaster.cellSize * 0.5;
			std::string landuse = landuseRaster.getPixelValue(coordinateX, coordinateY);
			bool landuseCatFound = false;

			for (int k = 0; k < (int)catLabels.size(); k++)
			{
				if (catLabels[k] == landuse)
				{
					catCount[k]++;
					landuseCatFound = true;
				}
			}

			if (landuseCatFound == false)
			{
				catLabels.push_back(landuse);
				catCount.push_back(1);
			}
		}
	}
}

bool Grid::isSingleLanduse(double lowerLeftCornerX, double lowerLeftCornerY, double cellSizeLoc, Raster &landuseRaster)
{
	std::vector<std::string> catLabels;
	std::vector<int> catCount;
	computeHistogram(catLabels, catCount, lowerLeftCornerX, lowerLeftCornerY, cellSizeLoc, landuseRaster);

	int numberOfCategories = 0;

	for (int i = 0; i < (int)catCount.size(); i++)
	{
		if (catCount[i] > 0)
		{
			numberOfCategories++;
		}
	}

	if (numberOfCategories == 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Grid::setCellNames()
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		std::stringstream sstream("");
		sstream << "s" << i;
		cells[i].name = sstream.str();
	}
}

void Grid::computeCellDimensions(double cellSizeNew)
{
	cellSize = cellSizeNew;

	for (int i = 0; i < nCols * nRows; i++)
	{
		cells[i].area = cellSize * cellSize;
		cells[i].cellSize = cellSize;
	}
}

void Grid::computeCellCenterpoints(double xllCornerNew, double yllCornerNew)
{
	xllCorner = xllCornerNew; // is this needed anymore here?
	yllCorner = yllCornerNew; // is this needed anymore here?

	for (int j = 0; j < nRows; j++)
	{
		for (int i = 0; i < nCols; i++)
		{
			double centerCoordX = (double)i * cellSize + 0.5 * cellSize + xllCorner;
			double centerCoordY = (double)j * cellSize + 0.5 * cellSize + yllCorner;
			cells[ i + j * nCols ].centerCoordX = centerCoordX;
			cells[ i + j * nCols ].centerCoordY = centerCoordY;
		}
	}
}

void Grid::setCellElevations(Raster &demRaster)
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		std::string elevation = demRaster.getPixelValue( cells[i].centerCoordX, cells[i].centerCoordY );
		cells[i].elevation = atof(elevation.c_str());
	}
}

void Grid::setCellLanduse(Raster &landuseRaster)
{
	for (int j = 0; j < nRows; j++)
	{
		for (int i = 0; i < nCols; i++)
		{
			std::string landuseType = landuseRaster.getPixelValue( i, nRows - 1 - j );
			cells[ i + j * nCols ].landuse = atoi(landuseType.c_str());
		}
	}
}

void Grid::computeGridExtents()
{
	//urcorner[0] = llcorner[0] = xllCorner + 0.5 * nCols * cellSize;
	//urcorner[1] = llcorner[1] = yllCorner + 0.5 * nRows * cellSize;

	if (nCols * nRows > 0)
	{
		urcorner[0] = llcorner[0] = cells[0].centerCoordX;
		urcorner[1] = llcorner[1] = cells[0].centerCoordY;
	}

	for (int i = 0; i < nCols * nRows; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			if (cells[i].centerCoordX < llcorner[0])
			{
				llcorner[0] = cells[i].centerCoordX;
			}

			if (cells[i].centerCoordX > urcorner[0])
			{
				urcorner[0] = cells[i].centerCoordX;
			}

			if (cells[i].centerCoordY < llcorner[1])
			{
				llcorner[1] = cells[i].centerCoordY;
			}

			if (cells[i].centerCoordY > urcorner[1])
			{
				urcorner[1] = cells[i].centerCoordY;
			}
		}
	}
}

void Grid::setSubcatchmentProperties(Table &catchPropTable)
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		for (int j = 1; j < catchPropTable.nRows; j++)
		{
			if (cells[i].landuse == atoi(catchPropTable.data[j * catchPropTable.nCols].c_str()))
			{
				cells[i].raingage = catchPropTable.data[j * catchPropTable.nCols + 7];
				cells[i].imperv = catchPropTable.data[j * catchPropTable.nCols + 1].c_str();
				cells[i].N_Imperv = catchPropTable.data[j * catchPropTable.nCols + 3].c_str();
				cells[i].N_Perv = catchPropTable.data[j * catchPropTable.nCols + 5].c_str();
				cells[i].S_Imperv = catchPropTable.data[j * catchPropTable.nCols + 2].c_str();
				cells[i].S_Perv = catchPropTable.data[j * catchPropTable.nCols + 4].c_str();
				cells[i].PctZero = catchPropTable.data[j * catchPropTable.nCols + 6].c_str();
				cells[i].Suction = catchPropTable.data[j * catchPropTable.nCols + 10].c_str();
				cells[i].HydCon = catchPropTable.data[j * catchPropTable.nCols + 8].c_str();
				cells[i].IMDmax = catchPropTable.data[j * catchPropTable.nCols + 9].c_str();
				cells[i].snowPack = catchPropTable.data[j * catchPropTable.nCols + 11];

				break;
			}
		}
	}
}

void Grid::findCellNeighbours()
{
	for (int j = 0; j < nRows; j++)
	{
		for (int i = 0; i < nCols; i++)
		{
			int currentCell = i + j * nCols;
			cells[currentCell].neighCellIndices.assign(8, -1);
			cells[currentCell].distanceToNeighbours.assign(8, 0.0 );

			if (j < nRows - 1)
			{
				cells[currentCell].neighCellIndices[0] = currentCell + nCols;
			}

			if (j < nRows - 1 && i > 0)
			{
				cells[currentCell].neighCellIndices[1] = currentCell + nCols - 1;
			}

			if (i > 0)
			{
				cells[currentCell].neighCellIndices[2] = currentCell - 1;
			}

			if (j > 0 && i > 0)
			{
				cells[currentCell].neighCellIndices[3] = currentCell - nCols - 1;
			}

			if (j > 0)
			{
				cells[currentCell].neighCellIndices[4] = currentCell - nCols;
			}

			if (i < nCols - 1 && j > 0)
			{
				cells[currentCell].neighCellIndices[5] = currentCell + 1 - nCols;
			}

			if (i < nCols - 1)
			{
				cells[currentCell].neighCellIndices[6] = currentCell + 1;
			}

			if (i < nCols - 1 && j < nRows - 1)
			{
				cells[currentCell].neighCellIndices[7] = currentCell + nCols + 1;
			}
		}
	}
}

// TJN 15 Mar 2017 START Update to more recent version
void Grid::routeCells()
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		double elevation = cells[i].elevation;
		int neighCellIndex = -1;
		cells[i].flowWidth = cells[i].cellSize; //Modified 20160909
		int flowDirection = -1;

		// Compute distances between neighbour cells.
		for (int j = 0; j < (int)cells[i].neighCellIndices.size(); j++)
		{
			if (cells[i].neighCellIndices[j] != -1)
			{
				double distance = sqrt( (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
				* (cells[i].centerCoordX - cells[ cells[i].neighCellIndices[j] ].centerCoordX)
				+ (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY)
				* (cells[i].centerCoordY - cells[ cells[i].neighCellIndices[j] ].centerCoordY) );

				if (distance > 0.0)
				{
					cells[i].distanceToNeighbours[j] = distance;
				}
				else
				{
					std::cout << "\n-> Error, distance between cell " << i << " and cell " << j << " is zero or below zero";
				}
			}
		}

		// Find outlet.
		for (int k = 0; k < (int)cells[i].neighCellIndices.size(); k++)
		{
			if (cells[i].neighCellIndices[k] != -1 && cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_ROOF_CONNECTED
				&& cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_ROOF_UNCONNECTED
				&& cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_NONE)
			{
				if (cells[ cells[i].neighCellIndices[k] ].elevation < elevation)
				{
					elevation = cells[ cells[i].neighCellIndices[k] ].elevation;
					neighCellIndex = cells[i].neighCellIndices[k];
					flowDirection = k;
				}
			}
		}

		// Save the outlet name and compute flow width.
		if (neighCellIndex != -1)
		{
			cells[i].outlet = cells[neighCellIndex].name;

			if (cells[i].distanceToNeighbours[flowDirection] > 0.0)
			{
				cells[i].flowWidth = cells[i].area / cells[i].distanceToNeighbours[flowDirection];
			}
			//else  //Modified 20160909
			//{
			//	cells[i].flowWidth = 0.0;
			//}

			// Update flow width if the flow direction is in a diagonal direction.
			//if (gridType == 0 && flowDirection != -1 && flowDirection % 2 != 0)
			//{
			//	cells[i].flowWidth = cells[i].cellSize / 1.4142135624;
			//}

		}
	}
}

void Grid::computeCellSlopes()
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		double slope = 0.0;
		int slopeCount = 0;

		//if (cells[i].landuse != LANDUSE_NONE && (int)cells[i].neighCellIndices.size() > 0)
		//{
		//	std::cout << "\n-> Cell " << i << " neighbour count: " <<  (int)cells[i].neighCellIndices.size();
		//}

		for (int k = 0; k < (int)cells[i].neighCellIndices.size(); k++)
		{
			if (cells[i].neighCellIndices[k] != -1 && cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_ROOF_CONNECTED
				&& cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_ROOF_UNCONNECTED
				&& cells[ cells[i].neighCellIndices[k] ].landuse != LANDUSE_NONE)
			{
				// Compute slope between cells.
				//double distance = 0.0;

				//if (gridType == 0)
				//{
				//	distance = cells[i].cellSize;
				//
				//	if (k % 2 != 0)
				//	{
				//		distance *= 1.4142135624; // approximate of square root of two
				//	}
				//}
				// Compute distance between cells in an adaptive grid.
				//else if (gridType == 1)
				//{
				//	distance = 0.5 * (cells[i].cellSize + cells[ cells[i].neighCellIndices[k] ].cellSize);
				//}

				double distance = cells[i].distanceToNeighbours[k];

				if (distance > 0.0)
				{
					slope += fabs((cells[i].elevation - cells[ cells[i].neighCellIndices[k] ].elevation) / distance);
					slopeCount++;
				}
				else
				{
					std::cout << "\n-> Error, distance between cell " << i << " and " << cells[i].neighCellIndices[k] << " is zero or below zero.";
				}
			}
		}

		// Save the average slope.
		if (slopeCount > 0)
		{
			cells[i].slope = slope / (double)slopeCount;
		}

		// Fix slope for the rooftops.
		if (cells[i].landuse == LANDUSE_ROOF_CONNECTED || cells[i].landuse == LANDUSE_ROOF_UNCONNECTED)
		{
			cells[i].slope = 0.05;
		}

		// Set minimum slope
		if (cells[i].slope < 0.001)
		{
			cells[i].slope = 0.001;
		}
	}
}
// TJN 15 Mar 2017 END

void Grid::connectCellsToJunctions(Table &juncTable)
{
	for (int k = 1; k < juncTable.nRows; k++) // pass the header line
	{
		double juncPosX = atof( juncTable.data[k * juncTable.nCols].c_str() );
		double juncPosY = atof( juncTable.data[k * juncTable.nCols + 1].c_str() );
		int isOpen = atoi( juncTable.data[k * juncTable.nCols + 6].c_str() );
		//isOpen = 0;

		if (juncPosX >= xllCorner && juncPosX < xllCorner + nCols * cellSize
			&& juncPosY >= yllCorner && juncPosY < yllCorner + nRows * cellSize && isOpen == 1) // pass closed junctions
		{
			if (nCols > 0 && nRows > 0 && cellSize > 0.0)
			{
				int col = (int)((juncPosX - xllCorner) / cellSize);
				int row = (int)((juncPosY - yllCorner) / cellSize);

				if (col + row * nCols >= 0 && col + row * nCols < nCols * nRows)
				{
					cells[ col + row * nCols ].outlet = juncTable.data[k * juncTable.nCols + 2];
					cells[ col + row * nCols ].flowWidth = cells[ col + row * nCols ].cellSize; //Modified 20160909     // TJN 15 Mar 2017 Added this from more recent version
				}
			}
		}
	}
}

void Grid::routePavedPitAndRooftopCells(Table &juncTable)
{
	// Route paved pit cells and connected rooftop cells to junctions.
	double distanceMaxSquared = (urcorner[0] - llcorner[0]) * (urcorner[0] - llcorner[0]) +
		(urcorner[1] - llcorner[1]) * (urcorner[1] - llcorner[1]);

	for (int i = 0; i < nCols * nRows; i++)
	{
		if (cells[i].landuse == LANDUSE_ROOF_CONNECTED
			|| (cells[i].landuse == LANDUSE_ASPHALT_STREET && cells[i].outlet ==  "*")
			|| (cells[i].landuse == LANDUSE_TILES && cells[i].outlet ==  "*"))
		{

			double distanceSquared = distanceMaxSquared;

			for (int j = 1; j < juncTable.nRows; j++) // pass the header line
			{
				double juncPosX = atof( juncTable.data[j * juncTable.nCols].c_str() );
				double juncPosY = atof( juncTable.data[j * juncTable.nCols + 1].c_str() );
				double dx = juncPosX - cells[i].centerCoordX;
				double dy = juncPosY - cells[i].centerCoordY;

				if (dx * dx + dy * dy < distanceSquared)
				{
					distanceSquared = dx * dx + dy * dy;
					cells[i].outlet = juncTable.data[j * juncTable.nCols + 2];
					cells[i].flowWidth = cells[i].cellSize; //Modified 20160909, compute by area / sqrt (distanceSquared) ? // TJN 15 Mar 2017 Added this from more recent version
				}
			}

			//cells[i].outlet = cells[i].name;
		}
	}

	// Route unconnected rooftop cells to the nearest non-roof cells.
	for (int i = 0; i < nCols * nRows; i++)
	{
		if (cells[i].landuse == LANDUSE_ROOF_UNCONNECTED)
		{
			double distanceSquared = distanceMaxSquared;

			for (int j = 0; j < nCols * nRows; j++)
			{
				if (cells[j].landuse != LANDUSE_ROOF_CONNECTED && cells[j].landuse != LANDUSE_ROOF_UNCONNECTED
					&& cells[j].landuse != LANDUSE_NONE)
				{
					double dx = cells[i].centerCoordX - cells[j].centerCoordX;
					double dy = cells[i].centerCoordY - cells[j].centerCoordY;

					if (dx * dx + dy * dy < distanceSquared)
					{
						distanceSquared = dx * dx + dy * dy;
						cells[i].outlet = cells[j].name;
						cells[i].flowWidth = cells[i].cellSize; //Modified 20160909, compute by area / sqrt (distanceSquared) ? // TJN 15 Mar 2017 Added this from more recent version
					}
				}
			}
		}
	}
}

void Grid::routePitCells()
{
	for (int i = 0; i < nCols * nRows; i++)
	{
		if (cells[i].outlet ==  "*" &&
			(cells[i].landuse == LANDUSE_SAND || cells[i].landuse == LANDUSE_VEGETATION
			|| cells[i].landuse == LANDUSE_WATER || cells[i].landuse == LANDUSE_BEDROCK))
		{
			cells[i].outlet = cells[i].name;

                        // TJN 15 Mar 2017 Added this from more recent version
                        // Set depression storage of pit cells in permeable areas to a very high value ...
                        // ... to prevent loss of water from the system.
                        cells[i].S_Imperv = "50000";
                        cells[i].S_Perv = "50000";
		}
	}
}

void Grid::saveRaster(std::string path)
{
	Raster outputRaster;
	outputRaster.pathName = path;
	outputRaster.nCols = nCols;
	outputRaster.nRows = nRows;
	outputRaster.xllCorner = xllCorner;
	outputRaster.yllCorner = yllCorner;
	outputRaster.cellSize = cellSize;
	std::string noValue = "-1";
	outputRaster.noDataValue = noValue;
	outputRaster.data = new std::string[outputRaster.nCols * outputRaster.nRows];

	for (int j = 0; j < outputRaster.nRows; j++)
	{
		for (int i = 0; i < outputRaster.nCols; i++)
		{
			std::stringstream sstream;
			sstream << std::fixed;
			sstream.precision(8);

			if (cells[ i + j * nCols ].landuse != LANDUSE_NONE)
			{
				sstream << cells[ i + j * nCols ].slope * 100.0;
			}
			else
			{
				sstream << noValue.c_str();
			}

			int flippedIndex = outputRaster.nRows - 1 - j;
			outputRaster.data[ i + flippedIndex * outputRaster.nCols ] = sstream.str();
		}
	}

	outputRaster.save( outputRaster.pathName );
}

// TJN 15 Mar 2017 Add cross section table to input parameters
void Grid::saveSWMM5File(Table &headerTable, Table &evaporationTable, Table &temperatureTable, Table &reportTable, Table &snowpacksTable, Table &raingagesTable, Table &symbolsTable, Table &juncTable, Table &outfallsTable, Table &condTable, Table &xsectionTable, std::string path)
{
    // TJN 13 Mar 2017 START Update to more recent version
	std::stringstream sstream;
	sstream << std::fixed;

	// If the output file is a PEST template file, write an identifier (ptf #) into the begining of the file
	if (path.substr ((int)path.length() - 3, 3) == "TPL")
	{
		std::cout << "\n-> test print: " << path.substr ((int)path.length() - 3, 3);
		sstream << "ptf #\n";
	}

	// Write the header table.
	std::cout << "\n-> Writing header table";
	sstream << "[TITLE]";
	sstream << "\n;;Project Title/Notes";
	sstream << "\n;;Generated with the RasterToSWMM5 program";
	sstream << "\n";
	sstream << "\n[OPTIONS]";
	sstream << "\n;Option Value";
	headerTable.writeToStringStream(sstream);
	sstream << "\n";

	// Write the evaporation table.
	std::cout << "\n-> Writing evaporation table";
	sstream << "\n[EVAPORATION]";
	sstream << "\n;;Evap Data      Parameters";
	sstream << "\n;;-------------- ----------------";
	evaporationTable.writeToStringStream(sstream);
	sstream << "\n";

	// Write the temperature table.
	std::cout << "\n-> Writing temperature table";
	sstream << "\n[TEMPERATURE]";
	sstream << "\n;;Temp/Wind/Snow   Source/Data";
	temperatureTable.writeToStringStream(sstream);
	sstream << "\n";

	// Write raingage properties.
	std::cout << "\n-> Writing raingages";
	sstream << "\n[RAINGAGES]";
	sstream << "\n;;Gage           Format    Interval SCF      Source";
	sstream << "\n;;-------------- --------- ------ ------ ----------";
	raingagesTable.writeToStringStream(sstream);
	sstream << "\n";
// TJN 13 Mar 2017 END

	// Write subcatchment properties.
	std::cout << "\n-> Writing subcatchment properties";

	sstream << "\n[SUBCATCHMENTS]";
	sstream << "\n;;Subcatchment   Rain Gage        Outlet           Area     %Imperv  Width    %Slope   CurbLen  Snow Pack       ";
	sstream << "\n;;-------------- ---------------- ---------------- -------- -------- -------- -------- -------- ----------------";

	for (int i = 0; i < nRows * nCols; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			sstream << "\n;" << cells[i].landuse;
			sstream << "\n" << cells[i].name << "   ";
			sstream << cells[i].raingage << "   ";
			sstream << cells[i].outlet << "   ";
			sstream.precision(5);
			sstream << cells[i].area / 10000.0 << "   "; // convert m2 to ha
			sstream.precision(2);
			sstream << cells[i].imperv << "   ";
			sstream << cells[i].flowWidth << "   ";
			sstream.precision(4);
			sstream << cells[i].slope * 100.0 << "   "; // convert fraction to percentage * 100.0
			sstream.precision(2);
			sstream << cells[i].cellSize << "   ";
			sstream << cells[i].snowPack << "   ";
		}
	}

	sstream << "\n";

	// Write subarea properties.
	std::cout << "\n-> Writing subarea properties";

	sstream << "\n[SUBAREAS]";
	sstream << "\n;;Subcatchment   N-Imperv   N-Perv     S-Imperv   S-Perv     PctZero    RouteTo    PctRouted ";
	sstream << "\n;;-------------- ---------- ---------- ---------- ---------- ---------- ---------- ----------";

	for (int i = 0; i < nRows * nCols; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			sstream << "\n" << cells[i].name << "   ";
			//sstream.precision(3);
			sstream << cells[i].N_Imperv << "   ";
			sstream << cells[i].N_Perv << "   ";
			//sstream.precision(2);
			sstream << cells[i].S_Imperv << "   ";
			sstream << cells[i].S_Perv << "   ";
			sstream << cells[i].PctZero << "   ";
			sstream << cells[i].RouteTo << "   ";
			sstream << cells[i].PctRouted << "   ";
		}
	}

	sstream << "\n";

	// Write infiltration properties.
	std::cout << "\n-> Writing infiltration properties";

	sstream << "\n[INFILTRATION]";
	sstream << "\n;;Subcatchment   Suction    HydCon     IMDmax    ";
	sstream << "\n;;-------------- ---------- ---------- ----------";

	for (int i = 0; i < nRows * nCols; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			sstream << "\n" << cells[i].name << "   ";
			//sstream.precision(2);
			sstream << cells[i].Suction << "   ";
			//sstream.precision(4);
			sstream << cells[i].HydCon << "   ";
			//sstream.precision(2);
			sstream << cells[i].IMDmax << "   ";
		}
	}

	sstream << "\n";

	// TJN 13 Mar 2017 START Update to more recent version
    // Write the snowpacks table.
	std::cout << "\n-> Writing snowpacks table";
	sstream << "\n[SNOWPACKS]";
	sstream << "\n;;Name           Surface    Parameters";
	sstream << "\n;;-------------- ---------- ----------";
	snowpacksTable.writeToStringStream(sstream);
	sstream << "\n";
	// TJN 13 Mar 2017 END

    // TJN 13 Mar 2017 START Update to more recent version
    // Write junction properties.
	std::cout << "\n-> Writing junction properties";
	sstream << "\n[JUNCTIONS]";
	sstream << "\n;;Junction       Invert     MaxDepth   InitDepth  SurDepth   Aponded   ";
	sstream << "\n;;-------------- ---------- ---------- ---------- ---------- ----------";

	for (int i = 1; i < juncTable.nRows; i++)
	{
		sstream << "\n" << juncTable.getData(i, 2) << "   ";
		sstream << juncTable.getData(i, 4) << "   ";
		sstream << juncTable.getData(i, 5) << "   ";
		sstream << juncTable.getData(i, 7) << "   ";
		sstream << juncTable.getData(i, 8) << "   ";
		sstream << juncTable.getData(i, 9) << "   ";
	}

	sstream << "\n";
    // TJN 13 Mar 2017 END

    // TJN 13 Mar 2017 START Update to more recent version
	// Write outfall properties.
	std::cout << "\n-> Writing outfalls properties";
	sstream << "\n[OUTFALLS]";
	sstream << "\n;;Outfall        Invert     Type       Stage Data       Gated   ";
	sstream << "\n;;-------------- ---------- ---------- ---------------- --------";

	for (int i = 1; i < outfallsTable.nRows; i++)
	{
		sstream << "\n" << outfallsTable.data[i * outfallsTable.nCols + 2] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols + 3] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols + 4] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols + 5] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols + 6] << "   ";
	}

	sstream << "\n";
	// TJN 13 Mar 2017 END

	// Write conduits properties.
	std::cout << "\n-> Writing conduits properties";

	sstream << "\n[CONDUITS]";
	sstream << "\n;;Conduit        From Node        To Node          Length     Roughness  InOffset   OutOffset  InitFlow   MaxFlow   ";
	sstream << "\n;;-------------- ---------------- ---------------- ---------- ---------- ---------- ---------- ---------- ----------";

	for (int i = 1; i < condTable.nRows; i++)
	{
	    // TJN 13 Mar 2017 START Update to more recent version
		sstream << "\n" << condTable.data[i * condTable.nCols + 4] << "   ";
		sstream << condTable.data[i * condTable.nCols + 8] << "   ";
		sstream << condTable.data[i * condTable.nCols + 9] << "   ";
		sstream << condTable.data[i * condTable.nCols + 7] << "   ";
		sstream << condTable.data[i * condTable.nCols + 10] << "   "; // roughness
		sstream << condTable.data[i * condTable.nCols + 11] << "   ";
		sstream << condTable.data[i * condTable.nCols + 12] << "   ";
		sstream << 0.0 << "   ";
		sstream << 0.0 << "   ";
		// TJN 13 Mar 2017 END
	}

	sstream << "\n";

    // TJN 15 Mar 2017 START Update to newer version
    // Write conduit cross section properties.
	std::cout << "\n-> Writing conduit cross section properties";
	sstream << "\n[XSECTIONS]";
	sstream << "\n;;Link           Shape        Geom1            Geom2      Geom3      Geom4      Barrels   ";
	sstream << "\n;;-------------- ------------ ---------------- ---------- ---------- ---------- ----------";
	xsectionTable.writeToStringStream(sstream);
	sstream << "\n";
	// TJN 15 Mar 2017 END



	// TJN 13 Mar 2017 START Update to more recent version
    // Write reporting settings.
	std::cout << "\n-> Writing reporting settings";
	sstream << "\n[REPORT]";
	sstream << "\n;;Reporting Options";
	reportTable.writeToStringStream(sstream);
    sstream << "\n";
    // TJN 13 Mar 2017 END

    // Write tags
	sstream << "\n";
	sstream << "\n[TAGS]";
	sstream << "\n";

	// Write map settings.
	std::cout << "\n-> Writing map settings";

	sstream << "\n[MAP]";
	sstream << "\nDIMENSIONS ";
	sstream.precision(2);
	sstream << llcorner[0] * 0.99999 << " ";
	sstream << llcorner[1] * 0.99999 << " ";
	sstream << urcorner[0] * 1.00001 << " ";
	sstream << urcorner[1] * 1.00001;
	sstream << "\nUnits      Meters";
	sstream << "\n";

	// Write node coordinates.
	std::cout << "\n-> Writing node coordinates";

	sstream << "\n[COORDINATES]";
	sstream << "\n;;Node           X-Coord            Y-Coord           ";
	sstream << "\n;;-------------- ------------------ ------------------";

	for (int i = 1; i < juncTable.nRows; i++)
	{
		sstream << "\n" << juncTable.data[i * juncTable.nCols + 2] << "   ";
		sstream << juncTable.data[i * juncTable.nCols + 0] << "   ";
		sstream << juncTable.data[i * juncTable.nCols + 1] << "   ";
	}

	for (int i = 1; i < outfallsTable.nRows; i++)
	{
		sstream << "\n" << outfallsTable.data[i * outfallsTable.nCols + 2] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols] << "   ";
		sstream << outfallsTable.data[i * outfallsTable.nCols + 1] << "   ";
	}

	sstream << "\n";
	sstream << "\n[VERTICES]";
	sstream << "\n;;Link           X-Coord            Y-Coord           ";
	sstream << "\n;;-------------- ------------------ ------------------";
	sstream << "\n";

	// Write polygon vertex coordinates.
	std::cout << "\n-> Writing polygon vertex coordinates";

	sstream << "\n[Polygons]";
	sstream << "\n;;Subcatchment   X-Coord            Y-Coord           ";
	sstream << "\n;;-------------- ------------------ ------------------";

	for (int i = 1; i < nRows * nCols; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			sstream.precision(2);
			sstream << "\n" << cells[i].name << "   ";
			sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << "   ";
			sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
			sstream << "\n" << cells[i].name << "   ";
			sstream << std::fixed << cells[i].centerCoordX - 0.5 * cells[i].cellSize << "   ";
			sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
			sstream << "\n" << cells[i].name << "   ";
			sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << "   ";
			sstream << std::fixed << cells[i].centerCoordY - 0.5 * cells[i].cellSize;
			sstream << "\n" << cells[i].name << "   ";
			sstream << std::fixed << cells[i].centerCoordX + 0.5 * cells[i].cellSize << "   ";
			sstream << std::fixed << cells[i].centerCoordY + 0.5 * cells[i].cellSize;
		}
	}

	sstream << "\n";

	// TJN 13 Mar 2017 START Update to more recent version
    // Write symbols.
	std::cout << "\n-> Writing symbols";
	sstream << "\n[SYMBOLS]";
	sstream << "\n;;Gage           X-Coord            Y-Coord";
	sstream << "\n;;-------------- ------------------ ------------------";
	symbolsTable.writeToStringStream(sstream);
	sstream << "\n";
	sstream << "\n";
	// TJN 13 Mar 2017 END

	// Write the file to disk.
	std::cout << "\n-> Writing the file to disk";

	FileIO fileio;
	int res = fileio.saveAsciiFile( path, sstream.str() );
}

void Grid::printReport(Table &catchPropTable)
{
    // TJN 13 Mar 2017 START Update to more recent version
	double elevationAverage = 0.0;
	double slopeAverage = 0.0;
	int numOfLandUseClasses = catchPropTable.nRows - 1;

	if (numOfLandUseClasses < 0)
	{
		numOfLandUseClasses = 0;
	}

	std::vector<int> cellsInlandUseClass;
	cellsInlandUseClass.assign(numOfLandUseClasses, 0);
	std::vector<int> landUseClassIds;
	landUseClassIds.assign(numOfLandUseClasses, 0);
	std::vector<double> areasInlandUseClass;
	areasInlandUseClass.assign(numOfLandUseClasses, 0.0);

	for (int i = 0; i < (int)landUseClassIds.size(); i++)
	{
		landUseClassIds[i] = atoi(catchPropTable.data[(i + 1) * catchPropTable.nCols].c_str());
	}

	int numOfActiveCells = 0;
	int numOfActiveCellsNoRoofs = 0;

	for (int i = 0; i < nCols * nRows; i++)
	{
		if (cells[i].landuse != LANDUSE_NONE)
		{
			numOfActiveCells += 1;
			elevationAverage += cells[i].elevation;

			if (cells[i].landuse != LANDUSE_ROOF_CONNECTED || cells[i].landuse != LANDUSE_ROOF_UNCONNECTED)
			{
				slopeAverage += cells[i].slope;
				numOfActiveCellsNoRoofs += 1;
			}

			for (int j = 0; j < (int)landUseClassIds.size(); j++)
			{
				if (landUseClassIds[j] == cells[i].landuse)
				{
					cellsInlandUseClass[j] += 1;
					areasInlandUseClass[j] += cells[i].area;
					break;
				}
			}
		}
	}

	double area = 0.0;

	for (int i = 0; i < numOfLandUseClasses; i++)
	{
		area += areasInlandUseClass[i];
	}

	if (numOfActiveCells > 0)
	{
		elevationAverage /= (double)(numOfActiveCells);
	}

	if (numOfActiveCellsNoRoofs > 0)
	{
		slopeAverage /= (double)(numOfActiveCellsNoRoofs);
	}

	std::cout << "\n-> Total number of cells: " << nCols * nRows;
	std::cout << "\n-> Number of active cells: " << numOfActiveCells;
	std::cout << "\n-> Catchment area: " << area / 10000.0 << " ha";
	std::cout << "\n-> Catchment average elevation: " << elevationAverage << " m";
	std::cout << "\n-> Catchment average slope: " << slopeAverage;
	std::cout << "\n-> Landuse information (code, number of cells, area in ha, % of catchment):";

	for (int i = 0; i < (int)cellsInlandUseClass.size(); i++)
	{
		std::cout << "\n"
		<< "\t" << landUseClassIds[i]
		<< "\t" << cellsInlandUseClass[i]
		<< "\t" << areasInlandUseClass[i] / 10000.0
		<< "\t" << areasInlandUseClass[i] / area * 100.0;
	}
	 // TJN 13 Mar 2017 END
}
