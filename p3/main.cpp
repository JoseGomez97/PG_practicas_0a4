
#include <glm/gtc/matrix_transform.hpp>

#include "PGUPV.h"
#include "mirror.h"

using namespace PGUPV;

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using std::vector;

class MyRender : public Renderer {
public:
  MyRender() :
    luz(0.2f, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)),
    teapotSpin(0.0f),
    showNormals(false),
    espejo(vec3(2.0f, 0.5f, -2.0f), glm::vec3(-1.0 / sqrt(2.0f), 0.0f, 1.0 / sqrt(2.0f)))
  {};
  void crearAgua(float height, float width);
  void crearTriangleStrip(std::vector<glm::vec3> vertices, const glm::vec4& color);
  void crearCilindro(float originY, float r_bottom, float r_top, float height, uint stacks,
      uint slices, const glm::vec4& color);
  void crearCaja(float size, float height, float width, float originY, const glm::vec4& color);
  void setup(void) override;
  void render(void) override;
  void reshape(uint w, uint h) override;
  void update(uint ms) override;
private:
  std::shared_ptr<GLMatrices> mats;
  Axes axes;
  Program litProgram;
  WireBox luz;	// Modelo de una caja para representar la fuente de luz
  std::shared_ptr<Scene> tetera;	// Modelo de una tetera
  float teapotSpin;	// Ángulo de rotación actual de la tetera
  bool showNormals;
  vec4 lightPos;		// Posición de la luz en coordenadas del mundo
  GLuint lightPosLoc;	// Localización del uniform que contiene la posición de la luz
  Mirror espejo;
  void dibujarAccesorios(void); // Se encarga de dibujar los ejes, la luz y las normales
  void dibujaTetera(void);
  Model fuente;
  Model agua;
};

void MyRender::crearAgua(float height, float width) {

    float w = width / 2;

    // Definición de los vértices
    std::vector<glm::vec3> vertices;

    vertices = {
        glm::vec3(-w, height, w), glm::vec3(w, height, w),
        glm::vec3(w, height, -w), glm::vec3(-w, height, -w)
    };

    auto mesh = std::make_shared<Mesh>();

    mesh->addVertices(vertices);
    mesh->addDrawCommand(new PGUPV::DrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size())));
    mesh->setColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    mesh->computeSmoothNormals();

    agua.addMesh(mesh);
}

void MyRender::crearTriangleStrip(std::vector<glm::vec3> vertices, const glm::vec4& color) {

    auto mesh = std::make_shared<Mesh>();

    mesh->addVertices(vertices);
    mesh->addDrawCommand(new PGUPV::DrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size())));
    mesh->setColor(color);
    mesh->computeSmoothNormals();

    fuente.addMesh(mesh);
}

