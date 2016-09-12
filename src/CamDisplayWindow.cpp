#include "CamDisplayWindow.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "CameraApp.h"

#include <SDL_image.h>

GLuint initTexture( void * PXA, int w, int h )
{
	GLuint tex;

	//Generate the device texture and bind it
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	//Upload host texture to device
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, PXA );

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

CamDisplayWindow::CamDisplayWindow( CameraApp * pApp, std::string strName,
									int posX, int posY, int width, int height, int flags,
									int glMajor, int glMinor, bool bDoubleBuf,
									std::string strVertShader, std::string strFragShader,
									int texWidth, int texHeight, float fQuadSize ) :
	SDLGLWindow( strName, posX, posY, width, height, flags, glMajor, glMinor, bDoubleBuf ),
	m_pCamApp( pApp ),
	m_bTexCreated( false ),
	m_bUploadReadImg( false )
{
	m_Shader.Init( strVertShader, strFragShader, true );
	m_Camera.InitOrtho(width, height, -1, 1, -1, 1 );

	auto sBind = m_Shader.ScopeBind();

	Drawable::SetPosHandle( m_Shader.GetHandle( "a_Pos" ) );
	Drawable::SetTexHandle( m_Shader.GetHandle( "a_Tex" ) );
	Drawable::SetColorHandle( m_Shader.GetHandle( "u_Color" ) );
	glUniform1i( m_Shader.GetHandle( "u_TexSampler" ), GL_TEXTURE20 );

	std::array<glm::vec3, 4> arTexCoords{
		glm::vec3( -fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, -fQuadSize, 0 ),
		glm::vec3( fQuadSize, fQuadSize, 0 ),
		glm::vec3( -fQuadSize, fQuadSize, 0 )
	};
	m_PictureQuad.Init( "PictureQuad", arTexCoords, vec4( 1 ), quatvec(), vec2( 1, 1 ) );

	memset( &m_ReadImg, 0, sizeof( EdsImgStream ) );
	memset( &m_WriteImg, 0, sizeof( EdsImgStream ) );

	//EdsError err = EdsCreateMemoryStream( 2, &m_ReadImg );
	//if ( err != EDS_ERR_OK )
	//	throw std::runtime_error( "Error creating image stream!" );
	//err = EdsCreateMemoryStream( 2, &m_WriteImg );
	//if ( err != EDS_ERR_OK )
	//	throw std::runtime_error( "Error creating image stream!" );

//	std::vector<int> vTexData( texWidth * texHeight, 0 );
	//m_PictureQuad.SetTexID( initTexture( vTexData.data(), texWidth, texHeight ) );

	//EdsImageRef imgRef( nullptr );
	//EdsError err = EdsCreateMemoryStream( sizeof( int )*vTexData.size(), &imgRef );
	//if ( err != EDS_ERR_OK )
	//	throw std::runtime_error( "Error creating image stream!" );

	//void * pDataStream( nullptr );
	//EdsUInt64 uDataSize( 0 );
	//err = EdsGetPointer( imgRef, &pDataStream );
	//err = EdsGetLength( imgRef, &uDataSize );
	//m_PictureQuad.SetTexID( initTexture( vTexData.data(), texWidth, texHeight ) );

	//m_ImgRefB = imgRef;
}

