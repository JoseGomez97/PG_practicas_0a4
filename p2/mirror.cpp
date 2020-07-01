#include <glm/gtc/matrix_transform.hpp>

#include "mirror.h"

using glm::vec4;
using glm::vec3;
using glm::mat3;
using glm::mat4;

using PGUPV::Arrow;
using PGUPV::UBOMaterial;
using PGUPV::GLMatrices;


glm::mat4 computeReflectionMatrix(glm::vec3 p, glm::vec3 n);
glm::mat4 rotationFromStoT(glm::vec3 s, glm::vec3 t);

Mirror::Mirror(glm::vec3 center, glm::vec3 normal) :
  Rect(2.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)) {
  _modelMatrix = glm::translate(glm::mat4(1.0f), center);
  float inv1z = 1.0f / (1.0f + normal.z);
  _modelMatrix *= glm::mat4(
      normal.z + (normal.y*normal.y)*inv1z, -normal.y*normal.x * inv1z, -normal.x, 0.f,
      -normal.y*normal.x * inv1z, normal.z + (normal.x * normal.x) * inv1z, -normal.y, 0.f,
      normal.x, normal.y, normal.z, 0.f,
      .0f, .0f, .0f, 1.0f);

  _reflectionMatrix = computeReflectionMatrix(center, normal);
}

// Dibuja el espejo
void Mirror::render() {
  auto gl = std::static_pointer_cast<GLMatrices>(PGUPV::gl_uniform_buffer.getBound(UBO_GL_MATRICES_BINDING_INDEX));
  gl->pushMatrix(GLMatrices::MODEL_MATRIX);
  gl->setMatrix(GLMatrices::MODEL_MATRIX, getModelMatrix());
  Rect::render();
  gl->popMatrix(GLMatrices::MODEL_MATRIX);
}


// Dibuja la normal del plano
void Mirror::renderNormal() {
  if (!_normal) {
    _normal = Arrow::build(1.0f, .02f, .05f, .25f, vec4(1.0f, 0.0f, 0.0f, 1.0f),
        vec4(0.0f, 1.0f, 0.0f, 1.0f));
  }
  auto gl = std::static_pointer_cast<GLMatrices>(PGUPV::gl_uniform_buffer.getBound(UBO_GL_MATRICES_BINDING_INDEX));
  gl->pushMatrix(GLMatrices::MODEL_MATRIX);
  gl->setMatrix(GLMatrices::MODEL_MATRIX, getModelMatrix());
  _normal->render();
  gl->popMatrix(GLMatrices::MODEL_MATRIX);
}

