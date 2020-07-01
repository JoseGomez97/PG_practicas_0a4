
#include <glm/gtc/matrix_transform.hpp>

#include "PGUPV.h"

using namespace PGUPV;

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using std::vector;

#define FBOSIZE 512

/*
Este ejemplo es igual que el ej3-2, pero usando la clase FBO de PGUPV
*/

class MyRender : public Renderer {
public:
  MyRender() : teapotSpin(0.0f), lightPosLoc(0) {}
  void setup(void) override;
  void render(void) override;
  void reshape(uint w, uint h) override;
  void update(uint ms) override;

private:
  std::shared_ptr<GLMatrices> mats;
  FBO fbo;
  Program ashader;
  std::shared_ptr<Scene> tetera;   // Modelo de una tetera
  float teapotSpin;   // Ángulo de rotación actual de la tetera
  vec4 lightPos;      // Posición de la luz en coordenadas del mundo
  GLuint lightPosLoc; // Localización del uniform que contiene la posición de la
  // luz
  int width, height;  // Tamaño de la ventana del S.O. en pixeles
};

void MyRender::setup() {
  // Posición de la luz en la escena
  lightPos = glm::vec4(2.0f, 5.0f, 5.0f, 1.0f);

  glClearColor(.1f, .1f, .1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  teapotSpin = 0.0f;

  // Cargando un modelo desde un fichero
  tetera = FileLoader::load("../recursos/modelos/teapot.3ds");
  auto brass = PGUPV::getMaterial(PredefinedMaterial::BRASS);
  tetera->processMeshes([brass](Mesh &m) { m.setMaterial(brass); });

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
  GLint i;
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &i);
  std::cout << "GL_MAX_RENDERBUFFER_SIZE: " << i << std::endl;
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &i);
  std::cout << "GL_MAX_COLOR_ATTACHMENTS: " << i << std::endl;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &i);
  std::cout << "GL_MAX_DRAW_BUFFERS: " << i << std::endl;

  // Vinculamos el objeto FBO a GL_DRAW_FRAMEBUFFER
  fbo.bind(GL_DRAW_FRAMEBUFFER);
  // Asociamos un nuevo renderbuffer de tipo color, del tamaño especificado
  fbo.createAndAttach(GL_COLOR_ATTACHMENT0, FBOSIZE, FBOSIZE);
  // Asociamos un nuevo renderbuffer de tipo depth, del tamaño especificado
  fbo.createAndAttach(GL_DEPTH_ATTACHMENT, FBOSIZE, FBOSIZE);

  // Nos aseguramos que el FBO está completo
  if (!fbo.isComplete()) {
    ERRT("Error al configurar el FBO");
  }

  // Como el tamaño del FBO no cambia, el volumen de la cámara tampoco: lo
  // establecemos aquí
  float ar = (float)FBOSIZE / FBOSIZE;
  mats->setMatrix(GLMatrices::PROJ_MATRIX,
    glm::perspective(glm::radians(60.0f), ar, .1f, 100.0f));
}

void MyRender::render() {
  // Dibujar al FBO
  fbo.bind(GL_DRAW_FRAMEBUFFER);
  glViewport(0, 0, FBOSIZE, FBOSIZE);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  mats->setMatrix(GLMatrices::VIEW_MATRIX,
    glm::lookAt(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f),
      vec3(0.0f, 1.0f, 0.0f)));
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

  // Ya hemos dibujado la escena al FBO. Ahora leeremos de él...
  fbo.bind(GL_READ_FRAMEBUFFER);
  // y reestablecemos el FBO por defecto (el asociado con la ventana) para
  // escritura
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  glViewport(0, 0, width, height);

  // Ahora dibujar la escena almacenada en el FBO en distintas posiciones de la
  // ventana
  glBlitFramebuffer(0, 0, FBOSIZE, FBOSIZE, 0, height / 2, width / 2, height,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBlitFramebuffer(FBOSIZE, FBOSIZE, 0, 0, width / 2, 0, width, height / 2,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {
      glBlitFramebuffer(0, 0, FBOSIZE, FBOSIZE, i * width / 4, j * height / 4,
        (i + 1) * width / 4, (j + 1) * height / 4,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
      glBlitFramebuffer(
        FBOSIZE, FBOSIZE, 0, 0, i * width / 4 + width / 2,
        j * height / 4 + height / 2, (i + 1) * width / 4 + width / 2,
        (j + 1) * height / 4 + height / 2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

  fbo.unbind();

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
