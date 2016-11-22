#include "PhaseExperiments.h"
#include <iostream>
#include <stdlib.h>
#include <fstream>

// Constructor
PhaseExperiments::PhaseExperiments(std::string outdir) 
{
	m_outdir = outdir;
}

// Deconstructor
PhaseExperiments::~PhaseExperiments() {}

// Methods

/**
 * Description
 *   This function saves the pixel values of a selection of random points
 *   in a contour over all the depths.  Right now the 'random' points are
 *   not seeded so it will return the same random points every time the 
 *   program is run. Now this may actually be nice to have to be able
 *   to compare the same set of points but if we decide to change it, the
 *   static function randu can be seeded by:
 *     
 *       cv::theRNG().state = seed;
 *
 *   A seed of the current time will make it produce new results with each
 *   run.
 *
 * Parameters
 *   [Contour *] c: the contour to inspect points from
 *   [int] samples: the number of points to inspect
 *   [int] start_depth: the depth to start at (inclusive)
 *   [int] stop_depth: the depth to stop at (inclusive)
 *   [int] step: the size of a jump to make in between depths
 *
 * Returns
 *   void
 *
 * TODO
 *   [+] finish the rough version of the method
 */
void PhaseExperiments::randomContourPixels(Contour *c, int samples, int start_depth, int stop_depth, int step) {
	int n = c->getArea();
	if (samples > n) {
		samples = n;
	}
	
	std::vector<int> indexes(samples);
	std::vector<cv::Point> & pointsRef = *c->getPoints();	
	cv::randu(indexes, 0, n);	
	
	std::ofstream out;
	out.open(m_outdir + "/" + "phaseTesting_" + std::to_string(((long long)c))  + ".txt");

	for (int i : indexes) {
		cv::Point p = pointsRef[i];
		out << "(" << p.x << ", " << p.y << ")\n";
	}
	
	out.close();
	
	if (!out) {
		std::cerr << "Error occured writing phase testing file\n";
	}
}



