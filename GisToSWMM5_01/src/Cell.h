#ifndef _STRING_H_
#define _STRING_H_
#include <string>
#endif

#ifndef _VECTOR_H_
#define _VECTOR_H_
#include <vector>
#endif

class Cell
{
	public:
		// Methods.
		Cell();
		~Cell();

		// Variables.
		std::string name;
		double centerCoordX;
		double centerCoordY;
		double elevation;
		double cellSize;
		double slope;                // Average surface slope (%)
		double area;                 // Area of subcatchment (ha)
		double flowWidth;            // Width of overland flow (m)
		int landuse;
		std::string outlet;          // Name of node or another subcatchment that receives runoff
		std::string raingage;        // Rain gage assigned to subcatchment (name)
		std::string imperv;          // Percent of impervious area (%)
		std::string snowPack;        // Name of snow pack parameter set (for snow melt analysis only)
		std::string N_Imperv;        // Mannings N for impervious area (-)
		std::string N_Perv;          // Mannings N for pervious area (-)
		std::string S_Imperv;        // Depth of depression storage on impervious area (mm)
		std::string S_Perv;          // Depth of depression storage on pervious area (mm)
		std::string PctZero;		 // percent of impervious area with no depression storage (%)
		std::string RouteTo;         // choice of internal routing between pervious and impervious sub-areas (OUTLET)
		std::string PctRouted;       // Percent of runoff routed between sub-areas (%)
		std::string Suction;         // Soil capillary suction head (mm)
		std::string HydCon;          // Soil saturated hydraulic conductivity (mm/hr)
		std::string IMDmax;          // Difference between soil porosity and initial moisture content (a fraction)
		//int neighCellIndices[8];
		std::vector<int> neighCellIndices;
		std::vector<double> distanceToNeighbours;
};
