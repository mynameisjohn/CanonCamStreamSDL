#include "CamDisplayWindow.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "CameraApp.h"

//#include <SDL_image.h>
#include <SDL.h>
#include <opencv2/opencv.hpp>

GLuint initTexture( void * PXA, int w, int h )
{
	//Generate the device texture and bind it
	GLuint tex( 0 );
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	//Upload host texture to device
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, PXA );
	
	// Set filtering   
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// Unbind, return handle to device texture
	glBindTexture( GL_TEXTURE_2D, 0 );

	return tex;
}

CamDisplayWindow::CamDisplayWindow( CameraApp * pApp, std::string strName, int posX, int posY, int width, int height, int flags,
									int glMajor, int glMinor, bool bDoubleBuf,
									std::string strVertShader, std::string strFragShader,
									float fQuadSize ) :
	SDLGLWindow( strName, posX, posY, width, height, flags, glMajor, glMinor, bDoubleBuf ),
	m_pCamApp( pApp ),
	m_bTexCreated( false ),
	m_bUploadReadImg( false ),
	m_EvfImgRef( nullptr ),
	m_EvfStmRef( nullptr )
{
	// Create shader and camera
	m_Shader.Init( strVertShader, strFragShader, true );
	m_Camera.InitOrtho(width, height, -1, 1, -1, 1 );

	// Bind shader
	auto sBind = m_Shader.ScopeBind();

	// Set shader variable handles
	Drawable::SetPosHandle( m_Shader.GetHandle( "a_Pos" ) );
	Drawable::SetTexHandle( m_Shader.GetHandle( "a_Tex" ) );
	Drawable::SetColorHandle( m_Shader.GetHandle( "u_Color" ) );

	// Don't know about this one
	//glUniform1i( m_Shader.GetHandle( "u_TexSampler" ), GL_TEXTURE20 );

	// Create drawable with texture coords
	std::array<glm::vec3, 4> arTexCoords{
		glm::vec3( -fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, fQuadSize, 0 ),
		glm::vec3( -fQuadSize, fQuadSize, 0 )
	};
	m_PictureQuad.Init( "PictureQuad", arTexCoords, vec4( 1 ), quatvec(), vec2( 1, 1 ) );

	// Init read and write streams
	//memset( &m_ReadImg, 0, sizeof( EdsImgStream ) );
	//memset( &m_WriteImg, 0, sizeof( EdsImgStream ) );
}

bool CamDisplayWindow::updateImage()
{
	// Either create a new texture and upload it
	// or update the existing texture
	std::lock_guard<std::mutex> lg( m_muEVFImage );
	if ( m_bUploadReadImg && m_ReadImg.empty() == false )
	{
		if ( m_bTexCreated )
		{
			glBindTexture( GL_TEXTURE_2D, m_PictureQuad.GetTexID() );
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, m_ReadImg.cols, m_ReadImg.rows, GL_RGB, GL_FLOAT, m_ReadImg.ptr() );
			glBindTexture( GL_TEXTURE_2D, 0 );
		}
		else
		{
			GLuint uTexID = initTexture( m_ReadImg.ptr(), m_ReadImg.cols, m_ReadImg.rows );
			if ( uTexID )
			{
				m_PictureQuad.SetTexID( uTexID );
				m_bTexCreated = true;
			}
		}

		m_bUploadReadImg = false;
		return true;
	}

	return false;
}

void CamDisplayWindow::Draw()
{
	//glActiveTexture( m_Shader.GetHandle( "u_TexSampler" ) );
	
	updateImage();

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

	//glActiveTexture( 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void equalizeIntensityHist( cv::Mat& in )
{
	// Convert to YCbCr and equalize histogram, recombine and unconvert
	cv::Mat matYCR;
	cv::cvtColor( in, matYCR, CV_RGB2YCrCb );
	std::vector<cv::Mat> vChannels;
	cv::split( matYCR, vChannels );
	cv::equalizeHist( vChannels[0], vChannels[0] );
	cv::merge( vChannels, matYCR );
	cv::cvtColor( matYCR, in, CV_YCrCb2RGB );
	in.convertTo( in, CV_32FC3 );
};

bool CamDisplayWindow::HandleEVFImage()
{
	if ( m_pCamApp == nullptr )
		return true;

	EdsError err = EDS_ERR_OK;

	// Create the stream once
	if ( m_EvfStmRef == nullptr )
	{
		err = EdsCreateMemoryStream( 2, &m_EvfStmRef );
	}

	// Create EvfImageRef.
	if ( err == EDS_ERR_OK )
	{
		// Release any old references (?)
		if ( m_EvfImgRef )
		{
			err = EdsRelease( m_EvfImgRef );
			m_EvfImgRef = nullptr;
		}

		// Create new image ref with our stream
		err = EdsCreateEvfImageRef( m_EvfStmRef, &m_EvfImgRef );
	}

	// Download live view image data.
	if ( err == EDS_ERR_OK )
	{
		err = EdsDownloadEvfImage( m_pCamApp->GetCamModel()->getCameraObject(), m_EvfImgRef );
	}

	// Convert JPG to img, post
	if ( err == EDS_ERR_OK )
	{
		// Get image data/size
		void * pData( nullptr );
		EdsUInt64 uDataSize( 0 );
		err = EdsGetLength( m_EvfStmRef, &uDataSize );
		if ( err == EDS_ERR_OK )
		{
			err = EdsGetPointer( m_EvfStmRef, &pData );
			if ( err == EDS_ERR_OK )
			{
				if ( uDataSize && pData )
				{
					// Convert from JPG to rgb24
					cv::Mat matJPG( 1, uDataSize, CV_8UC1, pData );
					cv::Mat matImg = cv::imdecode( matJPG, 1 );
					if ( matImg.empty() || matImg.type() != CV_8UC3 )
					{
						throw std::runtime_error( "Error decoding JPG image!" );
					}

					equalizeIntensityHist( matImg );
					matImg /= (float) 0xFF;
					m_vImageStack.emplace_back( std::move( matImg ) );
				}
			}
		}

		// Every 10 images, collapse the stack
		if ( m_vImageStack.size() >= 10 )
		{
			// Average the pixels of every image in the stack
			cv::Mat avgImg( m_vImageStack.front().rows, m_vImageStack.front().cols, m_vImageStack.front().type() );
			avgImg.setTo( 0 );
			const float fDiv = 1.f / m_vImageStack.size();
			for ( cv::Mat& img : m_vImageStack )
				avgImg += fDiv * img;

			//equalizeIntensityHist( avgImg );

			// Lock mutex, post to read image and set flag
			{
				std::lock_guard<std::mutex> lg( m_muEVFImage );
				m_ReadImg = avgImg;
				m_bUploadReadImg = true;
			}

			// Clear stack
			m_vImageStack.clear();
		}

		// Enqueue the next download command, get out
		if ( m_pCamApp->GetIsEvfRunning() )
			m_pCamApp->GetCmdQueue()->push_back( new DownloadEvfCommand( m_pCamApp->GetCamModel(), this ) );
		else
			return true;

		return true;
	}

	// Something bad happened
	return false;
}