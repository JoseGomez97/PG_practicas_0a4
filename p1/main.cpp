#include "PGUPV.h"

using namespace PGUPV;

using glm::vec3;
using std::vector;

class MyRender : public Renderer {
public:
    MyRender() : caja(2.0f, 4.0f, 2.0f, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f)) {};
    void crearAgua(float height, float width);
    void crearTriangleStrip(std::vector<glm::vec3> vertices, const glm::vec4& color);
    void crearCilindro(float originY, float r_bottom, float r_top, float height, uint stacks,
        uint slices, const glm::vec4& color);
    void crearCaja(float size, float height, float width, float originY, const glm::vec4& color);
    void setup(void) override;
    void render(void) override;
    void reshape(uint w, uint h) override;
    void update(uint) override;
private:
    std::shared_ptr<GLMatrices> mats;
    Axes axes;
    Program program;
    WireBox caja;
    Model model;
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

    model.addMesh(mesh);
}

void MyRender::crearTriangleStrip(std::vector<glm::vec3> vertices, const glm::vec4& color) {

    auto mesh = std::make_shared<Mesh>();

    mesh->addVertices(vertices);
    mesh->addDrawCommand(new PGUPV::DrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size())));
    mesh->setColor(color);

    model.addMesh(mesh);
}

void MyRender::crearCilindro(float originY, float r_bottom, float r_top, float height, uint stacks,
    uint slices, const glm::vec4& color) {

    if (r_bottom == 0.0 && r_top == 0.0)
        ERRT("Ambos radios no pueden ser cero.");
    if (r_bottom == 0.0 || r_top == 0.0)
        WARN("Para construir un cono, es preferible definir un radio muy pequeño, "
            "en vez de 0.0");

    auto m = std::make_shared<Mesh>();;
    model.addMesh(m);

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
        glm::vec3(-s2, originY, s2), glm::vec3(-s2, h1, s2),
        glm::vec3(s2, originY, s2), ::vec3(s2, h1, s2),
        glm::vec3(s2, originY, -s2), glm::vec3(s2, h1, -s2),
        glm::vec3(-s2, originY, -s2), glm::vec3(-s2, h1, -s2),
        glm::vec3(-s2, originY, s2), glm::vec3(-s2, h1, s2)
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

void MyRender::setup() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // Habilitamos el z-buffer
    glEnable(GL_DEPTH_TEST);
    // Habilitamos el back face culling. ¡Cuidado! Si al dibujar un objeto hay 
    // caras que desaparecen o el modelo se ve raro, puede ser que estés 
    // definiendo los vértices del polígono del revés (en sentido horario)
    // Si ese es el caso, invierte el orden de los vértices.
    // Puedes activar/desactivar el back face culling en cualquier aplicación 
    // PGUPV pulsando las teclas CTRL+B
    glEnable(GL_CULL_FACE);

    program.addAttributeLocation(Mesh::VERTICES, "position");
    program.addAttributeLocation(Mesh::COLORS, "vertcolor");
    mats = PGUPV::GLMatrices::build();
    program.connectUniformBlock(mats, UBO_GL_MATRICES_BINDING_INDEX);
    program.loadFiles("../recursos/shaders/constantshading");
    program.compile();

    // Establecemos una cámara que nos permite explorar el objeto desde cualquier
    // punto
    setCameraHandler(std::make_shared<OrbitCameraHandler>());

    // Creamos la fuente
    crearCaja(1.8f, 0.5f, 0.1f, 0.0f, glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
    crearCaja(1.0f, 0.3f, 0.05f, 0.6f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    crearCaja(0.6f, 0.2f, 0.02f, 1.1f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
    glm::vec4 colorCilindro = glm::vec4(0.5f, 0.5f, 0.3f, 1.0f);
    crearCilindro(0.75f, 0.1f, 0.1f, 1.5f, 20U, 20U, colorCilindro);
    crearCilindro(1.55f, 0.1f, 0.0f, 0.1f, 20U, 20U, colorCilindro);
}

void MyRender::render() {
    // Borramos el buffer de color y el zbuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Le pedimos a la cámara que nos de la matriz de la vista, que codifica la
    // posición y orientación actuales de la cámara.
    mats->setMatrix(GLMatrices::VIEW_MATRIX, getCamera().getViewMatrix());
    program.use();
    // Dibujamos los ejes
    axes.render();
    // Dibujamos la caja de referencia
    //caja.render();
    // Dibujamos la fuente
    model.render();
    // Si la siguiente llamada falla, quiere decir que OpenGL se encuentra en
    // estado erróneo porque alguna de las operaciones que ha ejecutado
    // recientemente (después de la última llamada a CHECK_GL) ha tenido algún
    // problema. Revisa tu código.
    CHECK_GL();
}

void MyRender::reshape(uint w, uint h) {
    glViewport(0, 0, w, h);
    mats->setMatrix(GLMatrices::PROJ_MATRIX, getCamera().getProjMatrix());
}

// Este método se ejecuta una vez por frame, antes de llamada a render. Recibe el 
// número de milisegundos que han pasado desde la última vez que se llamó, y se suele
// usar para hacer animaciones o comprobar el estado de los dispositivos de entrada
void MyRender::update(uint) {
    // Si el usuario ha pulsado el espacio, ponemos la cámara en su posición inicial
    if (App::isKeyUp(PGUPV::KeyCode::Space)) {
        getCameraHandler()->resetView();
    }
}

int main(int argc, char* argv[]) {
    App& myApp = App::getInstance();
    myApp.initApp(argc, argv, PGUPV::DOUBLE_BUFFER | PGUPV::DEPTH_BUFFER |
        PGUPV::MULTISAMPLE);
    myApp.getWindow().setRenderer(std::make_shared<MyRender>());
    return myApp.run();
}
