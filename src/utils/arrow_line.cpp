#include <utils/arrow_line.h>

ArrowLine::~ArrowLine() noexcept {
}

ArrowLine::ArrowLine(ArrowLine&& move) noexcept : points(std::move(move.points)), linePointsNum(std::move(move.linePointsNum)) {
}

ArrowLine& ArrowLine::operator=(ArrowLine&& move) noexcept {
    if (move.VAO) {
        points = std::move(move.points);
    } else {
        VAO = 0;
    }
    return *this;
}

ArrowLine::ArrowLine(vector<Point>& _points, int _linePointsNum) : points(_points), linePointsNum(_linePointsNum) {
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

void ArrowLine::Draw() const {
    glBindVertexArray(this->VAO);
    glDrawArrays(GL_LINES, 0, linePointsNum);
    glDrawArrays(GL_TRIANGLES, linePointsNum, points.size());
    glBindVertexArray(0);
}