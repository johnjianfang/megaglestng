// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "texture_gl.h"

#include <stdexcept>

#include "opengl.h"
#include "leak_dumper.h"
#include <iostream>

using namespace std;

namespace Shared{ namespace Graphics{ namespace Gl{

using namespace Platform;

GLint toCompressionFormatGl(GLint format) {
	if(Texture::useTextureCompression == false) {
		return format;
	}

	//GL_COMPRESSED_ALPHA             <- white things but tile ok!
	//GL_COMPRESSED_LUMINANCE         <- black tiles
	//GL_COMPRESSED_LUMINANCE_ALPHA   <- black tiles
	//GL_COMPRESSED_INTENSITY         <- black tiles
	//GL_COMPRESSED_RGB               <- black tiles
	//GL_COMPRESSED_RGBA              <- black tiles

	// With the following extension (GL_EXT_texture_compression_s3tc)
	//GL_COMPRESSED_RGB_S3TC_DXT1_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
	//GL_COMPRESSED_RGBA_S3TC_DXT5_EXT

	switch(format) {
		case GL_LUMINANCE:
		case GL_LUMINANCE8:
				return GL_COMPRESSED_LUMINANCE;
		case GL_RGB:
		case GL_RGB8:
				//return GL_COMPRESSED_RGB;
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case GL_RGBA:
		case GL_RGBA8:
				//return GL_COMPRESSED_RGBA;
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case GL_ALPHA:
		case GL_ALPHA8:
			return GL_COMPRESSED_ALPHA;
		default:
			return format;
	}
}

GLint toWrapModeGl(Texture::WrapMode wrapMode){
	switch(wrapMode){
	case Texture::wmClamp:
		return GL_CLAMP;
	case Texture::wmRepeat:
		return GL_REPEAT;
	case Texture::wmClampToEdge:
		return GL_CLAMP_TO_EDGE;
	default:
		assert(false);
		return GL_CLAMP;
	}
}

GLint toFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			std::cerr << "Components = " << (components) << std::endl;
			assert(false);
			return GL_RGBA;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE;
	case Texture::fAlpha:
		return GL_ALPHA;
	case Texture::fRgb:
		return GL_RGB;
	case Texture::fRgba:
		return GL_RGBA;
	default:
		assert(false);
		return GL_RGB;
	}
}

GLint toInternalFormatGl(Texture::Format format, int components){
	switch(format){
	case Texture::fAuto:
		switch(components){
		case 1:
			return GL_LUMINANCE8;
		case 3:
			return GL_RGB8;
		case 4:
			return GL_RGBA8;
		default:
			assert(false);
			return GL_RGBA8;
		}
		break;
	case Texture::fLuminance:
		return GL_LUMINANCE8;
	case Texture::fAlpha:
		return GL_ALPHA8;
	case Texture::fRgb:
		return GL_RGB8;
	case Texture::fRgba:
		return GL_RGBA8;
	default:
		assert(false);
		return GL_RGB8;
	}
}

// =====================================================
//	class Texture1DGl
// =====================================================

void Texture1DGl::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited) {

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_1D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")){
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int error= gluBuild1DMipmaps(
				GL_TEXTURE_1D, glCompressionFormat, pixmap.getW(),
				glFormat, GL_UNSIGNED_BYTE, pixels);

			if(error!=0){
				//throw runtime_error("Error building texture 1D mipmaps");
				char szBuf[1024]="";
				sprintf(szBuf,"Error building texture 1D mipmaps, returned: %d [%s] w = %d",error,pixmap.getPath().c_str(),pixmap.getW());
				throw runtime_error(szBuf);
			}
		}
		else{
			//build single texture
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage1D(
				GL_TEXTURE_1D, 0, glCompressionFormat, pixmap.getW(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error= glGetError();
			if(error!=GL_NO_ERROR){
				//throw runtime_error("Error creating texture 1D");
				char szBuf[1024]="";
				sprintf(szBuf,"Error creating texture 1D, returned: %d [%s] w = %d",error,pixmap.getPath().c_str(),pixmap.getW());
				throw runtime_error(szBuf);
			}
		}
		inited= true;
		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath());
	}

	assertGl();
}

void Texture1DGl::end(){
	if(inited){
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class Texture2DGl
// =====================================================

void Texture2DGl::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited) {

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

		//maxAnisotropy
		if(isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			//build mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int error= gluBuild2DMipmaps(
				GL_TEXTURE_2D, glCompressionFormat,
				pixmap.getW(), pixmap.getH(),
				glFormat, GL_UNSIGNED_BYTE, pixels);

			if(error!=0){
				//throw runtime_error("Error building texture 2D mipmaps");
				char szBuf[1024]="";
				sprintf(szBuf,"Error building texture 2D mipmaps, returned: %d [%s] w = %d, h = %d",error,(pixmap.getPath() != "" ? pixmap.getPath().c_str() : this->path.c_str()),pixmap.getW(),pixmap.getH());
				throw runtime_error(szBuf);
			}
		}
		else{
			//build single texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(
				GL_TEXTURE_2D, 0, glCompressionFormat,
				pixmap.getW(), pixmap.getH(),
				0, glFormat, GL_UNSIGNED_BYTE, pixels);
			GLint error= glGetError();

			//throw runtime_error("TEST!");

			if(error!=GL_NO_ERROR){
				char szBuf[1024]="";
				sprintf(szBuf,"Error creating texture 2D, returned: %d [%s] w = %d, h = %d, glInternalFormat = %d, glFormat = %d",error,pixmap.getPath().c_str(),pixmap.getW(),pixmap.getH(),glInternalFormat,glFormat);
				throw runtime_error(szBuf);
			}
		}
		inited= true;
		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath());
	}

	assertGl();
}