void MyRender::crearCilindro(float originY, float r_bottom, float r_top, float height, uint stacks,
    uint slices, const glm::vec4& color) {

    if (r_bottom == 0.0 && r_top == 0.0)
        ERRT("Ambos radios no pueden ser cero.");
    if (r_bottom == 0.0 || r_top == 0.0)
        WARN("Para construir un cono, es preferible definir un radio muy pequeño, "
            "en vez de 0.0");

    auto m = std::make_shared<Mesh>();;
    fuente.addMesh(m);

    uint total_unique_vertices = (slices + 1) * (stacks + 1);
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<glm::vec2> tex_coord;

    float tilt_angle = atan2(height, r_bottom - r_top) - glm::radians(90.0f);
    glm::mat4 tilt_mat =
        glm::rotate(glm::mat4(1.0f), tilt_angle, vec3(1.0f, 0.0f, 0.0f));
    float delta = (float)(TWOPIf / slices);
    for (uint j = 0; j <= stacks; j++) {
        float angle = 0.0;
        float ring_y = height * ((float)j / stacks - 0.5f);
        float radius_ring = r_bottom - j * (r_bottom - r_top) / stacks;
        for (uint i = 0; i <= slices; i++, angle += delta) {
            float sin_a = sin(angle);
            float cos_a = cos(angle);

            vertices.push_back(
                vec3(radius_ring * sin_a, ring_y + originY, radius_ring * cos_a));
            normals.push_back(
                vec3(glm::rotate(glm::mat4(1.0f), angle, vec3(0.0f, 1.0f, 0.0f)) *
                    tilt_mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
            tex_coord.push_back(glm::vec2(angle / TWOPIf, (float)j / stacks));
        }
    }

    assert(vertices.size() == total_unique_vertices);

    // # of indices per stack:
    //  each stack has 2*slices triangles
    //  a triangle strip requires 2 + #triangles = 2 + 2*slices
    //  we end each slice with a -1 index to ask OpenGL to start a new
    //  tri-strip (+1)
    uint indices_per_stack = 2 + 2 * slices + 1;

    std::vector<GLuint> indices;

    for (uint j = 0; j < stacks; j++) {
        uint topring = (j + 1) * (slices + 1);
        uint botring = j * (slices + 1);
        for (uint i = 0; i <= slices; i++) {
            indices.push_back(topring++);
            indices.push_back(botring++);
        }
        indices.push_back((uint)-1);
    }

    assert(indices_per_stack * stacks == indices.size());

    m->addIndices(indices);
    m->setColor(color);
    m->addVertices(vertices);
    m->addNormals(normals);
    m->addTexCoord(0, tex_coord);

    auto dc = new DrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT, (void*)0);
    dc->setPrimitiveRestart();
    dc->setRestartIndex((uint)-1);

    m->addDrawCommand(dc);
}


void MyRender::crearCaja(float size, float height, float width, float originY, const glm::vec4& color) {
    // Definición de los vértices
    std::vector<glm::vec3> vertices;

    float s1 = size / 2;
    float s2 = s1 - width;
    float h1 = height + originY;
    float h2 = width + originY;

    vertices = {
        // Cara inferior externa
        glm::vec3(s1, originY, -s1), glm::vec3(s1, originY, s1), glm::vec3(-s1, originY, -s1),
        glm::vec3(-s1, originY, s1)
    };
    crearTriangleStrip(vertices, color);

    vertices = {
        // Laterales exteriores
        glm::vec3(-s1, h1, s1), glm::vec3(-s1, originY, s1),
        glm::vec3(s1, h1, s1), glm::vec3(s1, originY, s1),
        glm::vec3(s1, h1, -s1), glm::vec3(s1, originY, -s1),
        glm::vec3(-s1, h1, -s1), glm::vec3(-s1, originY, -s1),
        glm::vec3(-s1, h1, s1), glm::vec3(-s1, originY, s1)
    };
    crearTriangleStrip(vertices, color);

    vertices = {
        // Laterales interiores
        glm::vec3(-s2, height / 4 * 3 + originY, s2), glm::vec3(-s2, h1, s2),
        glm::vec3(s2, height / 4 * 3 + originY, s2), ::vec3(s2, h1, s2),
        glm::vec3(s2, height / 4 * 3 + originY, -s2), glm::vec3(s2, h1, -s2),
        glm::vec3(-s2, height / 4 * 3 + originY, -s2), glm::vec3(-s2, h1, -s2),
        glm::vec3(-s2, height / 4 * 3 + originY, s2), glm::vec3(-s2, h1, s2)
    };
    crearTriangleStrip(vertices, color);

    vertices = {
        // Techo paredes
        glm::vec3(-s2, h1, s2), glm::vec3(-s1, h1, s1),
        glm::vec3(s2, h1, s2), glm::vec3(s1, h1, s1),
        glm::vec3(s2, h1, -s2), glm::vec3(s1, h1, -s1),
        glm::vec3(-s2, h1, -s2), glm::vec3(-s1, h1, -s1),
        glm::vec3(-s2, h1, s2), glm::vec3(-s1, h1, s1)
    };
    crearTriangleStrip(vertices, color);

    crearAgua(height / 4 * 3 + originY, size - width * 2);
}


/* IMPLEMENTA ESTA FUNCIÓN SIGUIENDO LAS INSTRUCCIONES DE LAS TRANSPARENCIAS */
glm::mat4 rotationFromStoT(glm::vec3 s, glm::vec3 t) {
  glm::vec3 v = glm::cross(s, t);
  float e = glm::dot(s, t);
  float h = 1 / (1 + e);

  glm::mat4 res = {
    glm::vec4(e + h * std::pow(v[0],2), h * v.x * v.y + v.z,     h * v.x * v.z - v.y,      0),
    glm::vec4(h * v.x * v.y - v.z,      e + h * std::pow(v.y,2), h * v.y * v.z + v.x,      0),
    glm::vec4(h * v.x * v.z + v.y,      h * v.y * v.z - v.x,     e + h * std::pow(v.z, 2), 0),
    glm::vec4(0,                        0,                       0,                        1)
  };

  return res;
}

/* IMPLEMENTA ESTA FUNCIÓN SIGUIENDO LAS INSTRUCCIONES DE LAS TRANSPARENCIAS */
glm::mat4 computeReflectionMatrix(glm::vec3 p, glm::vec3 n) {
  glm::mat4 res = glm::mat4(1.0f);

  // Devolver el plano reflector a su sitio
  res = glm::translate(res, p);
  res *= rotationFromStoT(glm::vec3(0, 1, 0), n);

  // Reflejar con respecto al plano XZ
  res = glm::scale(res, glm::vec3(1, -1, 1));

  // Convertir el plano al XZ
  res *= rotationFromStoT(n, glm::vec3(0, 1, 0));
  res = glm::translate(res, -p);

  return res;
}

void MyRender::setup() {
  // Posición de la luz en la escena
  lightPos = glm::vec4(2.0f, 5.0f, 5.0f, 1.0f);

  glClearColor(.1f, .1f, .1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  // Cargando un modelo desde un fichero
  tetera = FileLoader::load("../recursos/modelos/teapot.3ds");

  // Asignamos a todas las mallas de la tetera un color
  tetera->processMeshes([](Mesh &m) { 
    m.setColor(glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f));
  });

  litProgram.addAttributeLocation(Mesh::VERTICES, "position");
  litProgram.addAttributeLocation(Mesh::NORMALS, "normal");
  litProgram.addAttributeLocation(Mesh::COLORS, "color");
  mats = GLMatrices::build();
  litProgram.connectUniformBlock(mats, UBO_GL_MATRICES_BINDING_INDEX);
  litProgram.loadFiles("../recursos/shaders/vertexColorLit");
  litProgram.compile();

  // Localización del uniform que contiene la posición de la luz	
  lightPosLoc = litProgram.getUniformLocation("lightpos");

  setCameraHandler(std::make_shared<OrbitCameraHandler>(5.0f));
  
  // Creamos la fuente
  crearCaja(1.8f, 0.5f, 0.1f, 0.0f, glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
  crearCaja(1.0f, 0.3f, 0.05f, 0.6f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
  crearCaja(0.6f, 0.2f, 0.02f, 1.1f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
  glm::vec4 colorCilindro = glm::vec4(0.5f, 0.5f, 0.3f, 1.0f);
  crearCilindro(0.75f, 0.1f, 0.1f, 1.5f, 20U, 20U, colorCilindro);
  crearCilindro(1.55f, 0.1f, 0.0f, 0.1f, 20U, 20U, colorCilindro);
}

void MyRender::dibujaTetera(void) {
  // Tetera que gira sobre sí misma
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->rotate(GLMatrices::MODEL_MATRIX, teapotSpin, vec3(0.0f, 1.0f, 0.0f));
  mats->translate(GLMatrices::MODEL_MATRIX, vec3(1.0f, 1.0f, 0.0f));
  mats->rotate(GLMatrices::MODEL_MATRIX, teapotSpin, vec3(1.0f, 1.0f, 0.0f));
  mats->scale(GLMatrices::MODEL_MATRIX, vec3(0.5f / tetera->maxDimension()));
  tetera->render();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);
}


/* Tienes que completar esta función para dibujar los reflejos de la escena, tanto
sobre el agua de tu fuente como sobre el espejo.

RECUERDA QUE PARA QUE SE VEA BIEN LA ILUMINACIÓN SOBRE TU FUENTE, DEBES CALCULAR LAS
NORMALES DE CADA MALLA, CON LA FUNCIÓN Mesh::computeSmoothNormals();
*/
void MyRender::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  mats->setMatrix(GLMatrices::VIEW_MATRIX, getCamera().getViewMatrix());
  litProgram.use();

  vec4 lp = getCamera().getViewMatrix() * lightPos;
  glUniform4fv(lightPosLoc, 1, &lp.x);

  glEnable(GL_STENCIL_TEST);

  // Creación cara transparente espejo y máscara
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 2, 0xFF);
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  espejo.render();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  // Creación fuente reflejada
  glStencilMask(0x00);
  glStencilFunc(GL_EQUAL, 2, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, espejo.reflectionMatrix());
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  fuente.render();
  glDisable(GL_CULL_FACE);
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  // Creación tetera reflejada
  glStencilMask(0x00);
  glStencilFunc(GL_EQUAL, 2, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, espejo.reflectionMatrix());
  dibujaTetera();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  // Creación agua reflejada
  glStencilMask(0x00);
  glStencilFunc(GL_EQUAL, 2, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, espejo.reflectionMatrix());
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  agua.render();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  // Creación máscara agua reflejada
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 3, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, espejo.reflectionMatrix());
  glColorMask(false, false, false, false);
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  agua.render();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glColorMask(true, true, true, true);
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  // Creación tetera reflejo agua reflejada
  glStencilMask(0x00);
  glStencilFunc(GL_EQUAL, 2, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, computeReflectionMatrix(glm::vec3(0, 0.5, 0), glm::vec3(0, 1, 0)));
  mats->multMatrix(GLMatrices::MODEL_MATRIX, espejo.reflectionMatrix());
  dibujaTetera();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);

  // Creación fuente
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  fuente.render();
  glDisable(GL_CULL_FACE);
  glEnable(GL_STENCIL_TEST);

  // Creación agua y máscara
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);
  glClear(GL_STENCIL_BUFFER_BIT);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  agua.render();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  // Creación tetera reflejo agua
  glStencilMask(0x00);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, computeReflectionMatrix(glm::vec3(0, 0.5, 0), glm::vec3(0, 1, 0)));
  dibujaTetera();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);
  glDisable(GL_STENCIL_TEST);

  // Creación tetera
  dibujaTetera();

  // Creación cara opaca espejo
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  espejo.render();
  glDisable(GL_CULL_FACE);

  //   
  //// NO MODIFICAR A PARTIR DE AQUÍ...
  dibujarAccesorios();
  CHECK_GL();
}

