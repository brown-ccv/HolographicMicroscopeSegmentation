///\file main.cpp
///\author Benjamin Knorlein
///\date 08/10/2016

#define NOMINMAX

#include "ImageCache.h"
#include "OctopusClient.h"
#include "OfflineReader.h"
#include "ReportWriter.h"
#include "Settings.h"
#include "ContourDetection.h"
#include "PhaseExperiments.h"

#include <iostream>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS

#ifndef WITH_CONSOLE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <chrono>
#include <ctime>

#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include "Contour.h"
#include "ContourDepthDetection.h"
#include "ContourMerge.h"

using namespace cv;

std::string slash = "/"; 

int main(int argc, char** argv)
{
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	if (argc < 3)
	{
		std::cerr << "You need to provide a filename and a settings.xml" << std::endl;
		return 0;
	}
	
	std::string filename = std::string(argv[1]);
	Settings * settings = new Settings(argv[2]);

	std::string outFile = settings->getOutputFolder() + slash + filename;

#ifdef _MSC_VER
	CreateDirectory(outFile.c_str(), NULL);
#else
	mkdir(outFile.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

	//setup ReportWriter
	//ReportWriter * writer = new ReportWriter(settings->getOutputFolder(), filename);
	ReportWriter *writer = new ReportWriter(settings, filename);
	//setup ImageCache
	ImageCache * cache;
	if (settings->getOnline())
	{
		cache = new ImageCache(new OctopusClient(settings->getIp(), settings->getPort()),settings, 500);
	} 
	else
	{
		cache = new ImageCache(new OfflineReader(), settings, settings->getMaxImageCacheStorage());
	}
	cache->getImageSource()->setSourceHologram(settings->getDatafolder(), filename);

////////Find contours
	ContourDetection * detector = new ContourDetection(cache, settings);
	std::vector<Contour *> contours;

	detector->generateMaxMap();
	writer->saveImage(*detector->getMaxImage(), "maximum.png", true);

	detector->findContours(contours);
	writer->saveImage(detector->getDepthImage(),"depthImage.png");

	delete detector;
	writer->saveContourImage(contours, settings);

////////Find best depth for contour
	ContourDepthDetection * depthdetector = new ContourDepthDetection(cache, settings);
	for (int i = 0; i < contours.size(); i++)
		depthdetector->findBestDepth(contours[i], settings->getMinDepth(), settings->getMaxDepth(), settings->getStepSize());
	delete depthdetector;

//merge bounds
	if (settings->getDoMergebounds()){
		int count = 1;
		while (count > 0){
			ContourMerge * merger = new ContourMerge(settings);
			count = merger->mergeContours(contours);
			delete merger;
			//find best depth for contours if a merge occured
			if (count > 0){
				ContourDepthDetection * depthdetector = new ContourDepthDetection(cache, settings);
				for (int i = 0; i < contours.size(); i++) {
					depthdetector->findBestDepth(contours[i], settings->getMinDepth(), settings->getMaxDepth(), settings->getStepSize());
				}
				delete depthdetector;
			}
		}
	}

///////Refine depths
	if (settings->getDoRefine() && settings->getOnline())
	{
		ContourDepthDetection * depthdetector = new ContourDepthDetection(cache, settings);
		for (int i = 0; i < contours.size(); i++)
			depthdetector->findBestDepth(contours[i], contours[i]->getDepth() - settings->getStepSize(), contours[i]->getDepth() + settings->getStepSize(), settings->getStepSize() / 10.0);
		delete depthdetector;
	}

////////Phase Experimetal Testing	
	PhaseExperiments *pe = new PhaseExperiments(settings, cache, outFile);
	pe->randomContourPixels(contours[1], 5, 10000, 12000, 100);
	pe->randomContourPixels(contours[3], 10, 10000, 12000, 100);
	
////////Create Report
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	writer->writeXMLReport(contours, std::chrono::duration_cast<std::chrono::minutes>(end - begin).count());
	writer->saveROIImages(cache, contours);
	writer->saveContourImage(contours, settings);

////////Cleanup
	delete settings;
	delete cache->getImageSource();
	delete cache;
	delete writer;
	return 0;
}

