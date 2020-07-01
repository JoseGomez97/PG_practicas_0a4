#include "PGUPV.h"

using namespace PGUPV;

/*
Esta es la aplicaci�n m�nima que se puede ejecutar con PGUPV.
Simplemente borra la ventana a un color determinado.
*/

class MyRender : public Renderer {
public:
	void setup(void);
	void render(void);
	void reshape(uint w, uint h);
private:
	/* Un programa es un conjunto de shaders que se pueden instalar en la GPU para calcular el
	aspecto final en pantalla de las primitivas dibujadas. Por ahora usaremos programas existentes.
	*/
	Program program;
	Model model;
};

void MyRender::setup() {
	// Establece el color que se usar� al borrar el buffer de color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Variable vPosition del shader estar� en el �ndice 0
	program.addAttributeLocation(0, "vPosition");
	// Carga los ficheros de shader llamados "triangles"
	program.loadFiles("triangles");
	// Compilamos el programa
	program.compile();
	// Instalamos el programa ya compilado
	program.use();

	// Definici�n de los v�rtices
	std::vector<glm::vec2> vertices;

	vertices = {
		glm::vec2(-0.9f, 0.85f), glm::vec2(-0.9f, -0.9f), glm::vec2(0.85f, -0.9f),
		glm::vec2(-0.85f, 0.9f), glm::vec2(0.9f, -0.85f), glm::vec2(0.9f, 0.9f)
	};

	auto mesh = std::make_shared<Mesh>();

	mesh->addVertices(vertices);
	mesh->addDrawCommand(new PGUPV::DrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size())));

	model.addMesh(mesh);
}


void MyRender::render() {
	glClear(GL_COLOR_BUFFER_BIT);

	model.render();
}

void MyRender::reshape(uint w, uint h) {
	glViewport(0, 0, w, h);
}

int main(int argc, char* argv[]) {
	App& myApp = App::getInstance();
	myApp.initApp(argc, argv, PGUPV::DOUBLE_BUFFER);
	myApp.getWindow().setRenderer(std::make_shared<MyRender>());
	return myApp.run();
}