#include <memory>                             // for shared_ptr, make_shared, static_pointer_cast

#include "camera.h"
#include "model.h"
#include "indexedBindingPoint.h"
#include "stockPrograms.h"
#include "drawCommand.h"                      // for DrawElements

using glm::mat4;
using glm::vec3;

using PGUPV::Camera;
using PGUPV::GLMatrices;
using PGUPV::Mesh;


glm::vec3 Camera::getUpDirection() const {
	return glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
}

glm::vec3 Camera::getViewDirection() const {
	return glm::vec3(-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]);
}

glm::vec3 Camera::getRightDirection() const {
	return glm::cross(getViewDirection(), getUpDirection());
}

// From: https://www.opengl.org/discussion_boards/showthread.php/178484-Extracting-camera-position-from-a-ModelView-Matrix
glm::vec3 Camera::getCameraPos() const {
	// Get the 3 basis vector planes at the camera origin and transform them into model space.
	//  
	// NOTE: Planes have to be transformed by the inverse transpose of a matrix
	//       Nice reference here: http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
	//
	//       So for a transform to model space we need to do:
	//            inverse(transpose(inverse(MV)))
	//       This equals : transpose(MV) - see Lemma 5 in http://mathrefresher.blogspot.com.au/2007/06/transpose-of-matrix.html
	//
	// As each plane is simply (1,0,0,0), (0,1,0,0), (0,0,1,0) we can pull the data directly from the transpose matrix.
	//  
	mat4 modelViewT = glm::transpose(viewMatrix);

	// Get plane normals 
	vec3 n1(modelViewT[0]);
	vec3 n2(modelViewT[1]);
	vec3 n3(modelViewT[2]);

	// Get plane distances
	float d1(modelViewT[0].w);
	float d2(modelViewT[1].w);
	float d3(modelViewT[2].w);

	// Get the intersection of these 3 planes 
	// (uisng math from RealTime Collision Detection by Christer Ericson)
	vec3 n2n3 = glm::cross(n2, n3);
	float denom = glm::dot(n1, n2n3);

	vec3 top = (n2n3 * d1) + glm::cross(n1, (d3*n2) - (d2*n3));
	return top / -denom;
}

float Camera::getFOV() const {
	float fov = 2.f * atanf(1.0f / projMatrix[1][1]);
	return fov;
}

float Camera::getAspectRatio() const {
	return projMatrix[1][1] / projMatrix[0][0];
}

float Camera::getNear() const {
	float near = (2.0f*projMatrix[3][2]) / (2.0f*projMatrix[2][2] - 2.0f);
	return near;
}

float Camera::getFar() const {
	float far = ((projMatrix[2][2] - 1.0f)*getNear()) / (projMatrix[2][2] + 1.0f);
	return far;
}

void Camera::setViewMatrix(const glm::mat4 &viewM) {
	this->viewMatrix = viewM;
}

void Camera::setProjMatrix(const glm::mat4 &projM) {
	this->projMatrix = projM;
	frustum.reset();
}

void Camera::render() const {
	if (!frustum) {
		glm::mat4 ip = glm::inverse(projMatrix);

		glm::vec4 trnear = ip * glm::vec4(1.0f, 1.0f, -1.0, 1.0);
		glm::vec4 blnear = ip * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0);

		glm::vec4 trfar = ip * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 blfar = ip * glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);

		trnear = trnear / trnear.w;
		blnear = blnear / blnear.w;

		trfar = trfar / trfar.w;
		blfar = blfar / blfar.w;

		auto mesh = std::make_shared<Mesh>();

		std::vector<glm::vec4> vertices{
			blnear,
			glm::vec4(trnear.x, blnear.y, blnear.z, 1.0),
			trnear,
			glm::vec4(blnear.x, trnear.y, trnear.z, 1.0),

			blfar,
			glm::vec4(trfar.x, blfar.y, blfar.z, 1.0),
			trfar,
			glm::vec4(blfar.x, trfar.y, trfar.z, 1.0)
		};
		mesh->addVertices(vertices);

		std::vector<ushort> indices{
			0, 4, 5, 1, 0, 3, 7, 4, 0xFFFF,
			6, 2, 3, 7, 6, 5, 1, 2
		};

		mesh->addIndices(indices);
		mesh->setColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
		auto de = new PGUPV::DrawElements(GL_LINE_STRIP, 17, GL_UNSIGNED_SHORT, nullptr);
		de->setRestartIndex(0xFFFF);
		de->setPrimitiveRestart();

		mesh->addDrawCommand(de);

		frustum = std::make_shared<Model>();
		frustum->addMesh(mesh);

	}

	auto bo = gl_uniform_buffer.getBound(UBO_GL_MATRICES_BINDING_INDEX);
	auto mats = std::static_pointer_cast<GLMatrices>(bo);
	if (mats) {
		mats->pushMatrix(PGUPV::GLMatrices::MODEL_MATRIX);
		mats->loadIdentity(PGUPV::GLMatrices::MODEL_MATRIX);

		mats->translate(PGUPV::GLMatrices::MODEL_MATRIX, getCameraPos());

		mat4 rot = glm::mat4(
			glm::vec4(getRightDirection(), 0.0f),
			glm::vec4(getUpDirection(), 0.0f),
			glm::vec4(-getViewDirection(), 0.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		mats->multMatrix(PGUPV::GLMatrices::MODEL_MATRIX, rot);
	}

	auto prev = PGUPV::ConstantIllumProgram::use();
	frustum->render();
	if (prev) prev->use();

	if (mats)
		mats->popMatrix(PGUPV::GLMatrices::MODEL_MATRIX);
}
