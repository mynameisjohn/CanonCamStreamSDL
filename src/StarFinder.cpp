#include "StarFinder.h"

#include <opencv2/imgproc.hpp>

#include <iostream>

StarFinder::StarFinder( float fFilterRadius, float fDilationRadius, float fHWHM, float fIntensityThreshold ) :
	// These are some good defaults
	m_fFilterRadius( fFilterRadius ),
	m_fDilationRadius( fDilationRadius ),
	m_fHWHM( fHWHM ),
	m_fIntensityThreshold( fIntensityThreshold )
{}

void DoTophatFilter( const int nFilterRadius, cv::Mat& input, cv::Mat& output )
{
	int nDiameter = 2 * nFilterRadius + 1;
	cv::Mat h_Circle = cv::Mat::zeros( cv::Size( nDiameter, nDiameter ), CV_32F );
	cv::circle( h_Circle, cv::Size( nFilterRadius, nFilterRadius ), nFilterRadius, 1.f, -1 );
	h_Circle /= cv::sum( h_Circle )[0];
	cv::filter2D( input, output, CV_32F, h_Circle );
}

void DoGaussianFilter( const int nFilterRadius, const double dSigma, cv::Mat& input, cv::Mat& output )
{
	int nDiameter = 2 * nFilterRadius + 1;
	cv::GaussianBlur( input, output, cv::Size( nDiameter, nDiameter ), dSigma );
}

void DoDilationFilter( const int nFilterRadius, cv::Mat& input, cv::Mat& output )
{
	int nDilationDiameter = 2 * nFilterRadius + 1;
	cv::Mat hDilationStructuringElement = cv::getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( nDilationDiameter, nDilationDiameter ) );
	cv::dilate( input, output, hDilationStructuringElement );
}

// Arbitrarily small number
const double kEPS = .001;

// Collapse a vector of potentiall overlapping circles
// into a vector of non-overlapping circles
std::vector<Circle> CollapseCircles( const std::vector<Circle>& vInput )
{
	std::vector<Circle> vRet;

	for ( const Circle cInput : vInput )
	{
		bool bMatchFound = false;
		for ( Circle& cMatch : vRet )
		{
			// Compute distance from input to match
			float fDistX = cMatch.fX - cInput.fX;
			float fDistY = cMatch.fY - cInput.fY;
			float fDist2 = pow( fDistX, 2 ) + pow( fDistY, 2 );
			if ( fDist2 < pow( cInput.fR + cMatch.fR, 2 ) )
			{
				// Skip if they're very close
				if ( fDist2 < kEPS )
				{
					bMatchFound = true;
					continue;
				}

				// Compute union circle
				Circle cUnion { 0 };

				// Compute unit vector from input to match
				float fDist = sqrt( fDist2 );
				float nX = fDistX / fDist;
				float nY = fDistY / fDist;

				// Find furthest points on both circles
				float x0 = cInput.fX - nX * cInput.fR;
				float y0 = cInput.fY - nY * cInput.fR;
				float x1 = cMatch.fX + nX * cMatch.fR;
				float y1 = cMatch.fY + nY * cMatch.fR;

				// The distance between these points is the diameter of the union circle
				float fUnionDiameter = sqrt( pow( x1 - x0, 2 ) + pow( y1 - y0, 2 ) );
				cUnion.fR = fUnionDiameter / 2;

				// And the center is the midpoint between the furthest points
				cUnion.fX = ( x0 + x1 ) / 2;
				cUnion.fY = ( y0 + y1 ) / 2;

				// Replace this circle with the union circle
				cMatch = cUnion;
				bMatchFound = true;
			}
		}

		// If no match found, store this one
		if ( !bMatchFound )
		{
			vRet.push_back( cInput );
		}
	}

	// Run it again if we are still collapsing
	// This recursion will break when there are
	// no more circles to collapse
	if ( vInput.size() != vRet.size() )
		vRet = CollapseCircles( vRet );

	return vRet;
}

std::vector<Circle> FindStarsInImage( float fStarRadius, cv::Mat& dBoolImg )
{
	// We need a contiguous image of bytes (which we'll be treating as bools)
	if ( dBoolImg.type() != CV_8U || dBoolImg.empty() || dBoolImg.isContinuous() == false )
		throw std::runtime_error( "Error: Stars must be found in boolean images!" );

	// Find non-zero pixels in image, create circles
	std::vector<Circle> vRet;
	int x( 0 ), y( 0 );
#pragma omp parallel for shared(vRet, dBoolImg) private (x, y)
	for ( y = 0; y < dBoolImg.rows; y++ )
	{
		for ( x = 0; x < dBoolImg.cols; x++ )
		{
			uint8_t val = dBoolImg.at<uint8_t>( y, x );
			if ( val )
			{
#pragma omp critical
				vRet.push_back( { (float) x, (float) y, fStarRadius } );
			}
		}
	}

	// Collapse star images and return
	return CollapseCircles( vRet );
}

