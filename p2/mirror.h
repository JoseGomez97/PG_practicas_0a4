#ifndef _MIRROR_H
#define _MIRROR_H 2013

#include <glm/glm.hpp>
#include "PGUPV.h"

class Mirror : public PGUPV::Rect {
public:
	Mirror(glm::vec3 center, glm::vec3 normal);
    // Devuelve la matriz de reflexión con respecto al espejo
    glm::mat4 reflectionMatrix() const { return _reflectionMatrix; };
    // Devuelve el reflejo del punto p con respecto al espejo
    glm::vec4 reflect(glm::vec4 p) const { return _reflectionMatrix * p; };
    // Devuelve la matriz del modelo del espejo
    glm::mat4 getModelMatrix() const { return _modelMatrix; };
    // Dibuja el espejo
    void render() override;
    // Dibuja la normal del plano
	void renderNormal();
    
private:
    glm::mat4 _reflectionMatrix;
    glm::mat4 _modelMatrix;
	std::shared_ptr<PGUPV::Arrow> _normal;
};

#endif
