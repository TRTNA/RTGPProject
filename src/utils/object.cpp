#include <utils/object.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
using std::string;
using std::vector;

Object::Object() : _transform(Transform()) {}
Object::Object(string name) : _transform(Transform()), _name(name) {}

Object::Object(Object &&move) noexcept : _transform(std::move(move._transform)), _model(move._model), _name(move._name)
{
}
Object &Object::operator=(Object &&move) noexcept
{
    _transform = std::move(move._transform);
    _model = move._model;
    _name = move._name;
    return *this;
}

void Object::SetModel(Model *model)
{
    if (model == nullptr)
    {
        printf("Assigned model's pointer is null!\n");
        return;
    }
    _model = model;
}
void Object::Render(Shader &shader, glm::mat4 &view)
{
    // TODO ancora provvisorio, la texture 0 del modello non è per forza la diffusive
    glm::mat4 modelMatrix = _transform.GetTransformMatrix();
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix3fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
    _model->Draw();
}

Object::~Object() noexcept
{
}

Transform &Object::GetTransform()
{
    return _transform;
}