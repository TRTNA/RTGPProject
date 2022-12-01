#include <utils/line.h>

Line::~Line() noexcept {
}

Line::Line(Line&& move) noexcept : points(std::move(move.points)) {
}

Line& Line::operator=(Line&& move) noexcept {
    if (move.VAO) {
        points = std::move(move.points);
    } else {
        VAO = 0;
    }
    return *this;
}

Line::Line(vector<Point>& _points) : points(_points) {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), &this->points[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,        // size
            GL_FLOAT, // type
            GL_FALSE, // normalized?
            sizeof(Point),        // stride
            (GLvoid*)0 // array buffer offset
        );
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Point),
            (GLvoid*)offsetof(Point, Color)
        );

        glBindVertexArray(0);
}

void Line::Draw() const {
    glBindVertexArray(this->VAO);
    glDrawArrays(GL_LINES, 0, points.size());
    glBindVertexArray(0);
}