#pragma once

#include "SDLGLWindow.h"
#include "Shader.h"
#include "GLCamera.h"
#include "Drawable.h"

#include <mutex>

#include "DownloadEvfCommand.h"

class CameraApp;

class CamDisplayWindow : public SDLGLWindow, public DownloadEvfCommand::Receiver
{
	Shader m_Shader;
	GLCamera m_Camera;
	Drawable m_PictureQuad;
	CameraApp * m_pCamApp;

	std::mutex m_muEVFImage;
	EdsStreamRef m_ImgRef;

public:
	CamDisplayWindow( CameraApp * pApp, std::string strName, int posX, int posY, int width, int height, int flags,
					  int glMajor, int glMinor, bool bDoubleBuf,
					  std::string strVertShader, std::string strFragShader,
					  int texWidth, int texHeight, float fQuadSize );

	void Draw();

	bool HandleEVFImage() override;
};
