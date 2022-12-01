#include <utils/cubemap.h>
#include <stb_image/stb_image.h>
#include <iostream>

using std::string;
using std::cout;
using std::endl;

CubeMap::CubeMap(string path) : _path(path) {}

void CubeMap::Load() {

    // we create and activate the OpenGL cubemap texture
    glGenTextures(1, &_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _id);

    // we load and set the 6 images corresponding to the 6 views of the cubemap
    // we use as convention that the names of the 6 images are "posx, negx, posy, negy, posz, negz", placed at the path passed as parameter
    // we load the images individually and we assign them to the correct sides of the cube map
    loadSide(std::string("posx.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    loadSide(std::string("negx.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    loadSide(std::string("posy.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    loadSide(std::string("negy.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    loadSide(std::string("posz.jpg"), GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    loadSide(std::string("negz.jpg"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // we set how to consider the texture coordinates outside [0,1] range
    // in this case we have a cube map, so
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // we set the binding to 0 once we have finished
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void CubeMap::loadSide(const string& _name, GLenum _side) {
    int w, h;
    unsigned char* image;
    string fullname;

    // full name and path of the side of the cubemap
    fullname = _path + _name;
    // we load the image file
    image = stbi_load(fullname.c_str(), &w, &h, 0, STBI_rgb);
    if (image == nullptr)
        cout << "Failed to load texture!" << endl;
    // we set the image file as one of the side of the cubemap (passed as a parameter)
    glTexImage2D(_side, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);
}

void CubeMap::releaseGpuResources() {
    glDeleteTextures(1, &_id);
}

GLuint CubeMap::GetId() const {
    return _id;
}

CubeMap::~CubeMap() noexcept {
    releaseGpuResources();
}

CubeMap::CubeMap(CubeMap&& move) noexcept : _id(move._id), _path(move._path) {
    move._id = 0;
    move._path = "___MOVED___";
}
CubeMap& CubeMap::operator=(CubeMap&& move) noexcept {
    _id = move._id;
    _path = move._path;
    move._id = 0;
    move._path = "___MOVED___";
    return *this;
}