bool StarFinder::findStars( cv::Mat img )
{
	// Need data
	if ( img.empty() )
		return false;

	// Single channel
	if ( img.channels() > 1 )
	{
		cv::Mat imgGrey;
		cv::cvtColor( img, imgGrey, CV_RGB2GRAY );
		cv::swap( img, imgGrey );
	}

	// Normalized float
	if ( img.type() != CV_32F )
	{
		const double dDivFactor = 1. / ( 1 << ( 8 * img.elemSize() / img.channels() ) );
		cv::Mat imgFloat;
		img.convertTo( imgFloat, CV_32F, dDivFactor );
		cv::swap( img, imgFloat );
	}

	// So we know what we're working with here
	if ( img.type() != CV_32F )
		throw std::runtime_error( "Error: SF requires single channel float input!" );

	// Initialize if we haven't yet
	if ( m_imgInput.empty() )
	{
		// Preallocate the GPU mats needed during computation
		m_imgGaussian = cv::Mat( img.size(), CV_32F );
		m_imgTopHat = cv::Mat( img.size(), CV_32F );
		m_imgPeak = cv::Mat( img.size(), CV_32F );
		m_imgThreshold = cv::Mat( img.size(), CV_32F );
		m_imgDilated = cv::Mat( img.size(), CV_32F );
		m_imgLocalMax = cv::Mat( img.size(), CV_32F );
		m_imgStars = cv::Mat( img.size(), CV_32F );

		// We need a contiguous boolean image for CUDA
		m_imgBoolean = cv::Mat( img.size(), CV_8U );
	}

	// Work with reference to original, but it isn't modified
	m_imgInput = img;

	// Compute filter and dilation kernel sizes from image dimensions
	int nFilterRadius = std::min<int>( 15, ( .5f + m_fFilterRadius * m_imgInput.cols ) );
	int nDilationRadius = std::min<int>( 15, ( .5f + m_fDilationRadius * m_imgInput.cols ) );

	// Apply gaussian filter to input to remove high frequency noise
	const double dSigma = m_fHWHM / ( ( sqrt( 2 * log( 2 ) ) ) );
	DoGaussianFilter( nFilterRadius, dSigma, m_imgInput, m_imgGaussian );

	// Apply linear filter to input to magnify high frequency noise
	DoTophatFilter( nFilterRadius, m_imgInput, m_imgTopHat );

	// Subtract linear filtered image from gaussian image to clean area around peak
	// Noisy areas around the peak will be negative, so threshold negative values to zero
	cv::subtract( m_imgGaussian, m_imgTopHat, m_imgPeak );
	cv::threshold( m_imgPeak, m_imgPeak, 0, 1, cv::THRESH_TOZERO );

	// Create a thresholded image where the lowest pixel value is m_fIntensityThreshold
	m_imgThreshold.setTo( cv::Scalar( m_fIntensityThreshold ) );
	cv::max( m_imgPeak, m_imgThreshold, m_imgThreshold );

	// Create the dilated image (initialize its pixels to m_fIntensityThreshold)
	m_imgDilated.setTo( cv::Scalar( m_fIntensityThreshold ) );
	DoDilationFilter( nDilationRadius, m_imgThreshold, m_imgDilated );

	// Subtract the dilated image from the gaussian peak image
	// What this leaves us with is an image where the brightest
	// gaussian peak pixels are zero and all others are negative
	cv::subtract( m_imgPeak, m_imgDilated, m_imgLocalMax );

	// Exponentiating that image makes those zero pixels 1, and the
	// negative pixels some low number; threshold to drop them
	cv::exp( m_imgLocalMax, m_imgLocalMax );
	cv::threshold( m_imgLocalMax, m_imgStars, 1 - kEPS, 1 + kEPS, cv::THRESH_BINARY );

	// This star image is now a boolean image - convert it to bytes (TODO you should add some noise)
	m_imgStars.convertTo( m_imgBoolean, CV_8U, 0xff );

	return true;
}

// Just find the stars
bool StarFinder::HandleImage( cv::Mat img )
{
	if ( findStars( img ) )
	{
		// Use thrust to find stars in pixel coordinates
		const float fStarRadius = 10.f;
		std::vector<Circle> vStarLocations = FindStarsInImage( fStarRadius, m_imgBoolean );
		if ( !vStarLocations.empty() )
			std::cout << vStarLocations.size() << " stars found!" << std::endl;

		// Draw circles at stars in input
		// TODO should I be checking if we go too close to the edge?
		const int nHighlightThickness = 1;
		const cv::Scalar sHighlightColor( 0xde, 0xad, 0 );
		for ( const Circle ptStar : vStarLocations )
			cv::circle( img, cv::Point( ptStar.fX, ptStar.fY ), ptStar.fR, sHighlightColor, nHighlightThickness );

		return true;
	}

	return false;
}

void StarFinder::SetStarFinderParams( float fFilterRadius, float fDilationRadius, float fHWHM, float fIntensityThreshold )
{
	m_fFilterRadius = fFilterRadius;
	m_fDilationRadius = fDilationRadius;
	m_fHWHM = fHWHM;
	m_fIntensityThreshold = fIntensityThreshold;
}