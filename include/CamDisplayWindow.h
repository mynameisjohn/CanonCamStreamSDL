#pragma once

#include "SDLGLWindow.h"
#include "Shader.h"
#include "GLCamera.h"
#include "Drawable.h"
#include "DownloadEvfCommand.h"
#include "StarFinder.h"

#include <mutex>
#include <opencv2/core/mat.hpp>

class CameraApp;
//class SDL_Surface;

class CamDisplayWindow : public SDLGLWindow, public DownloadEvfCommand::Receiver
{
	Shader m_Shader;
	GLCamera m_Camera;
	Drawable m_PictureQuad;
	CameraApp * m_pCamApp;

	//struct EdsImgStream
	//{
	//	EdsStreamRef stmRef;
	//	EdsImageRef imgRef;
	//	cv::Mat imgMat;
	//	//SDL_Surface * imgSurf;
	//};

	EdsStreamRef m_EvfStmRef;
	EdsStreamRef m_EvfImgRef;
	std::vector<cv::Mat> m_vImageStack;
	cv::Mat m_ReadImg;

	bool m_bTexCreated;
	std::mutex m_muEVFImage;
	bool m_bUploadReadImg;
	//	EdsImgStream m_ReadImg, m_WriteImg;

	bool updateImage();

	StarFinder m_StarFinder;

public:
	CamDisplayWindow( CameraApp * pApp, std::string strName, int posX, int posY, int width, int height, int flags,
					  int glMajor, int glMinor, bool bDoubleBuf,
					  std::string strVertShader, std::string strFragShader,
					  float fQuadSize );

	void Draw();

	bool HandleEVFImage() override;

	void SetStarFinderParams( float fFilterRadius, float fDilationRadius, float fHWHM, float fIntensityThreshold )
	{
		m_StarFinder.SetStarFinderParams( fFilterRadius, fDilationRadius, fHWHM, fIntensityThreshold );
	}
};
