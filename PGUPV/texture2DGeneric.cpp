
#include "app.h"
#include "texture2DGeneric.h"
#include "utils.h"
#include "log.h"
#include "image.h"

using PGUPV::Texture2DGeneric;
using PGUPV::Image;

Texture2DGeneric::Texture2DGeneric(GLenum texture_type, GLenum minfilter,
	GLenum magfilter, GLenum wrap_s,
	GLenum wrap_t)
	: Texture(texture_type, minfilter, magfilter, wrap_s, wrap_t), _width(0),
	_height(0) {
	if (texture_type != GL_TEXTURE_2D && texture_type != GL_PROXY_TEXTURE_2D &&
		texture_type != GL_TEXTURE_1D_ARRAY &&
		texture_type != GL_PROXY_TEXTURE_1D_ARRAY &&
		texture_type != GL_TEXTURE_RECTANGLE &&
		texture_type != GL_PROXY_TEXTURE_RECTANGLE &&
		texture_type != GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
		texture_type != GL_TEXTURE_CUBE_MAP_NEGATIVE_X &&
		texture_type != GL_TEXTURE_CUBE_MAP_POSITIVE_Y &&
		texture_type != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y &&
		texture_type != GL_TEXTURE_CUBE_MAP_POSITIVE_Z &&
		texture_type != GL_TEXTURE_CUBE_MAP_NEGATIVE_Z &&
		texture_type != GL_PROXY_TEXTURE_CUBE_MAP)
		ERRT("Esta clase no soporta el tipo de textura pedida: " +
			PGUPV::hexString(texture_type));
};

// Allocates memory for a texture with the given size and format
void Texture2DGeneric::allocate(uint width, uint height, GLint internalformat) {
	glBindTexture(_texture_type, _texId);
	setParams();
	if (internalformat == GL_RED || internalformat == GL_RG || internalformat == GL_RGB
		|| internalformat == GL_RGBA)
		glTexImage2D(_texture_type, 0, internalformat, width, height, 0, internalformat,
			GL_UNSIGNED_BYTE, NULL);
	else if (internalformat == GL_DEPTH_COMPONENT ||
		internalformat == GL_DEPTH_COMPONENT16 ||
		internalformat == GL_DEPTH_COMPONENT24 ||
		internalformat == GL_DEPTH_COMPONENT32)
		glTexImage2D(_texture_type, 0, internalformat, width, height, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	else if (internalformat == GL_DEPTH_STENCIL)
		glTexImage2D(_texture_type, 0, GL_DEPTH_STENCIL, width, height, 0,
			GL_DEPTH_STENCIL /* no debería importar */,
			GL_UNSIGNED_INT_24_8 /* no debería importar */, NULL);
	else if (internalformat == GL_R32UI)
		glTexImage2D(_texture_type, 0, internalformat, width, height, 0,
			GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	else
		glTexImage2D(_texture_type, 0, internalformat, width, height, 0,
			GL_RGBA /* no debería importar */,
			GL_BYTE /* no debería importar */, NULL);
	CHECK_GL2("Error reservando memoria para la textura");

	_width = width;
	_height = height;
	_internalFormat = internalformat;
	_ready = true;
}

void Texture2DGeneric::setParams() {
	glTexParameteri(_texture_type, GL_TEXTURE_MAG_FILTER, _magfilter);
	glTexParameteri(_texture_type, GL_TEXTURE_MIN_FILTER, _minfilter);
	if (_texture_type != GL_TEXTURE_RECTANGLE) {
		glTexParameteri(_texture_type, GL_TEXTURE_WRAP_S, _wrap_s);
		glTexParameteri(_texture_type, GL_TEXTURE_WRAP_T, _wrap_t);
	}
}

void Texture2DGeneric::loadImageFromMemory(void *pixels, uint width,
	uint height, GLenum pixels_format, GLenum pixels_type,
	GLint internalformat) {
	_ready = false;
	/* Create and load textures to OpenGL */
	glBindTexture(_texture_type, this->_texId);
	setParams();

	glTexImage2D(_texture_type, 0, internalformat, width, height, 0,
		pixels_format, pixels_type, pixels);
	CHECK_GL();
	_width = width;
	_height = height;
	_internalFormat = internalformat;
	_ready = true;
}

void Texture2DGeneric::updateImageFromMemory(void *pixels, uint width, uint height, GLenum pixels_format,
	GLenum pixels_type) {

	glBindTexture(_texture_type, _texId);
	glTexSubImage2D(_texture_type, 0, 0, 0, width, height, pixels_format, pixels_type, pixels);

}


bool Texture2DGeneric::loadImage(const Image &image, GLenum internalFormat) {
	loadImageFromMemory(image.getPixels(), image.getWidth(), image.getHeight(),
		image.getGLFormatType(), image.getGLPixelBaseType(), internalFormat);
	return _ready;
}


bool Texture2DGeneric::loadImage(const Image &image) {
	return loadImage(image, image.getSuggestedGLInternalFormatType());
}

void Texture2DGeneric::updateImage(const Image &image) {
	updateImageFromMemory(image.getPixels(), image.getWidth(), image.getHeight(), image.getGLFormatType(), image.getGLPixelBaseType());
}


bool Texture2DGeneric::loadImage(const std::string &filename, GLenum internalFormat) {
	PGUPV::Image image(filename);
	_name = PGUPV::getFilenameFromPath(filename);
	return loadImage(image, internalFormat);
}

void saveBoundTexture(const std::string &filename, uint32_t width, uint32_t height, uint32_t bpp) {
	uint8_t *bytes = new uint8_t[width * height * bpp / 8];

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	switch (bpp) {
	case 8:
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);
		break;
	case 24:
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, bytes);
		break;
	case 32:
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
		break;
	}

	PGUPV::Image image(width, height, bpp, bytes);
	image.save(filename);
	delete[]bytes;
}

void Texture2DGeneric::save(const std::string &filename, GLuint texId, unsigned int bpp) {
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		ERRT("Sólo se puede guardar imágenes de 8, 24 y 32 bpp");
	}
	glActiveTexture(GL_TEXTURE0 + App::getScratchUnitTextureNumber());
	glBindTexture(GL_TEXTURE_2D, texId);
	GLint width, height;

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	saveBoundTexture(filename, width, height, bpp);
}

void Texture2DGeneric::save(const std::string &filename, unsigned int bpp) {
  if (bpp != 8 && bpp != 24 && bpp != 32) {
    ERRT("Sólo se puede guardar imágenes de 8, 24 y 32 bpp");
  }
  bind(GL_TEXTURE0 + App::getScratchUnitTextureNumber());
  saveBoundTexture(filename, _width, _height, bpp);
}