void Texture2DGl::end(){
	if(inited) {
		//printf("==> Deleting GL Texture [%s] handle = %d\n",getPath().c_str(),handle);
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class Texture3DGl
// =====================================================

void Texture3DGl::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//params
		GLint wrap= toWrapModeGl(wrapMode);
		GLint glFormat= toFormatGl(format, pixmap.getComponents());
		GLint glInternalFormat= toInternalFormatGl(format, pixmap.getComponents());
		GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);

		//pixel init var
		const uint8* pixels= pixmapInit? pixmap.getPixels(): NULL;

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_3D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);

		//build single texture
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage3D(
			GL_TEXTURE_3D, 0, glCompressionFormat,
			pixmap.getW(), pixmap.getH(), pixmap.getD(),
			0, glFormat, GL_UNSIGNED_BYTE, pixels);

		GLint error= glGetError();
		if(error!=GL_NO_ERROR){
			//throw runtime_error("Error creating texture 3D");
			char szBuf[1024]="";
			sprintf(szBuf,"Error creating texture 3D, returned: %d [%s] w = %d, h = %d, d = %d",error,pixmap.getPath().c_str(),pixmap.getW(),pixmap.getH(),pixmap.getD());
			throw runtime_error(szBuf);
		}
		inited= true;

		OutputTextureDebugInfo(format, pixmap.getComponents(),getPath());
	}

	assertGl();
}

void Texture3DGl::end(){
	if(inited){
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class TextureCubeGl
// =====================================================

void TextureCubeGl::init(Filter filter, int maxAnisotropy){
	assertGl();

	if(!inited){

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

		//wrap
		GLint wrap= toWrapModeGl(wrapMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);

		//filter
		if(mipmap){
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		for(int i=0; i<6; ++i){
			//params
			const Pixmap2D *currentPixmap= pixmap.getFace(i);

			GLint glFormat= toFormatGl(format, currentPixmap->getComponents());
			GLint glInternalFormat= toInternalFormatGl(format, currentPixmap->getComponents());
			GLint glCompressionFormat = toCompressionFormatGl(glInternalFormat);

			//pixel init var
			const uint8* pixels= pixmapInit? currentPixmap->getPixels(): NULL;
			GLenum target= GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			if(mipmap){
				int error= gluBuild2DMipmaps(
					target, glCompressionFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					glFormat, GL_UNSIGNED_BYTE, pixels);

				if(error!=0){
					//throw runtime_error("Error building texture cube mipmaps");
					char szBuf[1024]="";
					sprintf(szBuf,"Error building texture cube mipmaps, returned: %d [%s] w = %d, h = %d",error,currentPixmap->getPath().c_str(),currentPixmap->getW(),currentPixmap->getH());
					throw runtime_error(szBuf);
				}
			}
			else{
				glTexImage2D(
					target, 0, glCompressionFormat,
					currentPixmap->getW(), currentPixmap->getH(),
					0, glFormat, GL_UNSIGNED_BYTE, pixels);
			}

			int error = glGetError();
			if(error!=GL_NO_ERROR){
				//throw runtime_error("Error creating texture cube");
				char szBuf[1024]="";
				sprintf(szBuf,"Error creating texture cube, returned: %d [%s] w = %d, h = %d",error,currentPixmap->getPath().c_str(),currentPixmap->getW(),currentPixmap->getH());
				throw runtime_error(szBuf);
			}

			OutputTextureDebugInfo(format, currentPixmap->getComponents(),getPath());
		}
		inited= true;

	}

	assertGl();
}

void TextureCubeGl::end(){
	if(inited){
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

void TextureGl::OutputTextureDebugInfo(Texture::Format format, int components,const string path) {
	if(Texture::useTextureCompression == true) {
		GLint glFormat= toFormatGl(format, components);

		printf("**** Texture filename: [%s] format = %d components = %d, glFormat = %d\n",path.c_str(),format,components,glFormat);

		GLint compressed=0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &compressed);
		int error = glGetError();

		printf("**** Texture compressed status: %d, error [%d]\n",compressed,error);

		compressed=0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressed);
		error = glGetError();
		printf("**** Texture image size in video RAM: %d, error [%d]\n",compressed,error);

		compressed=0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &compressed);
		error = glGetError();
		printf("**** Texture image compression format used: %d, error [%d]\n",compressed,error);
	}
}

}}}//end namespace
