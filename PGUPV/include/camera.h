
#pragma once


#include <glm/glm.hpp>
#include <memory>

namespace PGUPV {
	class Model;

	class Camera {
	public:
		Camera() : viewMatrix(glm::mat4(1.0f)), projMatrix(glm::mat4(1.0f)) {};
		/**
		Devuelve la matriz de la vista actual
		*/
		glm::mat4 getViewMatrix() const { return viewMatrix; };
		/**
		Devuelve la matriz de proyección.
		*/
		glm::mat4 getProjMatrix() const { return projMatrix; };
		/**
		Devuelve la dirección de la vista actual en el S.C. del mundo
		*/
		glm::vec3 getViewDirection() const;
		/**
		Devuelve el vector up actual en el S.C. del mundo
		*/
		glm::vec3 getUpDirection() const;
		/**
		Devuelve la dirección del eje X local a la cámara (right) en el S.C. del mundo
		*/
		glm::vec3 getRightDirection() const;
		/**
		Devuelve la posición actual de la cámara en el S.C. del mundo
		*/
		glm::vec3 getCameraPos() const;
		/**
		Devuelve el FOV de la cámara, en radianes
		*/
		float getFOV() const;
		/**
		Devuelve el aspect ratio de la cámara
		*/
		float getAspectRatio() const;
		/**
		Devuelve la distancia near de la cámara
		*/
		float getNear() const;
		/**
		Devuelve la distancia far de la cámara
		*/
		float getFar() const;
		/**
		Establece una nueva matriz de la vista
		*/
		void setViewMatrix(const glm::mat4 &viewMatrix);
		/**
		Establece una nueva matriz de proyección
		*/
		void setProjMatrix(const glm::mat4 &projMatrix);
		/**
		Dibuja el volumen de la cámara
		*/
		void render() const;
	protected:
		glm::mat4 viewMatrix, projMatrix;
		mutable std::shared_ptr<Model> frustum;
	};

};
