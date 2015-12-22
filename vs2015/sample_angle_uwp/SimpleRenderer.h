#pragma once

#include "pch.h"
#include "theoraplayer/TheoraPlayer.h"
#include "theoraplayer/TheoraDataSource.h"

namespace sample_angle_uwp
{
    class SimpleRenderer
    {
    public:
        SimpleRenderer();
        ~SimpleRenderer();
        void Draw();
        void UpdateWindowSize(GLsizei width, GLsizei height);

    private:
        TheoraVideoManager* _mgr;
        TheoraVideoClip* _clip;

        GLuint _texture;

        void drawAxisAlignedQuad(float sw, float sh, float sx, float sy);

        GLuint mProgram;
        GLuint a_Pos;
        GLuint a_uv;
        GLuint u_texture;

        GLsizei mWindowWidth;
        GLsizei mWindowHeight;

        int mDrawCount;
    };
}