bool CamDisplayWindow::updateImage()
{
	std::lock_guard<std::mutex> lg( m_muEVFImage );
	if ( m_bUploadReadImg && m_ReadImg.imgSurf )
	{
		if ( m_bTexCreated )
		{
			for ( int i = 0; i < m_ReadImg.imgSurf->w*m_ReadImg.imgSurf->h; i++ )
			{
				char * addr = &((char *) m_ReadImg.imgSurf->pixels)[i];
				*addr++ = 0xff;
			}
			glBindTexture( GL_TEXTURE_2D, m_PictureQuad.GetTexID() );
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, m_ReadImg.imgSurf->w, m_ReadImg.imgSurf->h, GL_RGB, GL_UNSIGNED_BYTE, m_ReadImg.imgSurf->pixels );
		}
		else
		{
			GLuint uTexID = initTexture( m_ReadImg.imgSurf->pixels, m_ReadImg.imgSurf->w, m_ReadImg.imgSurf->h );
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
	glActiveTexture( m_Shader.GetHandle( "u_TexSampler" ) );
	
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

	glActiveTexture( 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

bool CamDisplayWindow::HandleEVFImage()
{
	if ( m_pCamApp == nullptr )
		return true;

	EdsError err = EDS_ERR_OK;
	if ( m_WriteImg.stmRef == nullptr )
	{
		err = EdsCreateMemoryStream( 2, &m_WriteImg.stmRef );
	}

	// Create EvfImageRef.
	if ( err == EDS_ERR_OK )
	{
		if ( m_WriteImg.imgRef )
		{
			err = EdsRelease( m_WriteImg.imgRef );
			m_WriteImg.imgRef = nullptr;
		}

		err = EdsCreateEvfImageRef( m_WriteImg.stmRef, &m_WriteImg.imgRef );
	}

	// Download live view image data.
	if ( err == EDS_ERR_OK )
	{
		err = EdsDownloadEvfImage( m_pCamApp->GetCamModel()->getCameraObject(), m_WriteImg.imgRef );
	}

	if ( err == EDS_ERR_OK )
	{
		// Create SDL rwops
		void * pData( nullptr );
		EdsUInt64 uDataSize( 0 );
		err = EdsGetLength( m_WriteImg.stmRef, &uDataSize );
		if ( err == EDS_ERR_OK )
		{
			err = EdsGetPointer( m_WriteImg.stmRef, &pData );
			if ( err == EDS_ERR_OK )
			{
				if ( uDataSize && pData )
				{
					SDL_RWops * rwJpgImg = SDL_RWFromConstMem( pData, (int) uDataSize );
					if ( rwJpgImg != nullptr )
					{
						if ( m_WriteImg.imgSurf )
						{
							SDL_FreeSurface( m_WriteImg.imgSurf );
							m_WriteImg.imgSurf = nullptr;
						}
						m_WriteImg.imgSurf = IMG_LoadJPG_RW( rwJpgImg );
						SDL_RWclose( rwJpgImg );
					}
				}
			}
		}
		//// Get magnification ratio (x1, x5, or x10).
		//EdsGetPropertyData( m_WriteImg.imgRef, kEdsPropID_Evf_Zoom, 0, sizeof( m_WriteImg.data.zoom ), &m_WriteImg.data.zoom );

		//// Get position of image data. (when enlarging)
		//// Upper left coordinate using JPEG Large size as a reference.
		//EdsGetPropertyData( m_WriteImg.imgRef, kEdsPropID_Evf_ImagePosition, 0, sizeof( m_WriteImg.data.imagePosition ), &m_WriteImg.data.imagePosition );

		//// Get histogram (RGBY).
		//EdsGetPropertyData( m_WriteImg.imgRef, kEdsPropID_Evf_Histogram, 0, sizeof( m_WriteImg.data.histogram ), m_WriteImg.data.histogram );

		//// Get rectangle of the focus border.
		//EdsGetPropertyData( m_WriteImg.imgRef, kEdsPropID_Evf_ZoomRect, 0, sizeof( m_WriteImg.data.zoomRect ), &m_WriteImg.data.zoomRect );

		//// Get the size as a reference of the coordinates of rectangle of the focus border.
		//EdsGetPropertyData( m_WriteImg.imgRef, kEdsPropID_Evf_CoordinateSystem, 0, sizeof( m_WriteImg.data.sizeJpegLarge ), &m_WriteImg.data.sizeJpegLarge );

		//// Set to model.
		//m_pCamApp->GetCamModel()->setEvfZoom( m_WriteImg.data.zoom );
		//m_pCamApp->GetCamModel()->setEvfZoomPosition( m_WriteImg.data.zoomRect.point );
		//m_pCamApp->GetCamModel()->setEvfZoomRect( m_WriteImg.data.zoomRect );

		EdsImgStream oldRead = m_ReadImg;

		{
			std::lock_guard<std::mutex> lg( m_muEVFImage );
			m_ReadImg = m_WriteImg;
			m_bUploadReadImg = true;
		}

		m_WriteImg = oldRead;

		m_pCamApp->GetCmdQueue()->push_back( new DownloadEvfCommand( m_pCamApp->GetCamModel(), this ) );
		return true;
	}

	return false;

	//EdsEvfImageRef evfImage = NULL;

	//// Create EvfImageRef.
	//EdsError err = EdsCreateEvfImageRef( m_WriteImg, &evfImage );

	//// Download live view image data.
	//if ( err == EDS_ERR_OK )
	//{
	//	err = EdsDownloadEvfImage( m_pCamApp->GetCamModel()->getCameraObject(), evfImage );
	//}

	//// Get meta data for live view image data.
	//if ( err == EDS_ERR_OK )
	//{
	//	EVF_DATASET dataSet = { 0 };

	//	dataSet.stream = m_WriteImg;

	//	// Get magnification ratio (x1, x5, or x10).
	//	EdsGetPropertyData( evfImage, kEdsPropID_Evf_Zoom, 0, sizeof( dataSet.zoom ), &dataSet.zoom );

	//	// Get position of image data. (when enlarging)
	//	// Upper left coordinate using JPEG Large size as a reference.
	//	EdsGetPropertyData( evfImage, kEdsPropID_Evf_ImagePosition, 0, sizeof( dataSet.imagePosition ), &dataSet.imagePosition );

	//	// Get histogram (RGBY).
	//	EdsGetPropertyData( evfImage, kEdsPropID_Evf_Histogram, 0, sizeof( dataSet.histogram ), dataSet.histogram );

	//	// Get rectangle of the focus border.
	//	EdsGetPropertyData( evfImage, kEdsPropID_Evf_ZoomRect, 0, sizeof( dataSet.zoomRect ), &dataSet.zoomRect );

	//	// Get the size as a reference of the coordinates of rectangle of the focus border.
	//	EdsGetPropertyData( evfImage, kEdsPropID_Evf_CoordinateSystem, 0, sizeof( dataSet.sizeJpegLarge ), &dataSet.sizeJpegLarge );

	//	// Set to model.
	//	m_pCamApp->GetCamModel()->setEvfZoom( dataSet.zoom );
	//	m_pCamApp->GetCamModel()->setEvfZoomPosition( dataSet.zoomRect.point );
	//	m_pCamApp->GetCamModel()->setEvfZoomRect( dataSet.zoomRect );
	//}

	////if ( m_ImgRef != NULL )
	////{
	////	EdsRelease( stream );
	////	stream = NULL;
	////}

	//if ( evfImage != NULL )
	//{
	//	EdsRelease( evfImage );
	//	evfImage = NULL;
	//}

	////Notification of error
	//if ( err != EDS_ERR_OK )
	//{

	//	// Retry getting image data if EDS_ERR_OBJECT_NOTREADY is returned
	//	// when the image data is not ready yet.
	//	if ( err == EDS_ERR_OBJECT_NOTREADY )
	//	{
	//		return false;
	//	}

	//	// It retries it at device busy
	//	if ( err == EDS_ERR_DEVICE_BUSY )
	//	{
	//		return false;
	//	}
	//}

	//m_pCamApp->GetCmdQueue()->push_back( new DownloadEvfCommand( m_pCamApp->GetCamModel(), this ) );
	//return true;
}