void MyRender::dibujarAccesorios(void) {
  // Instalar el shader para dibujar los ejes, las normales y la luz
  ConstantIllumProgram::use();
  // Dibujamos los ejes de coordenadas
  axes.render();
  // Cuando el usuario pulsa la N, mostramos/ocultamos las normales
  if (App::isKeyUp(KeyCode::N))
    showNormals = !showNormals;
  if (showNormals) {
    // Dibujamos las normales de los planos 
    espejo.renderNormal();
  }
  // Dibujamos la posición de la luz
  mats->pushMatrix(GLMatrices::MODEL_MATRIX);
  mats->setMatrix(GLMatrices::MODEL_MATRIX, glm::translate(glm::mat4(1.0f), vec3(lightPos)));
  luz.render();
  mats->popMatrix(GLMatrices::MODEL_MATRIX);
}

void MyRender::reshape(uint w, uint h) {
  glViewport(0, 0, w, h);
  mats->setMatrix(GLMatrices::PROJ_MATRIX, getCamera().getProjMatrix());
}

// Radianes por segundo a los que gira la tetera
#define SPINSPEED glm::radians(90.0f)

void MyRender::update(uint ms) {
  teapotSpin += SPINSPEED*ms / 1000.0f;
  if (teapotSpin > TWOPIf) teapotSpin -= TWOPIf;

}

int main(int argc, char *argv[]) {
  App &myApp = App::getInstance();
  myApp.setInitWindowSize(800, 600);
  myApp.initApp(argc, argv, PGUPV::DOUBLE_BUFFER | PGUPV::DEPTH_BUFFER | PGUPV::MULTISAMPLE | PGUPV::STENCIL_BUFFER);
  myApp.getWindow().setRenderer(std::make_shared<MyRender>());
  return myApp.run();
}
