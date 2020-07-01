
#include <glm/gtc/matrix_transform.hpp>

#include "PGUPV.h"

using namespace PGUPV;

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using std::vector;


/*
Ejemplo de dibujo fuera de pantalla usando FBO
*/

// Tamaño del FBO
#define FBO_SIZE 512

class MyRender : public Renderer {
public:
  MyRender() :
    teapotSpin(0.0f),
    fbowidth(FBO_SIZE),
    fboheight(FBO_SIZE)
  {};
  void setup(void) override;
  void render(void) override;
  void reshape(uint w, uint h) override;
  void update(uint ms) override;
private:
  std::shared_ptr<GLMatrices> mats;
  GLuint fbo;
  GLuint rbos[2];
  Program ashader;
  std::shared_ptr<Scene> tetera;	// Modelo de una tetera
  float teapotSpin;	// Ángulo de rotación actual de la tetera
  vec4 lightPos;		// Posición de la luz en coordenadas del mundo
  GLuint lightPosLoc;	// Localización del uniform que contiene la posición de la luz
  int width, height;	// Tamaño de la ventana del S.O. en pixeles
  int fbowidth, fboheight;  // Tamaño de los buffers del FBO
};


void MyRender::setup() {
  // Posición de la luz en la escena
  lightPos = glm::vec4(2.0f, 5.0f, 5.0f, 1.0f);

  glClearColor(.1f, .1f, .1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  // Cargando un modelo desde un fichero
  tetera = FileLoader::load("../recursos/modelos/teapot.3ds");
  auto laton = PGUPV::getMaterial(PredefinedMaterial::BRASS);
  tetera->processMeshes([laton](Mesh &m) { m.setMaterial(laton); });

  mats = GLMatrices::build();

  ashader.addAttributeLocation(Mesh::VERTICES, "position");
  ashader.addAttributeLocation(Mesh::NORMALS, "normal");
  ashader.connectUniformBlock(mats, UBO_GL_MATRICES_BINDING_INDEX);
  ashader.replaceString("$" + UBOMaterial::blockName, UBOMaterial::definition);
  ashader.loadFiles("../recursos/shaders/ambdiff");
  ashader.compile();
  // Localización del uniform que contiene la posición de la luz	
  lightPosLoc = ashader.getUniformLocation("lightpos");

  // Mostrar por consola información relacionada con los FBO en este sistema
  // Recuerda que pulsando Ctrl+I en cualquier aplicación PGUPV mostrará por consola
  // un montón de información sobre tu sistema
  GLint i;
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &i);
  std::cout << "GL_MAX_RENDERBUFFER_SIZE: " << i << std::endl;
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &i);
  std::cout << "GL_MAX_COLOR_ATTACHMENTS: " << i << std::endl;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &i);
  std::cout << "GL_MAX_DRAW_BUFFERS: " << i << std::endl;

  // Generamos un FBO
  glGenFramebuffers(1, &fbo);
  // Lo vinculamos para dobujo
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

  // Generamos dos renderbuffers
  glGenRenderbuffers(2, rbos);
  // Vinculamos el primero
  glBindRenderbuffer(GL_RENDERBUFFER, rbos[0]);
  // Especificamos su tamaño y su formato (colores RGBA)
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, fbowidth, fboheight);

  // Vinculamos el segundo RBO
  glBindRenderbuffer(GL_RENDERBUFFER, rbos[1]);
  // Especificamos su tamaño y formato (profundidad)
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbowidth, fboheight);

  // Vinculamos los RBO al FBO
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    GL_RENDERBUFFER, rbos[0]);
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
    GL_RENDERBUFFER, rbos[1]);

  // Comprobamos si el FBO está completo, y escribimos por pantalla el resultado
  std::string message;
  assert(PGUPV::checkFBOCompleteness(GL_DRAW_FRAMEBUFFER, message));
  std::cout << message << std::endl;

  // Como el tamaño del FBO no cambia, el volumen de la cámara tampoco: lo establecemos aquí
  float ar = (float)fbowidth / fboheight;
  mats->setMatrix(GLMatrices::PROJ_MATRIX, glm::perspective(glm::radians(60.0f), ar, .1f, 100.0f));
}


void MyRender::render() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glViewport(0, 0, fbowidth, fboheight);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  mats->setMatrix(GLMatrices::VIEW_MATRIX, glm::lookAt(vec3(0.0f, 0.0f, 1.0f),
    vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));
  ashader.use();

  // Posición de la luz en coordenadas de la cámara
  vec4 lp = mats->getMatrix(GLMatrices::VIEW_MATRIX) * lightPos;
  glUniform4f(lightPosLoc, lp.x, lp.y, lp.z, lp.w);

  // Tetera que gira sobre sí misma
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->rotate(GLMatrices::MODEL_MATRIX, teapotSpin, vec3(1.0f, 1.0f, 0.0f));
  mats->scale(GLMatrices::MODEL_MATRIX, vec3(1.0f / tetera->maxDimension()));
  tetera->render();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  glReadBuffer(GL_COLOR_ATTACHMENT0);


  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK_LEFT);
  glViewport(0, 0, width, height);

  glBlitFramebuffer(0, 0, fbowidth, fboheight, 0, height / 2, width / 2, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBlitFramebuffer(fbowidth, fboheight, 0, 0, width / 2, 0, width, height / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {
      glBlitFramebuffer(0, 0, fbowidth, fboheight, i*width / 4, j*height / 4, (i + 1)*width / 4, (j + 1)*height / 4, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBlitFramebuffer(fbowidth, fboheight, 0, 0, i*width / 4 + width / 2, j*height / 4 + height / 2, (i + 1)*width / 4 + width / 2, (j + 1)*height / 4 + height / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  CHECK_GL();
}

void MyRender::reshape(uint w, uint h) {
  width = w;
  height = h;
}

// Radianes por segundo a los que gira la tetera
#define SPINSPEED glm::radians(180.0f)

void MyRender::update(uint ms) {
  teapotSpin += SPINSPEED*ms / 1000.0f;
  if (teapotSpin > TWOPIf) teapotSpin -= TWOPIf;
}

int main(int argc, char *argv[]) {
  App &myApp = App::getInstance();
  myApp.initApp(argc, argv, PGUPV::DOUBLE_BUFFER | PGUPV::DEPTH_BUFFER);
  myApp.getWindow().setRenderer(std::make_shared<MyRender>());
  return myApp.run();
}
