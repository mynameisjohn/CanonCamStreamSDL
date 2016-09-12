#pragma once

#include "SDLGLWindow.h"
#include "Shader.h"
#include "GLCamera.h"
#include "Drawable.h"

#include <mutex>

#include "DownloadEvfCommand.h"

class CameraApp;
class SDL_Surface;

class CamDisplayWindow : public SDLGLWindow, public DownloadEvfCommand::Receiver
{
	Shader m_Shader;
	GLCamera m_Camera;
	Drawable m_PictureQuad;
	CameraApp * m_pCamApp;

	struct EdsImgStream
	{
		EdsStreamRef stmRef;
		EdsImageRef imgRef;
		SDL_Surface * imgSurf;
	};

	bool m_bTexCreated;
	std::mutex m_muEVFImage;
	bool m_bUploadReadImg;
	EdsImgStream m_ReadImg, m_WriteImg;

	bool updateImage();

public:
	CamDisplayWindow( CameraApp * pApp, std::string strName, int posX, int posY, int width, int height, int flags,
					  int glMajor, int glMinor, bool bDoubleBuf,
					  std::string strVertShader, std::string strFragShader,
					  int texWidth, int texHeight, float fQuadSize );

	void Draw();

	bool HandleEVFImage() override;
};
