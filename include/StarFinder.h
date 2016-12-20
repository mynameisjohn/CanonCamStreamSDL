#pragma once

#include <opencv2/core.hpp>

// Circle struct - made my
// own so it can be constructed
// in host and device code
struct Circle
{
	float fX;	// x pos
	float fY;	// y pos
	float fR;	// radius
};

// Base star finder class
// All it's for is calling findStars,
// which performs the signal processing
// and leaves m_dBoolImg with a byte
// image where nonzero pixels are stars
class StarFinder
{
protected:
	// Processing params
	float m_fFilterRadius;
	float m_fDilationRadius;
	float m_fHWHM;
	float m_fIntensityThreshold;

	// The images we use and their size
	cv::Mat m_imgInput;
	cv::Mat m_imgGaussian;
	cv::Mat m_imgTopHat;
	cv::Mat m_imgPeak;
	cv::Mat m_imgThreshold;
	cv::Mat m_imgDilated;
	cv::Mat m_imgLocalMax;
	cv::Mat m_imgStars;
	cv::Mat m_imgBoolean;
	cv::Mat m_imgTmp;

	// Leaves bool image with star locations
	bool findStars( cv::Mat& img );

public:
	// TODO work out some algorithm parameters,
	// it's all hardcoded nonsense right now
	StarFinder( float fFilterRadius = .03f, float fDilationRadius = .015f, float fHWHM = 2.5f, float fIntensityThreshold = .25f );

	bool HandleImage( cv::Mat& img );

	void SetStarFinderParams( float fFilterRadius, float fDilationRadius, float fHWHM, float fIntensityThreshold );
};