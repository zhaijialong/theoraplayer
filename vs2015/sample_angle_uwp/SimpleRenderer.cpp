//
// This file is used by the template to render a basic scene using GL.
//

#include "pch.h"
#include "SimpleRenderer.h"
#include "MathHelper.h"

// These are used by the shader compilation methods.
#include <vector>
#include <iostream>
#include <fstream>

using namespace Platform;

using namespace sample_angle_uwp;

#define STRING(s) #s

GLuint CompileShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);

    const char *sourceArray[1] = { source.c_str() };
    glShaderSource(shader, 1, sourceArray, NULL);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

    if (compileResult == 0)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::wstring errorMessage = std::wstring(L"Shader compilation failed: ");
        errorMessage += std::wstring(infoLog.begin(), infoLog.end()); 

        throw Exception::CreateException(E_FAIL, ref new Platform::String(errorMessage.c_str()));
    }

    return shader;
}

GLuint CompileProgram(const std::string &vsSource, const std::string &fsSource)
{
    GLuint program = glCreateProgram();

    if (program == 0)
    {
        throw Exception::CreateException(E_FAIL, L"Program creation failed");
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vs == 0 || fs == 0)
    {
        glDeleteShader(fs);
        glDeleteShader(vs);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == 0)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<GLchar> infoLog(infoLogLength);
        glGetProgramInfoLog(program, (GLsizei)infoLog.size(), NULL, infoLog.data());

        std::wstring errorMessage = std::wstring(L"Program link failed: ");
        errorMessage += std::wstring(infoLog.begin(), infoLog.end()); 

        throw Exception::CreateException(E_FAIL, ref new Platform::String(errorMessage.c_str()));
    }

    return program;
}

static std::string PlatformStringToString(Platform::String^ s) {
    std::wstring t = std::wstring(s->Data());
    return std::string(t.begin(), t.end());
}

static std::string getAppPath()
{
    Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
    return PlatformStringToString(package->InstalledLocation->Path);
}

static int nextPow2(int x)
{
    int y;
    for (y = 1;y<x;y *= 2);
    return y;
}

SimpleRenderer::SimpleRenderer() :
    mWindowWidth(0),
    mWindowHeight(0),
    mDrawCount(0)
{
    _mgr = new TheoraVideoManager();

    std::string path = getAppPath();
    _clip = _mgr->createVideoClip(path + "/bunny.ogv", TH_RGB, 16);

    //  use this if you want to preload the file into ram and stream from there
    //	clip=mgr->createVideoClip(new TheoraMemoryFileDataSource("../media/short" + resourceExtension),TH_RGB);
    _clip->setAutoRestart(1);

    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _clip->getWidth(), _clip->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Vertex Shader source
    const std::string vs = STRING
    (
        attribute vec2 a_pos;
        attribute vec2 a_uv; 
        varying vec2 v_uv;
        void main()
        {
            gl_Position = vec4(-a_pos.x, a_pos.y, 0.0, 1.0);
            v_uv = -a_uv * 0.5 + 0.5;
        }
    );

    // Fragment Shader source
    const std::string fs = STRING
    (
        precision mediump float;

        varying vec2 v_uv;
        uniform sampler2D u_texture;

        void main()
        {
            //...
            gl_FragColor = texture2D(u_texture, v_uv);


        }
    );

    // Set up the shader and its uniform/attribute locations.
    mProgram = CompileProgram(vs, fs);
    a_Pos = glGetAttribLocation(mProgram, "a_pos");
    a_uv = glGetAttribLocation(mProgram, "a_uv");
    u_texture = glGetUniformLocation(mProgram, "u_texture");
}

SimpleRenderer::~SimpleRenderer()
{
    if (mProgram != 0)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }

    delete _mgr;
}

void SimpleRenderer::Draw()
{
    glClear(GL_COLOR_BUFFER_BIT);

    _mgr->update(0.016);//hard coded
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture);

    TheoraVideoFrame* f = _clip->getNextFrame();
    if (f)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _clip->getWidth(), _clip->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _clip->getWidth(), f->getHeight(), GL_RGB, GL_UNSIGNED_BYTE, f->getBuffer());
        _clip->popFrame();
    }

    float w = _clip->getSubFrameWidth(), h = _clip->getSubFrameHeight();
    float sx = _clip->getSubFrameOffsetX(), sy = _clip->getSubFrameOffsetY();
    float tw = nextPow2(w), th = nextPow2(h);

    drawAxisAlignedQuad(w / tw, h / th, sx / tw, sy / th);

    mDrawCount += 1;
}

void SimpleRenderer::drawAxisAlignedQuad(float sw, float sh, float sx, float sy)
{
    glUseProgram(mProgram);
    glUniform1i(u_texture, 0);

    const float s_Quad_VertexData[] = { -1.0,  1.0 , 1.0,  1.0, -1.0, -1.0 , 1.0, -1.0 };
    const float s_Quad_TexCoordData[] = { 0, 1,  1, 1, 0, 0,  1, 0 };


    glEnableVertexAttribArray(a_Pos);
    glVertexAttribPointer(a_Pos, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, s_Quad_VertexData);
    glEnableVertexAttribArray(a_uv);
    glVertexAttribPointer(a_uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, s_Quad_VertexData);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void SimpleRenderer::UpdateWindowSize(GLsizei width, GLsizei height)
{
    glViewport(0, 0, width, height);
    mWindowWidth = width;
    mWindowHeight = height;
}
