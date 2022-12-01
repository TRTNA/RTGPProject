#include <utils/transform.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

Transform::Transform(const glm::vec3 &position, const glm::vec3 &orientation, const glm::vec3 &dimension) : _position(position), _orientation(orientation), _dimension(dimension) {}
Transform::Transform() {}

void Transform::SetPosition(const glm::vec3 &position) {
    _position = glm::vec3(position.x, position.y, position.z);
} 


glm::quat Transform::makeQuaternion(const glm::vec3& axis, float angle) const {
    
    GLfloat sin = glm::sin(angle / 2.0f);
    GLfloat cos = glm::cos(angle / 2.0f);
    glm::quat q = glm::normalize(glm::quat(cos, axis.x * sin, axis.y * sin, axis.z * sin));
    return q;
}

void Transform::Translate(const glm::vec3 &translation)
{
    _position += translation;
}
void Transform::Rotate(const glm::vec3 axis, GLfloat angle) {
    glm::quat q = makeQuaternion(axis, angle);
    _orientation = glm::normalize(_orientation*q);
}
void Transform::Scale(const glm::vec3 &scaling)
{
    _dimension += scaling;
}
glm::mat4 Transform::GetTransformMatrix()
{
    _matrix = glm::mat4(1.0f);
    _matrix = glm::translate(_matrix, _position);
    _matrix = _matrix * glm::transpose(glm::toMat4(_orientation));
    _matrix = glm::scale(_matrix, _dimension);
    return _matrix;
}

void Transform::Reset() {
    _position = glm::vec3(0.0f);
    _orientation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    _dimension = glm::vec3(1.0f);
    _matrix = glm::mat4(1.0f);
}
