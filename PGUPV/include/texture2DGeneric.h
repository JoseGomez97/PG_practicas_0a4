
#ifndef _TEXTURE2DGENERIC_H
#define _TEXTURE2DGENERIC_H 2014

#include "texture.h"
#include "common.h"

namespace PGUPV {

  class Image;

  /*
  \class Texture2DGeneric
  Esta clase representa un objeto textura 2D. Permite cargar imágenes desde
  fichero y establecer los parámetros para su uso.
  */
  class Texture2DGeneric : public Texture {
  public:
    /**
     Constructor por defecto con unos parámetros iniciales (NO corresponden con los
     valores por defecto de OpenGL)
     */
    Texture2DGeneric(GLenum texture_type, GLenum minfilter = GL_LINEAR,
      GLenum magfilter = GL_LINEAR, GLenum wrap_s = GL_REPEAT,
      GLenum wrap_t = GL_REPEAT);
    virtual ~Texture2DGeneric(){};
    /**
     Función para cargar en el objeto textura una imagen desde fichero:

     \param filename nombre y ruta (absoluta o relativa) del fichero a cargar
	 \param internalFormat (opcional) establece el formato de los píxeles (GL_RGB, GL_RGBA...)
     \return true en caso de haber podido cargar la imagen.
     */
    virtual bool loadImage(const std::string &filename, GLenum internalFormat = GL_RGB);

    /**
     Función para cargar el objeto Image al objeto textura.
     \param image Imagen a cargar
     \return true si la carga ha tenido éxito
     */
    virtual bool loadImage(const Image &image);
	void updateImage(const Image &image);

	/**
	Función para cargar el objeto Image al objeto textura.
	\param image Imagen a cargar
	\param internalFormat (opcional) establece el formato de los píxeles (GL_RGB, GL_RGBA...)
	\return true si la carga ha tenido éxito
	*/
	virtual bool loadImage(const Image &image, GLenum internalFormat);

    /**
     Carga al objeto textura desde la zona de memoria indicada una imagen del
     tamaño indicado.

     \param pixels Puntero al buffer que contiene la imagen
     \param width Ancho de la imagen, en píxeles
     \param height Alto de la imagen, en píxeles
     \param pixels_format Contenido del buffer (GL_RGBA, GL_RED,
     GL_DEPTH_STENCIL...) Ver Table 8.3 de la especificación de OpenGL 4.4
     \param pixels_type Tipo de datos de los elementos del buffer
     (GL_UNSIGNED_BYTE, GL_FLOAT...) Ver Table 8.2 de la especificación de OpenGL
     4.4
     \param internalformat Formato interno de la textura (GL_RGBA, GL_DEPTH_COMPONENT,
     GL_DEPTH_STENCIL)
     */
    void loadImageFromMemory(void *pixels, uint width, uint height,
      GLenum pixels_format,
      GLenum pixels_type,
      GLint internalformat);

	void updateImageFromMemory(void *pixels, uint width, uint height, GLenum pixels_format,
		GLenum pixels_type);
    /**
     Reserva memoria asociada a la textura para almacenar una imagen del tamaño y
     formato especificado.
     \param width Ancho de la imagen, en píxeles
     \param height Alto de la imagen, en píxeles
     \param internalformat Formato interno de la image (GL_RGBA,
     GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL)
     */
    void allocate(uint width, uint height, GLint internalformat);

	/**
	Guarda el contenido de la textura en el fichero indicado. Se puede almacenar en los 
	formatos habituales.
	*/
	void save(const std::string &filename, unsigned int bpp = 24);

    uint getWidth() { return _width; };
    uint getHeight() { return _height; };

	static void save(const std::string &filename, GLuint texId, unsigned int bpp);
  protected:
    void setParams();
    uint _width, _height;
  };

};

#endif
