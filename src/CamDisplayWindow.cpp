#include "CamDisplayWindow.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

GLuint initTexture( void * PXA, int w, int h )
{
	GLuint tex;

	//Generate the device texture and bind it
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	//Upload host texture to device
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, PXA );

	//Does this really help?
	glGenerateMipmap( GL_TEXTURE_2D );

	// Set filtering   
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	//This necessary?
	float aniso = 0.0f;
	glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso );

	//Unbind, Delete the host texture, return handle to device texture
	glBindTexture( GL_TEXTURE_2D, 0 );

	return tex;
}

CamDisplayWindow::CamDisplayWindow( std::string strName, int posX, int posY, int width, int height, int flags,
									int glMajor, int glMinor, bool bDoubleBuf,
									std::string strVertShader, std::string strFragShader,
									int texWidth, int texHeight, float fQuadSize ) :
	SDLGLWindow( strName, posX, posY, width, height, flags, glMajor, glMinor, bDoubleBuf )
{
	m_Shader.Init( strVertShader, strFragShader, true );
	m_Camera.InitOrtho(width, height, -1, 1, -1, 1 );

	auto sBind = m_Shader.ScopeBind();

	Drawable::SetPosHandle( m_Shader.GetHandle( "a_Pos" ) );
	Drawable::SetTexHandle( m_Shader.GetHandle( "a_Tex" ) );
	Drawable::SetColorHandle( m_Shader.GetHandle( "u_Color" ) );
	glUniform1i( m_Shader.GetHandle( "u_TexSampler" ), 0 );

	std::array<glm::vec3, 4> arTexCoords{
		glm::vec3( -fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, fQuadSize, 0 ),
		glm::vec3( -fQuadSize, fQuadSize, 0 )
	};
	m_PictureQuad.Init( "PictureQuad", arTexCoords, vec4( 1 ), quatvec(), vec2( 1, 1 ) );

	std::vector<int> vTexData( texWidth * texHeight, 0 );
	m_PictureQuad.SetTexID( initTexture( vTexData.data(), texWidth, texHeight ) );
}

void CamDisplayWindow::Draw()
{
	Updater update( GetWindow() );
	Shader::ScopedBind sBind = m_Shader.ScopeBind();
	// Get the camera mat as well as some handles
	GLuint pmvHandle = m_Shader.GetHandle( "u_PMV" );
	GLuint clrHandle = m_Shader.GetHandle( "u_Color" );
	mat4 P = m_Camera.GetCameraMat();
	mat4 PMV = P * m_PictureQuad.GetMV();
	vec4 c = m_PictureQuad.GetColor();
	glUniformMatrix4fv( pmvHandle, 1, GL_FALSE, glm::value_ptr( PMV ) );
	glUniform4fv( clrHandle, 1, glm::value_ptr( c ) );
	m_PictureQuad.Draw();
}