#include <utils/texture2D.h>
#include <stb_image/stb_image.h>
#include <iostream>

Texture2D::Texture2D(string path, Texture2DType type) : _path(path), _type(type)  {
}

bool Texture2D::Load()
{
    int w, h, channels;
    unsigned char *image;
    printf("Loading texture from path: %s\n", this->_path.c_str());
    image = stbi_load(this->_path.c_str(), &w, &h, &channels, STBI_rgb);

    if (image == nullptr)
    {
        std::cout << "Failed to load texture at: " << this->_path << std::endl;
        return false;
    }

    glGenTextures(1, &_textureID);
    glBindTexture(GL_TEXTURE_2D, _textureID);

    if (channels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

GLenum Texture2D::GetMinFilter() const
{
    return _minFilter;
}
GLenum Texture2D::GetMagFilter() const
{
    return _magFilter;
}
GLenum Texture2D::GetWrapS() const
{
    return _wrapS;
}
GLenum Texture2D::GetWrapT() const
{
    return _wrapT;
}

void Texture2D::SetMinFilter(GLenum filter)
{
    _minFilter = filter;
}
void Texture2D::SetMagFilter(GLenum filter)
{
    _magFilter = filter;
}
void Texture2D::SetWrapS(GLenum wrap)
{
    _wrapS = wrap;
}
void Texture2D::SetWrapT(GLenum wrap)
{
    _wrapT = wrap;
}
GLuint Texture2D::GetTextureId() const
{
  
  return _textureID;
}

string Texture2D::GetPath() const {
    return _path;
}