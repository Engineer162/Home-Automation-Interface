#pragma once

struct UiFrameContext
{
    const char* platform;
    int framebuffer_width;
    int framebuffer_height;
    float framerate;
};

using UiWindowRenderFn = void(*)(const UiFrameContext&);

struct UiWindow
{
    const char* id;
    UiWindowRenderFn render;
};
