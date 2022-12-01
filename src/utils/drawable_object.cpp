#include <utils/drawable_object.h>

DrawableObject::~DrawableObject() noexcept {
    if (VAO) {
        glDeleteVertexArrays(1, &this->VAO);
        glDeleteBuffers(1, &this->VBO);
    }
}

DrawableObject::DrawableObject() noexcept {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

DrawableObject::DrawableObject(DrawableObject&& move) noexcept : VAO(move.VAO), VBO(move.VBO) {
    move.VAO = 0;
}

DrawableObject& DrawableObject::operator=(DrawableObject&& move) noexcept {
    if (VAO)
    {        
        glDeleteVertexArrays(1, &this->VAO);
        glDeleteBuffers(1, &this->VBO);
    }
    if (move.VAO) {
        VAO = move.VAO;
        VBO = move.VBO;
    } else {
        VAO = 0;
    }
    return *this;
}