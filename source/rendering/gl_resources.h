#ifndef RME_RENDERING_GL_RESOURCES_H
#define RME_RENDERING_GL_RESOURCES_H

#include "main.h"

class GLTexture {
public:
    GLTexture() : id(0) {}
    ~GLTexture() { release(); }

    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    GLTexture(GLTexture&& other) noexcept : id(other.id) { other.id = 0; }
    GLTexture& operator=(GLTexture&& other) noexcept {
        if (this != &other) {
            release();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    void create() {
        if (id == 0) {
            glGenTextures(1, &id);
        }
    }

    void release() {
        if (id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
        }
    }

    void bind(GLenum target = GL_TEXTURE_2D) const {
        glBindTexture(target, id);
    }

    operator GLuint() const { return id; }
    GLuint get() const { return id; }

private:
    GLuint id;
};

class GLBuffer {
public:
    GLBuffer() : id(0) {}
    ~GLBuffer() { release(); }

    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;

    GLBuffer(GLBuffer&& other) noexcept : id(other.id) { other.id = 0; }
    GLBuffer& operator=(GLBuffer&& other) noexcept {
        if (this != &other) {
            release();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    void create() {
        if (id == 0) {
            glGenBuffers(1, &id);
        }
    }

    void release() {
        if (id != 0) {
            glDeleteBuffers(1, &id);
            id = 0;
        }
    }

    void bind(GLenum target = GL_ARRAY_BUFFER) const {
        glBindBuffer(target, id);
    }

    void unbind(GLenum target = GL_ARRAY_BUFFER) const {
        glBindBuffer(target, 0);
    }

    void data(GLsizeiptr size, const void* data, GLenum usage = GL_STATIC_DRAW, GLenum target = GL_ARRAY_BUFFER) {
        bind(target);
        glBufferData(target, size, data, usage);
    }

    operator GLuint() const { return id; }
    GLuint get() const { return id; }

private:
    GLuint id;
};

#endif
