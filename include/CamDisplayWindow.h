#include "SDLGLWindow.h"
#include "Shader.h"
#include "GLCamera.h"
#include "Drawable.h"

class CamDisplayWindow : public SDLGLWindow
{
	Shader m_Shader;
	GLCamera m_Camera;
	Drawable m_PictureQuad;

public:
	CamDisplayWindow( std::string strName, int posX, int posY, int width, int height, int flags,
					  int glMajor, int glMinor, bool bDoubleBuf,
					  std::string strVertShader, std::string strFragShader,
					  int texWidth, int texHeight, float fQuadSize );

	void Draw();
};
