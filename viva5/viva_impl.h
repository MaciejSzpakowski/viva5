#pragma once

// Viva engine by Maciej Szpakowski
// IMPORTANT, can compile only in x64

// error checking
// #define VI_VALIDATE yet another error checking category

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <functional>
#include <random>

#define WIN32_LEAN_AND_MEAN
#include <Ws2tcpip.h> // winsock
#include <WinSock2.h> // winsock
#include <Windows.h> // winapi

// image loading library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// windows D3D11
#include <d3d11.h>
#include <d3dcompiler.h>
//#pragma comment(linker, "/subsystem:windows")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")
#pragma comment(lib, "ws2_32.lib")

#define KEYBOARD_KEY_COUNT 256
#define WND_CLASSNAME "mywindow"

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short ushort;

// if you see a problem with vi::memory
// it's because you have to enable new namespace syntax (c++latest)
namespace vi::memory
{
    struct alloctrack
    {
        std::vector<void*> allocations;
        bool track;

        // size is how many elements of T (NOT size in bytes of the chunk to allocate)
        template<typename T>
        T* alloc(uint size)
        {
#ifdef VI_VALIDATE
            if (size == 0)
            {
                fprintf(stderr, "size is 0\n");
                return nullptr;
            }
#endif

            T* block = (T*)malloc(size * sizeof(T));

            if (this->track) this->allocations.push_back(block);

            return block;
        }

        void free(void* block)
        {
            ::free(block);
            if (this->track)
            {
                for (uint i = 0; i < this->allocations.size(); i++)
                {
                    if (this->allocations[i] == block)
                    {
                        this->allocations.erase(this->allocations.begin() + i);
                        break;
                    }
                }
            }
        }

        void report()
        {
            for (uint i = 0; i < this->allocations.size(); i++) fprintf(stderr, "Not freed: %p\n", this->allocations[i]);
        }
    };
}

namespace vi::time
{
    struct timer
    {
        float gameTime;
        float tickTime;
        long long ticksPerSecond;
        long long startTime;
        long long prevTick;

        void init()
        {
            LARGE_INTEGER li;
            BOOL result = ::QueryPerformanceFrequency(&li);

#ifdef VI_VALIDATE
            if (!result)
            {
                fprintf(stderr, "QueryPerformanceFrequency failed");
                return;
            }
#endif
            this->gameTime = 0;
            this->tickTime = 0;
            this->ticksPerSecond = li.QuadPart;
            ::QueryPerformanceCounter(&li);
            this->startTime = li.QuadPart;
            this->prevTick = li.QuadPart;
        }

        // this updates the timer so it must be called once per frame
        void update()
        {
            LARGE_INTEGER currentTime;
            ::QueryPerformanceCounter(&currentTime);

            long long frameDelta = currentTime.QuadPart - this->prevTick;
            long long gameDelta = currentTime.QuadPart - this->startTime;
            this->prevTick = currentTime.QuadPart;
            this->tickTime = (float)((double)frameDelta / (double)this->ticksPerSecond);
            this->gameTime = (float)((double)gameDelta / (double)this->ticksPerSecond);
        }

        // get tick time
        // tick time is the time between two calls to 'updateTimer'
        // can be used as frame time if updateTimer is called once per frame
        float getTickTimeSec()
        {
            return this->tickTime;
        }

        // get time since game started in seconds
        float getGameTimeSec()
        {
            return this->gameTime;
        }
    };
}

namespace vi::util
{
    template<typename T>
    void swap(T& a, T& b)
    {
        T tmp = a;
        a = b;
        b = tmp;
    }

    // zero out memory at dst sizeof(T)
    template<typename T>
    void zero(T* dst)
    {
        memset(dst, 0, sizeof(T));
    }

    template<typename T>
    void zeron(T* dst, uint len)
    {
        memset(dst, 0, sizeof(T) * len);
    }

    struct rng
    {
        std::mt19937 mt;
        std::uniform_int_distribution<int> distr;

        void init(int min, int max)
        {
            mt.seed(::time(0));
            this->distr = std::uniform_int_distribution<int>(min, max);

            // warm up
            for (int i = 0; i < 100; i++)
                this->rnd();
        }

        /// <summary>
        /// get random number between min and max inclusive
        /// </summary>
        int rnd()
        {
            return this->distr(this->mt);
        }
    };

    template<typename T>
    T* find(T* arr, uint len, std::function<bool(T*)> pred)
    {
        for (uint i = 0; i < len; i++)
        {
            if (pred(arr + i)) return arr + i;
        }

        return nullptr;
    }
}

namespace vi::math
{
    const float PI = 3.1415926f;
    const float TWO_PI = PI * 2;
    const float HALF_PI = PI / 2;
    const float THIRD_PI = PI / 3;
    const float FORTH_PI = PI / 4;
    const float DEGREE = PI / 180;

    // magnitude of a vector
    float mag2D(float x, float y)
    {
        return sqrtf(x * x + y * y);
    }

    // magnitude squared of a vector
    float mag2Dsq(float x, float y)
    {
        return x * x + y * y;
    }

    // normalize src vector (x,y), result is dst vector (dstx, dsty)
    void norm2D(float x, float y, float* dstx, float* dsty)
    {
        float mag = mag2D(x, y);
        *dstx = x / mag;
        *dsty = y / mag;
    }

    float distance2D(float x1, float y1, float x2, float y2)
    {
        return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }

    float distance2Dsq(float x1, float y1, float x2, float y2)
    {
        return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    }

    // sets velx, vely in such way that object will move from start to dst at speed 'speed'
    void moveTo(float startx, float starty, float dstx, float dsty, float speed, float* velx, float* vely)
    {
        float dx = dstx - startx;
        float dy = dsty - starty;
        norm2D(dx, dy, velx, vely);
        *velx *= speed;
        *vely *= speed;
    }

    // calculates the angle when (x,y) points at (targetx,targety)
    float lookAt(float x, float y, float targetx, float targety, float* rot)
    {
        // not implemented
        assert(false);
        return 0;
    }
}

namespace vi::system
{
    size_t getFileSize(FILE* file)
    {
        size_t retval = 0;
        fseek(file, SEEK_SET, 0);

        while (!feof(file))
        {
            fgetc(file);
            retval++;
        }

        fseek(file, SEEK_SET, 0);

        return retval;
    }
    short wheelDelta = 0;
    int rawMouseDeltax = 0;
    bool focused = false;
    int rawMouseDeltay = 0;
    bool quitMessagePosted = false;
    byte* readFile(const char* filename, vi::memory::alloctrack* a, size_t* outSize)
    {
        FILE* file = fopen(filename, "rb");

        if (!file)
        {
            fprintf(stderr, "Could not open %s\n", filename);
            exit(1);
        }

        size_t size = getFileSize(file);
        byte* block = a->alloc<byte>(size + 1);
        size_t it = 0;

        while (true)
        {
            int c = fgetc(file);

            if (c == EOF)
                break;

            block[it++] = c;
        }

        fclose(file);
        block[it++] = 0;

        if (outSize)
            *outSize = size - 1;

        return block;
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_SYSKEYDOWN:
        {
            if (wParam == VK_MENU)//ignore left alt stop
            {
            }
            else
            {
                return DefWindowProc(hwnd, uMsg, wParam, lParam); // this makes ALT+F4 work
            }

            break;
        }
        case WM_CLOSE:
        {
            ShowWindow(hwnd, false);
            PostQuitMessage(0);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            break;
        }
        case WM_INPUT:
        {
            UINT dwSize = 48; // 48 for 64bit build
            static BYTE lpb[48];

            // this gets relative coords
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

            RAWINPUT* raw = (RAWINPUT*)lpb;

            if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                rawMouseDeltax = raw->data.mouse.lLastX;
                rawMouseDeltay = raw->data.mouse.lLastY;
            }

            break;
        }
        case WM_SETFOCUS:
            focused = true;
            break;
        case WM_KILLFOCUS:
            focused = false;
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        return 0;
    }

    struct windowInfo
    {
        uint height;
        uint width;
        const char* title;
    };

    struct window
    {
        HWND handle;
        HINSTANCE hinstance;
        // client height
        uint height;
        // client width
        uint width;

        void init(windowInfo* info)
        {
            this->width = info->width;
            this->height = info->height;
            this->hinstance = GetModuleHandle(0);
            HBRUSH bg = CreateSolidBrush(RGB(255, 0, 0));

            WNDCLASS wc = { };
            ZeroMemory(&wc, sizeof(WNDCLASS));
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = this->hinstance;
            wc.lpszClassName = WND_CLASSNAME;
            wc.hbrBackground = bg;
            RegisterClass(&wc);

            DWORD wndStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
            RECT r = { 0, 0, (LONG)info->width, (LONG)info->height };
            // this tells you what should be the window size if r is rect for client
            // IMPORTANT. window client, swap chain and VkImages (render target) dimensions must match
            AdjustWindowRect(&r, wndStyle, false);
            this->handle = CreateWindowEx(0, WND_CLASSNAME, info->title, wndStyle, 100, 100,
                r.right - r.left, r.bottom - r.top, 0, 0, this->hinstance, 0);
#ifdef VI_VALIDATE
            if (!this->handle)
            {
                fprintf(stderr, "Failed to create window");
                exit(1);
            }
#endif
            ShowWindow(this->handle, SW_SHOW);

            USHORT HID_USAGE_PAGE_GENERIC = 1;
            USHORT HID_USAGE_GENERIC_MOUSE = 2;

            RAWINPUTDEVICE Rid;
            Rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
            Rid.usUsage = HID_USAGE_GENERIC_MOUSE;
            Rid.dwFlags = RIDEV_INPUTSINK;
            Rid.hwndTarget = this->handle;
            RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE));

            SetFocus(this->handle);
        }

        void destroy()
        {
            DestroyWindow(this->handle);
            UnregisterClass(WND_CLASSNAME, this->hinstance);
        }

        bool update()
        {
            // reset delta
            wheelDelta = 0;
            rawMouseDeltax = 0;
            rawMouseDeltay = 0;

            MSG msg;
            msg.message = 0;

            while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                // NEED additional check because WM_QUIT might be immediately followed by something
                // else in the same while loop so break right away
                if (msg.message == WM_QUIT)
                    break;

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (msg.message == WM_QUIT) return false;
            return true;
        }
    };
}

// d3d11
namespace vi::gl
{
    const uint APPLY_TRANSFORM = 4;
    const uint psBufferSize = 16;
    const char rc_PixelShader[] = R"(
Texture2D textures[1];
SamplerState ObjSamplerState;

cbuffer jedziemy
{
	bool notexture;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
    uint4 data: COLOR2;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    if(input.Col.a == 0.0f)
    {
        discard;
        return float4(0,0,0,0);
    }
    else if(notexture)
    {
        return float4(input.Col.rgba);
    }
    else
    {
		float4 result = textures[0].Sample(ObjSamplerState, input.TexCoord);
        if(result.a == 0.0f)
            discard;
		return result * input.Col;
    }
}
)";

    const char rc_VertexShader[] = R"(
struct sprite
{
    float x,y,z;
    float sx,sy;
    float rot;
    float ox,oy;
    float4 uv;
    float4 color;
};

struct camera
{
	float aspectRatio;
	float x;
	float y;
	float rotation;
	float scale;
};

cbuffer jedziemy: register(b0)
{
	sprite spr;
};

cbuffer poziolo: register(b1)
{
	camera camObj;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
    uint4 data: COLOR2;
};

static float4 vertices[6] = {
    float4(-0.5f, -0.5f, 1.0f, 1.0f),
    float4(0.5f, -0.5f, 0.0f, 1.0f),
    float4(-0.5f, 0.5f, 1.0f, 0.0f),
    float4(-0.5f, 0.5f, 1.0f, 0.0f),
    float4(0.5f, -0.5f, 0.0f, 1.0f),
	float4(0.5f, 0.5f, 0.0f, 0.0f)
};

static uint2 uv[6] = {
    uint2(0,3),
    uint2(2,3),
    uint2(0,1),
    uint2(0,1),
    uint2(2,3),
	uint2(2,1)
};

VS_OUTPUT main(uint vid : SV_VertexID)
{
	// camera
    // that is correct camera transformation
    // includes aspect ratio adjustment and it FIRST moves camera to postion and then scales
    // so when zooming, it always zooms around the center of the screen
	float4x4 cam = float4x4(
		1/camObj.aspectRatio * camObj.scale, 0, 0, 1/camObj.aspectRatio * camObj.scale * -camObj.x,
		0, camObj.scale, 0, -camObj.scale * -camObj.y,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

    // hack do draw a line because I dont want to write a new VS for this
    if(spr.z < 0)
    {
        // this is for point A
        float4 pos = float4(spr.x,spr.y,0,1.0f);
        // this is for point B
        if(vid > 0)
            pos = float4(spr.sx,spr.sy,0,1.0f);
        VS_OUTPUT output;
	    output.Pos = mul(cam, pos);
        output.Pos.z = -spr.z;
        if(vid > 0)
            output.Pos.z = -spr.rot;
	    output.Col = spr.color;
        output.TexCoord = float2(0,0);
        output.data = float4(0,0,0,0);
	    return output;
    }

	// origin
	float4x4 ori = float4x4(
		1, 0, 0, -spr.ox,
		0, 1, 0, spr.oy,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	// scale
	float4x4 sca = float4x4(
		spr.sx, 0, 0, 0,
		0, spr.sy, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	// rot
	float4x4 rot = float4x4(
		cos(spr.rot), sin(spr.rot), 0, 0,
		-sin(spr.rot), cos(spr.rot), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	// loc
	float4x4 loc = float4x4(
		1, 0, 0, spr.x,
		0, 1, 0, -spr.y,
		0, 0, 1, 0,
		0, 0, 0, 1 // z from transform is not put over here because it's on the vertex itself
	);
	float4 pos = float4(vertices[vid].x,vertices[vid].y,0,1.0f);

	VS_OUTPUT output;
	output.Pos = mul(mul(mul(mul(mul(cam,loc), rot), sca), ori), pos);
    output.Pos.z = spr.z;
	output.Col = spr.color;
    int u = uv[vid].x;
    int v = uv[vid].y;
    output.TexCoord = float2(spr.uv[u],spr.uv[v]);
    output.data = float4(0,0,0,0);

	return output;
}
)";

    const char rc_VertexShaderMesh[] = R"(
struct VertexInputType
{
    float3 pos : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 light : LIGHT;
};

struct world
{
    float x,y,z, pad1;
    float q1,q2,q3, pad2;
    float sx,sy,sz;
    uint data;
    float4 color;
};

struct view
{
    float aspectRatio,fovy,near,far;
    float eyex,eyey,eyez;
    float atx,aty,atz;
    float upx,upy,upz;
};

cbuffer jedziemy: register(b0)
{
    world w;
};

cbuffer poziolo : register(b1)
{
	view v;
};

cbuffer testb : register(b2)
{
    float4x4 transform;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
    uint4 data: COLOR2;
};

float4x4 calcWorldViewProj()
{
    float cr = cos(w.q1 * 0.5);
    float sr = sin(w.q1 * 0.5);
    float cp = cos(w.q2 * 0.5);
    float sp = sin(w.q2 * 0.5);
    float cy = cos(w.q3 * 0.5);
    float sy = sin(w.q3 * 0.5);
    float qw = cr * cp * cy + sr * sp * sy;
    float qx = sr * cp * cy - cr * sp * sy;
    float qy = cr * sp * cy + sr * cp * sy;
    float qz = cr * cp * sy - sr * sp * cy;
    float4x4 rotMat = {
        qw*qw+qx*qx-qy*qy-qz*qz, 2*(qx*qy-qw*qz), 2*(qw*qy+qx*qz),0,
        2*(qx*qy+qw*qz),qw*qw-qx*qx+qy*qy-qz*qz,2*(qy*qz-qw*qx),0,
        2*(qx*qz-qw*qy),2*(qw*qx+qy*qz),qw*qw-qx*qx-qy*qy+qz*qz,0,
        0,0,0,1
    };
    
    float4x4 locMat = {
            1,0,0, w.x,
            0,1,0, w.y,
            0,0,1, w.z,
            0,0,0, 1,
        };

    float4x4 scaleMat = {
        w.sx,0,0,0,
        0,w.sy,0,0,
        0,0,w.sz,0,
        0,0,0,1
    };

    float4x4 worldMat = mul(locMat, mul(rotMat, scaleMat));
    
    float3 eye = {v.eyex, v.eyey, v.eyez};
    float3 at = {v.atx,v.aty,v.atz};
    float3 up = {v.upx,v.upy,v.upz};
    float3 zaxis = normalize(at - eye);
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);
    float4x4 viewMat = {
        xaxis.x, xaxis.y, xaxis.z, -dot( xaxis, eye ),
        yaxis.x, yaxis.y, yaxis.z, -dot( yaxis, eye ),
        zaxis.x, zaxis.y, zaxis.z, -dot( zaxis, eye ),
        0,0,0,1
    };

    // left handed
    float h = 1/tan(v.fovy*0.5);
    float4x4 projMat = {
        h/v.aspectRatio, 0, 0, 0,
        0, h, 0, 0,
        0, 0, v.far/(v.far-v.near), (-v.near*v.far)/(v.far-v.near),
        0, 0, 1, 0
    };

    float4x4 worldViewProj = mul(projMat, mul(viewMat, worldMat));
    return worldViewProj;
}

VS_OUTPUT main(VertexInputType data)
{    
	float4 pos = float4(data.pos.x,data.pos.y,data.pos.z,1.0f);

	VS_OUTPUT output;
    
    if(w.data & 4)
    {
	    output.Pos = mul(transform,pos);
    }
    else if(w.data & 8)
    {
        output.Pos = pos;
        output.Col = float4(data.light);
	    output.TexCoord = float2(data.TexCoord[0],data.TexCoord[1]);
        output.data = uint4(w.data,0,0,0);
	    return output;
    }        
    else
    {
        output.Pos = mul(calcWorldViewProj(),pos);
    }

	output.Col = w.color;
    output.Col.a = 1;
	output.TexCoord = float2(data.TexCoord[0],data.TexCoord[1]);
    output.data = uint4(w.data,0,0,0);

	return output;
}
)";

    struct vector4
    {
        float x, y, z, w;
    };

    struct vector3
    {
        float x, y, z;
    };

    struct camera
    {
        float aspectRatio;
        float x;
        float y;
        float rotation;
        float scale;
        // padding because this struct must be multiple of 16bytes
        byte padding[12];
    };

    struct camera3D
    {
        float aspectRatio;
        float fovy;
        float znear;
        float zfar;
        /// <summary>
        /// where camera is
        /// </summary>
        vector3 eye;
        /// <summary>
        /// where camera looks at
        /// </summary>
        vector3 at;
        /// <summary>
        /// up vector, usually (0,1,0)
        /// </summary>
        vector3 up;
        // padding because this struct must be multiple of 16bytes
        float padding[3];
    };

    struct vector2
    {
        float x, y;
    };

    struct color
    {
        float r, g, b, a;
    };

    struct uv
    {
        float left, top, right, bottom;
    };

    struct texture
    {
        int index;
        int width;
        int height;
        ID3D11ShaderResourceView* shaderResource;
    };

    // WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // you updating something ? update shaders as well
    // struct passed to uniform buffer must be multiple of 16bytes
    struct sprite1
    {
        float x;
        float y;
        float z;
        // scale x
        float sx;

        // 16 byte

        // scale y
        float sy;
        // angle
        float rot;
        // origin x
        float ox;
        // origin y
        float oy;

        // 16 byte

        // uv left
        float left;
        // uv top
        float top;
        // uv right
        float right;
        // uv bottom
        float bottom;

        // 16 byte

        float r;
        float g;
        float b;
        float a;

        // 16 byte
        // some extra settings
        uint nodraw : 1;
        /// <summary>
        /// no need to set manually, set to true if no texture on sprite
        /// </summary>
        uint notexture : 1;
        uint padding : 30;

        texture* t;
    };

    struct sprite2
    {
        vector3 pos;
        vector2 scale;
        float rot;
        vector2 origin;
        uv uv1;
        color col;
        uint flags;
        texture* t;
    };

    struct line
    {
        float x1;
        float y1;
        float z1;
        float x2;
        float y2;
        float z2;
        float _1;
        float _2;
        float _3;
        float _4;
        float _5;
        float _6;
        float r;
        float g;
        float b;
        float a;
    };

    // 16 bytes alignment, although I'm not sure it's necessary
    _declspec(align(16))
        union sprite
    {
        sprite1 s1;
        sprite2 s2;
        line line;

        // makes minimum changes to make object show when drawn
        void init(texture* t)
        {
            vi::util::zero(this);
            this->s1.t = t;
            this->s2.col = { 1,1,1,1 };
            this->s2.scale = { 1,1 };
            this->s2.uv1 = { 0,0,1,1 };
        }
    };

    struct dynamic
    {
        sprite* s;
        time::timer* t;
        float velx, vely, velz;
        float accx, accy, accz;
        // angular velocity
        float velrot;
        // angular acceleration
        float accrot;
        // grow speed
        float velsx, velsy;
        // grow acceleration
        float accsx, accsy;
        float _lastUpdate;

        void init(sprite* s, time::timer* t)
        {
            vi::util::zero(this);
            this->s = s;
            this->t = t;
            this->_lastUpdate = t->getGameTimeSec();
        }

        void update()
        {
            float currentTime = this->t->getGameTimeSec();
            float delta = currentTime - this->_lastUpdate;
            this->_lastUpdate = currentTime;

            this->velx += this->accx * delta;
            this->s->s1.x += this->velx * delta;
            this->vely += this->accy * delta;
            this->s->s1.y += this->vely * delta;
            this->velz += this->accz * delta;
            this->s->s1.z += this->velz * delta;
            this->velrot += this->accrot * delta;
            this->s->s1.rot += this->velrot * delta;
            this->velsx += this->accsx * delta;
            this->s->s1.sx += this->velsx * delta;
            this->velsy += this->accsy * delta;
            this->s->s1.sy += this->velsy * delta;
        }
    };

    // for now speed must be non negative
    struct animation
    {
        sprite* s;
        time::timer* t;
        uv* u;
        float speed;
        uint frameCount;
        int currentFrame;
        uint stopAfter;
        // this is true if last 'updateAnimation' changed 'currentFrame'
        bool frameChanged;

        uint _frameChanges;
        float _elapsedTime;
        float _lastUpdate;
        bool _playing;

        // 'stopAfter' stop animation after that many frame changes, 0 = never stop
        void init(sprite* s, time::timer* t, uv* u, uint frameCount, float secondsPerFrame, uint stopAfter)
        {
            this->t = t;
            this->s = s;
            this->u = u;
            this->speed = secondsPerFrame;
            this->frameCount = frameCount;
            this->currentFrame = 0;
            this->stopAfter = stopAfter;
            this->frameChanged = false;
            this->_elapsedTime = 0;
            this->_playing = false;
            this->_frameChanges = 0;
            this->_lastUpdate = 0;
            // update uv to the current frame
            this->s->s2.uv1 = this->u[this->currentFrame];
        }

        // make 'updateAnimation' animate frames
        void play()
        {
            if (this->_playing) return;

            this->_playing = true;
            this->_lastUpdate = this->t->getGameTimeSec();
            // update uv to the current frame
            this->s->s2.uv1 = { this->u[this->currentFrame] };
        }

        // animation will stop and 'updateAnimation' will no longer animate frames
        void pause()
        {
            this->_playing = false;
        }

        // stop and reset animation so it can be played from the beginning
        void reset()
        {
            this->currentFrame = 0;
            this->frameChanged = false;
            this->_elapsedTime = 0;
            this->_playing = false;
            this->_frameChanges = 0;
        }

        // stops playing 'from' starts playing 'to'
        // if 'from' is not playing ot 'to' is playing then nothing happens
        void change(animation* dst)
        {
            if (!this->_playing || dst->_playing) return;

            this->reset();
            dst->play();
        }

        void flipHorizontally()
        {
            for (uint i = 0; i < this->frameCount; i++)
                util::swap(this->u[i].left, this->u[i].right);
        }

        void flipVertically()
        {
            for (uint i = 0; i < this->frameCount; i++)
                util::swap(this->u[i].top, this->u[i].bottom);
        }

        // current algorithm 
        // measure how much time elapsed since last update and add it to total time elapsed
        // if total time elapsed is greater than speed (thus measured in seconds per frame)
        // then reduce total time elapsed by speed and change frame
        void update()
        {
            // not playing, early break
            if (!this->_playing) return;

            // set frame changed to false to invalidate previous true
            this->frameChanged = false;
            float gameTime = this->t->getGameTimeSec();
            // elpased since last update
            float elapsed = gameTime - this->_lastUpdate;
            // update last update
            this->_lastUpdate = gameTime;
            // update elapsed
            this->_elapsedTime += elapsed;

            // see if enough time elapsed to change frame
            if (this->_elapsedTime > this->speed)
            {
                // subtract the duration of one frame
                this->_elapsedTime -= this->speed;
                // update frame index including looping
                this->currentFrame = (this->currentFrame + 1) % this->frameCount;
                this->frameChanged = true;
                this->_frameChanges++;

                // update uv
                uv* uv = this->u + this->currentFrame;
                this->s->s2.uv1 = *uv;

                // enough frame changed occured so stop playing
                if (this->stopAfter != 0 && this->_frameChanges > this->stopAfter)
                    this->_playing = false;
            }
        }
    };

    struct font
    {
        texture* tex;
        uv uv[256];
    };

    struct text
    {
        font* f;
        sprite* s;
        uint capacity;
        const char* str;
        float horizontalSpace;
        float verticalSpace;

        void init(font* f, sprite* s, uint capacity, const char* str)
        {
#ifdef VI_VALIDATE
            if (!f || !s || capacity < 1 || !str)
            {
                fprintf(stderr, "%s invalid argument\n", __func__);
                return;
            }

            if (!f->tex)
            {
                fprintf(stderr, "%s font has not texture\n", __func__);
                return;
            }
#endif

            this->f = f;
            this->s = s;
            this->capacity = capacity;
            this->str = str;
            this->verticalSpace = 0;
            this->horizontalSpace = 0;

            for (uint i = 0; i < capacity; i++)
            {
                vi::util::zero(s + i);
                s[i].init(f->tex);
                s[i].s2.col = { 0,0,0,1 };
            }
        }

        void update()
        {
            float x = s->s1.x;
            float y = s->s1.y;

            bool zero = false;
            for (uint i = 0; i < this->capacity; i++)
            {
                if (!zero && this->str[i] == 0) zero = true;

                if (zero)
                {
                    this->s[i].s1.nodraw = 1;
                }
                else if (this->str[i] == '\n')
                {
                    x = this->s[0].s1.x;
                    y += this->s[0].s1.sy + this->verticalSpace;
                    this->s[i].s1.nodraw = true;
                    this->s[i].s2.scale = { 0,0 };
                }
                else
                {
                    this->s[i].s2.uv1 = this->f->uv[str[i] - ' '];
                    this->s[i].s1.x = x;
                    this->s[i].s1.y = y;
                    // set scale and origin equal to the first sprite in the set
                    this->s[i].s2.scale = this->s[0].s2.scale;
                    this->s[i].s2.origin = this->s[0].s2.origin;
                    this->s[i].s1.nodraw = 0;

                    x += this->s[0].s1.sx + this->horizontalSpace;
                }
            }
        }
    };

    struct vertex
    {
        vector3 pos;
        vector2 uv;
        /// <summary>
        /// light information
        /// </summary>
        color color;
    };

    struct mesh
    {
        vertex* v;
        /// <summary>
        /// vertex order for index buffer, this can be null
        /// if it's null then vertices are rendered in order, topology is always 'triangle list'
        /// </summary>
        uint* index;
        texture* t;
        ID3D11Buffer* vertexBuffer;
        ID3D11Buffer* indexBuffer;
        /// <summary>
        /// this should be multiple of 3 since topology is triangle list
        /// </summary>
        uint vertexCount;
        uint indexCount;

        vector3 pos;
        float pad1;
        vector3 rot;
        float pad2;
        vector3 sca;
        uint data;
        color color;
        uint pad3;
    };

    struct rendererInfo
    {
        system::window* wnd;
        float clearColor[4];
    };

    enum class TextureFilter { Point, Linear };

    struct uvSplitInfo
    {
        uint pixelTexWidth;
        uint pixelTexHeight;
        uint pixelOffsetx;
        uint pixelOffsety;
        uint pixelFrameWidth;
        uint pixelFrameHeight;
        uint rowLength;
        uint frameCount;
    };

    struct renderer
    {
        system::window* window;
        IDXGISwapChain* swapChain;
        ID3D11RenderTargetView* backBuffer;
        ID3D11Device* device;
        ID3D11DeviceContext* context;
        ID3D11VertexShader* defaultVS;
        ID3D11VertexShader* defaultMeshVS;
        ID3D11VertexShader* currentVS;
        ID3D11PixelShader* defaultPS;
        ID3D11DepthStencilView* depthStencilView;
        ID3D11Texture2D* depthStencilBuffer;
        ID3D11InputLayout* inputLayout;
        ID3D11RasterizerState* wireframe;
        ID3D11RasterizerState* solid;
        ID3D11SamplerState* point;
        ID3D11SamplerState* linear;
        ID3D11Buffer* cbufferVS;
        ID3D11Buffer* cbufferPS;
        ID3D11Buffer* cbufferVScamera;
        ID3D11Buffer* world;
        ID3D11Buffer* view;
        ID3D11Buffer* transform;
        ID3D11Buffer* dynamicVertexBuffer;
        ID3D11BlendState* blendState;
        camera camera;
        camera3D* camera3Dptr;
        float backBufferColor[4];
        double frequency;
        long long startTime;
        long long prevFrameTime;
        double gameTime;
        double frameTime;
        bool fullscreen;
        /// <summary>
        /// if you render meshes and sprites then batch them together
        /// different constant buffers have to be set when mesh or sprite is rendered
        /// </summary>
        bool drawingSprites;

        void checkhr(HRESULT hr, int line)
        {
            if (hr == 0) return;
            char str[128];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0,
                hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                str, 128, 0);
            fprintf(stderr, str);
        }

        ID3D11SamplerState* createSampler(TextureFilter mode)
        {
            ID3D11SamplerState* sampler;
            D3D11_SAMPLER_DESC sampDesc;
            ZeroMemory(&sampDesc, sizeof(sampDesc));
            sampDesc.Filter = mode == TextureFilter::Point ?
                D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            sampDesc.MinLOD = 0;
            sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
            this->device->CreateSamplerState(&sampDesc, &sampler);
            return sampler;
        }

        ID3D11PixelShader* createPixelShaderFromString(const char* str,
            const char* entryPoint, const char* target)
        {
            ID3D11PixelShader* result;
            ID3D10Blob* ps;
            ID3D10Blob* errorMsg;
            HRESULT hr = D3DCompile(str, strlen(str), 0, 0, 0, entryPoint, target, 0, 0, &ps, &errorMsg);
            if (errorMsg)
            {
                void* ptr = errorMsg->GetBufferPointer();
                uint sz = errorMsg->GetBufferSize();
                byte buffer[1000];
                memset(buffer, 0, 1000);
                memcpy(buffer, ptr, sz);
                fprintf(stderr, "%s\n", buffer);
                return nullptr;
            }
            else
            {
                this->checkhr(hr, __LINE__);
            }
            //D3DCompile
            hr = this->device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), 0, &result);
            this->checkhr(hr, __LINE__);
            ps->Release();
            return result;
        }

        ID3D11VertexShader* createVertexShaderFromString(const char* str,
            const char* entryPoint, const char* target, bool setInputLayout)
        {
            ID3D11VertexShader* result = nullptr;
            ID3D10Blob* vs;
            ID3D10Blob* errorMsg;
            HRESULT hr = D3DCompile(str, strlen(str), 0, 0, 0, entryPoint, target, 0, 0, &vs, &errorMsg);

            if (errorMsg)
            {
                void* ptr = errorMsg->GetBufferPointer();
                uint sz = errorMsg->GetBufferSize();
                byte buffer[1000];
                memset(buffer, 0, 1000);
                memcpy(buffer, ptr, sz);
                fprintf(stderr, "%s\n", buffer);
                return nullptr;
            }
            else
            {
                this->checkhr(hr, __LINE__);
            }

            // have to do this only once
            if (setInputLayout)
            {
                //// VERTEX LAYOUT ////
            //The vertex Structure
            //The input-layout description
                D3D11_INPUT_ELEMENT_DESC layout[] =
                {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"LIGHT",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                hr = this->device->CreateInputLayout(layout, 3, vs->GetBufferPointer(),
                    vs->GetBufferSize(), &this->inputLayout);
                this->checkhr(hr, __LINE__);

                //Set the Input Layout
                this->context->IASetInputLayout(this->inputLayout);
            }

            hr = this->device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), 0, &result);
            this->checkhr(hr, __LINE__);
            vs->Release();
            return result;
        }

        void init(rendererInfo* info)
        {
            HRESULT hr = 0;
            this->drawingSprites = true;
            this->window = info->wnd;
            this->fullscreen = false;
            //assign global variable
            memcpy(this->backBufferColor, info->clearColor, sizeof(float) * 4);

            // camera
            this->camera3Dptr = nullptr;
            this->camera.aspectRatio = this->window->width / (float)this->window->height;
            this->camera.rotation = 0;
            this->camera.scale = 1;
            this->camera.x = 0;
            this->camera.y = 0;

            //// *********** PIPELINE SETUP STARTS HERE *********** ////
            // create a struct to hold information about the swap chain
            DXGI_SWAP_CHAIN_DESC scd;
            ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
            scd.BufferCount = 1;                                    // one back buffer
            scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
            scd.OutputWindow = info->wnd->handle;                   // the window to be used
            scd.SampleDesc.Quality = 0;
            scd.SampleDesc.Count = 1;                               // no anti aliasing
            scd.Windowed = TRUE;                                    // windowed/full-screen mode
            //scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // alternative fullscreen mode

            UINT creationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

            ////    DEVICE, DEVICE CONTEXT AND SWAP CHAIN    ////
            hr = D3D11CreateDeviceAndSwapChain(NULL,
                D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, NULL, NULL,
                D3D11_SDK_VERSION, &scd, &this->swapChain, &this->device, NULL,
                &this->context);
            this->checkhr(hr, __LINE__);

            ////    BACK BUFFER AS RENDER TARGET, DEPTH STENCIL   ////
            // get the address of the back buffer
            ID3D11Texture2D* buf;
            this->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&buf);
            // use the back buffer address to create the render target
            hr = this->device->CreateRenderTargetView(buf, NULL, &this->backBuffer);
            this->checkhr(hr, __LINE__);
            buf->Release();

            //Describe our Depth/Stencil Buffer and View
            D3D11_TEXTURE2D_DESC depthStencilDesc;
            depthStencilDesc.Width = info->wnd->width;
            depthStencilDesc.Height = info->wnd->height;
            depthStencilDesc.MipLevels = 1;
            depthStencilDesc.ArraySize = 1;
            depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;
            depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            depthStencilDesc.CPUAccessFlags = 0;
            depthStencilDesc.MiscFlags = 0;

            hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, &this->depthStencilBuffer);
            this->checkhr(hr, __LINE__);
            hr = this->device->CreateDepthStencilView(this->depthStencilBuffer, NULL, &this->depthStencilView);
            this->checkhr(hr, __LINE__);

            ////   VIEWPORT    ////
            // Set the viewport
            D3D11_VIEWPORT viewport;
            ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
            viewport.TopLeftX = 0;
            viewport.TopLeftY = 0;
            viewport.Width = (FLOAT)info->wnd->width;
            viewport.Height = (FLOAT)info->wnd->height;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            this->context->RSSetViewports(1, &viewport);

            ////    BLEND STATE  ////
            D3D11_BLEND_DESC blendDesc;
            ZeroMemory(&blendDesc, sizeof(blendDesc));
            D3D11_RENDER_TARGET_BLEND_DESC rtbd;
            ZeroMemory(&rtbd, sizeof(rtbd));
            rtbd.BlendEnable = true;
            rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
            rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            rtbd.BlendOp = D3D11_BLEND_OP_ADD;
            rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
            rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
            rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
            //blendDesc.AlphaToCoverageEnable = false;
            blendDesc.RenderTarget[0] = rtbd;
            hr = this->device->CreateBlendState(&blendDesc, &this->blendState);

            ////    VS and PS    ////
            this->defaultVS = this->createVertexShaderFromString(rc_VertexShader, "main", "vs_5_0", false);
            this->currentVS = this->defaultVS;
            this->defaultMeshVS = this->createVertexShaderFromString(rc_VertexShaderMesh, "main", "vs_5_0", true);
            this->defaultPS = this->createPixelShaderFromString(rc_PixelShader, "main", "ps_5_0");

            D3D11_BUFFER_DESC cbbd;
            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(sprite);
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->cbufferVS);

            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = psBufferSize;
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->cbufferPS);
            this->context->PSSetConstantBuffers(0, 1, &this->cbufferPS);

            // vertex buffer for camera also UpdateSubresource
            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(camera);
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->cbufferVScamera);

            // vs cbuffer for world view proj
            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = 64;
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->world);

            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(camera3D);
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->view);

            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(float) * 16;
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->transform);

            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DYNAMIC;
            cbbd.ByteWidth = sizeof(vertex) * 15000;
            cbbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            cbbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            this->device->CreateBuffer(&cbbd, NULL, &this->dynamicVertexBuffer);

            /*ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(camera);
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->cbufferVScamera);

            ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
            cbbd.Usage = D3D11_USAGE_DEFAULT;
            cbbd.ByteWidth = sizeof(camera);
            cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            this->device->CreateBuffer(&cbbd, NULL, &this->cbufferVScamera);*/

            D3D11_RASTERIZER_DESC rd;
            ZeroMemory(&rd, sizeof(rd));
            rd.FillMode = D3D11_FILL_WIREFRAME;
            rd.CullMode = D3D11_CULL_NONE;
            hr = this->device->CreateRasterizerState(&rd, &this->wireframe);
            this->checkhr(hr, __LINE__);
            rd.FillMode = D3D11_FILL_SOLID;
            rd.CullMode = D3D11_CULL_FRONT;
            hr = this->device->CreateRasterizerState(&rd, &this->solid);
            this->checkhr(hr, __LINE__);

            this->context->OMSetRenderTargets(1, &this->backBuffer, this->depthStencilView);
            this->context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            this->context->RSSetState(this->solid);
            this->context->PSSetShader(this->defaultPS, 0, 0);

            this->point = this->createSampler(TextureFilter::Point);
            this->linear = this->createSampler(TextureFilter::Linear);
            this->context->PSSetSamplers(0, 1, &this->point);

            // default is drawing sprites so set them
            this->context->VSSetShader(this->defaultVS, 0, 0);
            this->context->VSSetConstantBuffers(0, 1, &this->cbufferVS);
            this->context->VSSetConstantBuffers(1, 1, &this->cbufferVScamera);

            //// *********** PIPELINE SETUP ENDS HERE *********** ////
        }

        void destroy()
        {
            this->blendState->Release();
            this->blendState = nullptr;
            this->dynamicVertexBuffer->Release();
            this->dynamicVertexBuffer = nullptr;
            this->world->Release();
            this->world = nullptr;
            this->view->Release();
            this->view = nullptr;
            this->transform->Release();
            this->transform = nullptr;
            this->inputLayout->Release();
            this->inputLayout = nullptr;
            this->cbufferVS->Release();
            this->cbufferVS = nullptr;
            this->cbufferVScamera->Release();
            this->cbufferVScamera = nullptr;
            this->point->Release();
            this->point = nullptr;
            this->linear->Release();
            this->linear = nullptr;
            this->wireframe->Release();
            this->wireframe = nullptr;
            this->solid->Release();
            this->solid = nullptr;
            this->cbufferPS->Release();
            this->cbufferPS = nullptr;
            this->defaultPS->Release();
            this->defaultPS = nullptr;
            this->defaultVS->Release();
            this->defaultVS = nullptr;
            this->depthStencilView->Release();
            this->depthStencilView = nullptr;
            this->depthStencilBuffer->Release();
            this->depthStencilBuffer = nullptr;
            this->backBuffer->Release();
            this->backBuffer = nullptr;
            this->swapChain->Release();
            this->swapChain = nullptr;
            this->context->Release();
            this->context = nullptr;
            this->device->Release();
            this->device = nullptr;
        }

        // Create texture where pixels are uncompressed, not encoded, 4 bytes per pixel formatted RGBA, stored lineary.
        // insert texture to internal storage
        void createTextureFromBytes(texture* t, byte* data, uint width, uint height)
        {
            t->width = width;
            t->height = height;

            ID3D11Texture2D* tex = nullptr;
            D3D11_TEXTURE2D_DESC desc;
            D3D11_SUBRESOURCE_DATA sub;

            sub.pSysMem = (void*)data;
            sub.SysMemPitch = (UINT)width * 4;
            sub.SysMemSlicePitch = (UINT)height * (UINT)width * 4;

            desc.Width = (UINT)width;
            desc.Height = (UINT)height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;

            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            HRESULT hr = this->device->CreateTexture2D(&desc, &sub, &tex);
            this->checkhr(hr, __LINE__);

            D3D11_TEXTURE2D_DESC desc2;
            tex->GetDesc(&desc2);
            ID3D11ShaderResourceView* srv = nullptr;
            hr = this->device->CreateShaderResourceView(tex, 0, &srv);
            this->checkhr(hr, __LINE__);
            tex->Release();

            t->shaderResource = srv;
        }

        // Create texture from file in memory.
        // Difference between this and 'createTextureFromFile' is that file is in memory.
        // It's useful because you can have PNG or other encoded image in memory
        // and this can create texture from that. Supports lots of formats.
        void createTextureFromInMemoryFile(texture* t, byte* file, int len)
        {
            int x = -1, y = -1, n = -1;
            const int components = 4; // components means how many elements from 'RGBA'
            // you want to return, I want 4 (RGBA) even in not all 4 are present
            byte* data = stbi_load_from_memory(file, len, &x, &y, &n, components);

#ifdef VI_VALIDATE
            if (!data)
            {
                fprintf(stderr, "createTexture could not process in memory file\n");
                exit(1);
            }
#endif

            this->createTextureFromBytes(t, data, x, y);
            stbi_image_free(data);
        }

        // Create texture from file on disk. Supports lots of formats.
        void createTextureFromFile(texture* t, const char* filename)
        {
            int x = -1, y = -1, n = -1;
            const int components = 4; // components means how many elements from 'RGBA'
            // you want to return, I want 4 (RGBA) even in not all 4 are present
            byte* data = stbi_load(filename, &x, &y, &n, components);

#ifdef VI_VALIDATE
            if (!data)
            {
                fprintf(stderr, "createTexture could not open the file %s\n", filename);
                exit(1);
            }
#endif

            this->createTextureFromBytes(t, data, x, y);
            stbi_image_free(data);
        }

        void destroyTexture(texture* t)
        {
            t->shaderResource->Release();
            t->shaderResource = nullptr;
        }

        /// <summary>
        /// this can be used to start a new layer
        /// </summary>
        void clearDepth()
        {
            this->context->ClearDepthStencilView(this->depthStencilView,
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }

        void beginScene()
        {
            this->context->ClearRenderTargetView(this->backBuffer, this->backBufferColor);
            this->context->ClearDepthStencilView(this->depthStencilView,
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            // update camera only once per frame
            this->context->UpdateSubresource(this->cbufferVScamera, 0, NULL, &this->camera, 0, 0);
            if (this->camera3Dptr)
                this->context->UpdateSubresource(this->view, 0, NULL, this->camera3Dptr, 0, 0);
        }

        void updateCamera(gl::camera* c)
        {
            this->context->UpdateSubresource(this->cbufferVScamera, 0, NULL, c, 0, 0);
        }

        void drawSprite(sprite* s)
        {
            if (s->s1.nodraw) return;

            if (!this->drawingSprites)
            {
                this->drawingSprites = true;
                this->context->VSSetShader(this->currentVS, 0, 0);
                this->context->VSSetConstantBuffers(0, 1, &this->cbufferVS);
                this->context->VSSetConstantBuffers(1, 1, &this->cbufferVScamera);
            }

            if (s->s1.t) this->context->PSSetShaderResources(0, 1, &s->s1.t->shaderResource);
            s->s1.notexture = !s->s1.t;
            this->context->UpdateSubresource(this->cbufferVS, 0, NULL, s, 0, 0);
            this->context->UpdateSubresource(this->cbufferPS, 0, 0, &s->s2.flags, 0, 0);            
            this->context->Draw(6, 0);            
        }

        /// <summary>
        /// use 'line' component of 'sprite';
        /// only static line at this point;
        /// line cannot be scaled, moved or rotated;
        /// position of end points still exists in world space so it's affected by camera scale and pos
        /// REMEMBER TO SET WIREFRAME
        /// </summary>
        void drawLine(sprite* s)
        {
            if (!this->drawingSprites)
            {
                this->drawingSprites = true;
                this->context->VSSetShader(this->currentVS, 0, 0);
                this->context->VSSetConstantBuffers(0, 1, &this->cbufferVS);
                this->context->VSSetConstantBuffers(1, 1, &this->cbufferVScamera);
            }

            // 4 uints because min size is 16 bytes
            uint flags[4] = { 2 }; // notexture flag on
            s->s1.z -= 1.0f;
            this->context->UpdateSubresource(this->cbufferVS, 0, NULL, s, 0, 0);
            s->s1.z += 1.0f;
            this->context->UpdateSubresource(this->cbufferPS, 0, 0, &flags, 0, 0);

            this->context->Draw(3, 0);
        }

        void drawMesh(mesh* m, float* transform = nullptr)
        {
            UINT stride = sizeof(vertex);
            UINT offset = 0;
            this->context->IASetVertexBuffers(0, 1, &m->vertexBuffer, &stride, &offset);

            if (this->drawingSprites)
            {
                this->drawingSprites = false;
                this->context->VSSetShader(this->defaultMeshVS, 0, 0);
                this->context->VSSetConstantBuffers(0, 1, &this->world);
                this->context->VSSetConstantBuffers(1, 1, &this->view);
                this->context->VSSetConstantBuffers(2, 1, &this->transform);
            }

            if (m->t)
                this->context->PSSetShaderResources(0, 1, &m->t->shaderResource);

            this->context->UpdateSubresource(this->world, 0, NULL, &m->pos, 0, 0);

            if (transform)
            {
                m->data |= APPLY_TRANSFORM;
                this->context->UpdateSubresource(this->transform, 0, NULL, transform, 0, 0);
            }

            if (m->indexBuffer)
            {
                this->context->IASetIndexBuffer(m->indexBuffer, DXGI_FORMAT_R32_UINT, 0);
                this->context->DrawIndexed(m->indexCount, 0, 0);
            }
            else
            {
                this->context->Draw(m->vertexCount, 0);
            }
        }

        /// <summary>
        /// updates vertex data every call;
        /// will use global dynamic vertex buffer;
        /// won't use mesh's vertex buffer or index buffer;
        /// slower than constant one;
        /// no transform applied at the moment so pos xy must be in screen coordinates
        /// </summary>
        void drawMeshDynamic(mesh* m, uint vertexCount)
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
            //  Disable GPU access to the vertex buffer data.
            this->context->Map(this->dynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            //  Update the vertex buffer here.
            memcpy(mappedResource.pData, m->v, sizeof(vertex) * vertexCount);
            //  Reenable GPU access to the vertex buffer data.
            this->context->Unmap(this->dynamicVertexBuffer, 0);

            if (this->drawingSprites)
            {
                this->drawingSprites = false;
                this->context->VSSetShader(this->defaultMeshVS, 0, 0);
                this->context->VSSetConstantBuffers(0, 1, &this->world);
            }

            if (m->t)
                this->context->PSSetShaderResources(0, 1, &m->t->shaderResource);

            m->data |= 8;
            this->context->UpdateSubresource(this->world, 0, NULL, &m->pos, 0, 0);

            UINT stride = sizeof(vertex);
            UINT offset = 0;
            this->context->IASetVertexBuffers(0, 1, &this->dynamicVertexBuffer, &stride, &offset);
            this->context->Draw(vertexCount, 0);
        }

        void endScene()
        {
            this->swapChain->Present(0, 0);
        }

        // utility function to calculate uv
        // offset is where on texture to start
        // width and height are frame size
        // row length: in case frames on texture are stacked in multiple rows, how many frames per row are there
        // frame count is how many uv elements to calculate
        // uv is the destination, there must be at least as many uv elements avaiable as 'frameCount'
        void uvSplit(uvSplitInfo* info, uv* uv)
        {
            const float width = (float)info->pixelFrameWidth / info->pixelTexWidth;
            const float height = (float)info->pixelFrameHeight / info->pixelTexHeight;
            float offsetx = (float)info->pixelOffsetx / info->pixelTexWidth;
            float x = offsetx;
            float y = (float)info->pixelOffsety / info->pixelTexHeight;

            for (uint i = 0; i < info->frameCount; i++, uv++)
            {
                uv->left = x;
                uv->top = y;
                uv->right = x + width;
                uv->bottom = y + height;

                x += width;

                if (x >= width * info->rowLength + offsetx)
                {
                    x = offsetx;
                    y += height;
                }
            }
        }

        // 'pixelWidth' and 'pixelHeight' are dimensions in pixel
        void setPixelScale(sprite* s, uint pixelWidth, uint pixelHeight)
        {
            s->s1.sx = 2.0f / this->window->width / this->camera.scale * pixelWidth * this->camera.aspectRatio;
            s->s1.sy = 2.0f / this->window->height / this->camera.scale * pixelHeight;
        }

        // 'pixelx' and 'pixely' are window coordinates in pixels (0,0) is in upper left corner
        void setScreenPos(sprite* s, uint pixelx, uint pixely)
        {
            s->s1.x = 2.0f / this->window->width * (pixelx - this->window->width / 2.0f) / this->camera.scale * this->camera.aspectRatio;
            s->s1.y = 2.0f / this->window->height * (pixely - this->window->height / 2.0f) / this->camera.scale;
        }

        // 'pixelWidth' and 'pixelHeight' are dimensions in pixel, camera independent
        void setPixelScale2(sprite* s, uint pixelWidth, uint pixelHeight)
        {
            s->s1.sx = 2.0f / this->window->width * pixelWidth * this->window->width / (float)this->window->height;
            s->s1.sy = 2.0f / this->window->height * pixelHeight;
        }

        // 'pixelx' and 'pixely' are window coordinates in pixels (0,0) is in upper left corner, camera independent
        void setScreenPos2(sprite* s, uint pixelx, uint pixely)
        {
            s->s1.x = 2.0f / this->window->width * (pixelx - this->window->width / 2.0f) * this->window->width / (float)this->window->height;
            s->s1.y = 2.0f / this->window->height * (pixely - this->window->height / 2.0f);
        }

        // puts width and height in world coordinates of 1px in f[0] and f[1]
        void getPixelScale(float* f)
        {
            f[0] = 2.0f / this->window->width / this->camera.scale * this->camera.aspectRatio;
            f[1] = 2.0f / this->window->height / this->camera.scale;
        }

        // utli function that will calc uv coords if you know pixel coords
        void setUvFromPixels(sprite* s, float pixelOffsetX, float pixelOffsetY, float pixelWidth,
            float pixelHeight, float pixelTextureWidth, float pixelTextureHeight)
        {
            s->s1.left = pixelOffsetX / pixelTextureWidth;
            s->s1.top = pixelOffsetY / pixelTextureHeight;
            s->s1.right = s->s1.left + pixelWidth / pixelTextureWidth;
            s->s1.bottom = s->s1.top + pixelHeight / pixelTextureHeight;
        }

        /// <summary>
        /// index is not required;
        /// mesh will be drawn as triangle list so index count should be multiple of 3
        /// </summary>
        void initMesh(mesh* m, vertex* v, uint vertexCount, uint* index, uint indexCount, texture* t)
        {
            util::zero(m);
            m->index = index;
            m->indexCount = indexCount;
            m->vertexCount = vertexCount;
            m->v = v;
            m->sca = { 1,1,1 };
            m->color = { 1,1,1 };
            m->t = t;

            D3D11_BUFFER_DESC vertexBufferDesc = {};

            vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            vertexBufferDesc.ByteWidth = sizeof(vertex) * m->vertexCount;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vertexBufferData = {};
            vertexBufferData.pSysMem = m->v;

            HRESULT hr = this->device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m->vertexBuffer);
            checkhr(hr, __LINE__);

            D3D11_BUFFER_DESC indexBufferDesc = {};

            indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            indexBufferDesc.ByteWidth = sizeof(uint) * m->indexCount;
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            if (m->index)
            {
                D3D11_SUBRESOURCE_DATA iinitData = {};
                iinitData.pSysMem = m->index;

                hr = this->device->CreateBuffer(&indexBufferDesc, &iinitData, &m->indexBuffer);
                checkhr(hr, __LINE__);
            }
        }

        void setWireframe()
        {
            this->context->RSSetState(this->wireframe);
        }

        void setSolid()
        {
            this->context->RSSetState(this->solid);
        }

        void destroyMesh(mesh* m)
        {
            if (m->indexBuffer)
            {
                m->indexBuffer->Release();
                m->indexBuffer = nullptr;
            }
            m->vertexBuffer->Release();
            m->vertexBuffer = nullptr;
        }

        void enableBlendState()
        {
            float blendFactor[] = { 0, 0, 0, 0 };
            this->context->OMSetBlendState(this->blendState, blendFactor, 0xffffffff);
        }

        void disableBlendState()
        {
            this->context->OMSetBlendState(0, 0, 0xffffffff);
        }

        void setDefaultSpriteVS()
        {
            this->currentVS = this->defaultVS;
            this->context->VSSetShader(this->currentVS, 0, 0);
        }

        void setSpriteVS(ID3D11VertexShader* vs)
        {
            this->currentVS = vs;
            this->context->VSSetShader(this->currentVS, 0, 0);
        }

        ID3D11VertexShader* createVertexShader(const char* str)
        {
            return this->createVertexShaderFromString(str, "main", "vs_5_0", false);
        }

        void destroyVertexShader(ID3D11VertexShader* vs)
        {
            vs->Release();
        }
    };
}

namespace vi::input
{
    // for letters and numbers use 'A' - 'Z', '0' - '9' etc
    //// this doesnt have to be enum class because numbers are allowed from outside of this set
    enum key : int
    {
        LMOUSE = VK_LBUTTON,
        RMOUSE = VK_RBUTTON,
        MMOUSE = VK_MBUTTON,

        LEFT = VK_LEFT,
        RIGHT = VK_RIGHT,
        UP = VK_UP,
        DOWN = VK_DOWN,

        INSERT = VK_INSERT,
        DEL = VK_DELETE,
        HOME = VK_HOME,
        END = VK_END,
        PAGEUP = VK_PRIOR,
        PAGEDOWN = VK_NEXT,
        SCROLLLOCK = VK_SCROLL,
        PRNT_SCRN = VK_SNAPSHOT,

        TAB = VK_TAB,
        CAPSLOCK = VK_CAPITAL,
        LSHIFT = VK_LSHIFT,
        LALT = VK_LMENU,
        LCONTROL = VK_LCONTROL,
        LWIN = VK_LWIN,

        SPACE = VK_SPACE,

        BACKSPACE = VK_BACK,
        ENTER = VK_RETURN,
        RSHIFT = VK_RSHIFT,
        RCONTROL = VK_RCONTROL,
        RALT = VK_RMENU,
        RWIN = VK_RWIN,
        MENU = VK_APPS,

        ESCAPE = VK_ESCAPE,
        F1 = VK_F1,
        F2 = VK_F2,
        F3 = VK_F3,
        F4 = VK_F4,
        F5 = VK_F5,
        F6 = VK_F6,
        F7 = VK_F7,
        F8 = VK_F8,
        F9 = VK_F9,
        F10 = VK_F10,
        F11 = VK_F11,
        F12 = VK_F12,

        NUMLOCK = VK_NUMLOCK,
        NUM0 = VK_NUMPAD0,
        NUM1 = VK_NUMPAD1,
        NUM2 = VK_NUMPAD2,
        NUM3 = VK_NUMPAD3,
        NUM4 = VK_NUMPAD4,
        NUM5 = VK_NUMPAD5,
        NUM6 = VK_NUMPAD6,
        NUM7 = VK_NUMPAD7,
        NUM8 = VK_NUMPAD8,
        NUM9 = VK_NUMPAD9,
        NUMDIV = VK_DIVIDE,
        NUMMUL = VK_MULTIPLY,
        NUMMINUS = VK_SUBTRACT,
        MULPLUS = VK_ADD,
        NUMDEL = VK_DECIMAL,

        MINUS = VK_OEM_MINUS,
        EQUALS = VK_OEM_PLUS,
        BRACKETOPEN = VK_OEM_4,
        BRACKETCLOSE = VK_OEM_6,
        PIPE = VK_OEM_5,
        SEMICOLON = VK_OEM_1,
        QUOTE = VK_OEM_7,
        COMMA = VK_OEM_COMMA,
        PERIOD = VK_OEM_PERIOD,
        SLASH = VK_OEM_2,
        TILD = VK_OEM_3
    };

    struct keyboard
    {
        bool buf1[KEYBOARD_KEY_COUNT];
        bool buf2[KEYBOARD_KEY_COUNT];
        char typemaplower[KEYBOARD_KEY_COUNT];
        char typemapupper[KEYBOARD_KEY_COUNT];
        bool* curState;
        bool* prevState;
        char typedKey;

        void init()
        {
            this->typedKey = 0;
            this->curState = this->buf1;
            this->prevState = this->buf2;
            memset(this->buf1, 0, KEYBOARD_KEY_COUNT);
            memset(this->buf2, 0, KEYBOARD_KEY_COUNT);
            memset(this->typemaplower, 0, KEYBOARD_KEY_COUNT);
            memset(this->typemapupper, 0, KEYBOARD_KEY_COUNT);

            this->typemaplower[9] = '\t';
            this->typemaplower[32] = ' ';
            memcpy(this->typemaplower + 48, "0123456789", 10);
            memcpy(this->typemaplower + 65, "abcdefghijklmnopqrstuvwxyz", 26);
            memcpy(this->typemaplower + 186, ";=,-./`", 7);
            memcpy(this->typemaplower + 219, "[\\]'", 4);
            memcpy(this->typemaplower + 96, "0123456789*+'", 13);
            this->typemaplower[109] = '-';
            this->typemaplower[111] = '/';

            this->typemapupper[9] = '\t';
            this->typemapupper[32] = ' ';
            memcpy(this->typemapupper + 48, ")!@#$%^&*(", 10);
            memcpy(this->typemapupper + 65, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
            memcpy(this->typemapupper + 186, ":+<_>?~", 7);
            memcpy(this->typemapupper + 219, "{|}\"", 4);
            memcpy(this->typemapupper + 96, "0123456789*+'", 13);
            this->typemapupper[109] = '-';
            this->typemapupper[111] = '/';
        }

        void update()
        {
            this->typedKey = 0;
            // swap states
            vi::util::swap(this->curState, this->prevState);

            // get shift first
            bool isUpper = (::GetAsyncKeyState(16) & 0x8000) && true;
            // get button states
            for (int i = 0; i < KEYBOARD_KEY_COUNT; i++)
            {
                this->curState[i] = (::GetAsyncKeyState(i) & 0x8000) && true;
                if (this->typemapupper[i] != 0 && !this->prevState[i] && this->curState[i] && isUpper)
                    this->typedKey = this->typemapupper[i];
                else if (this->typemaplower[i] != 0 && !this->prevState[i] && this->curState[i] && !isUpper)
                    this->typedKey = this->typemaplower[i];
            }
        }

        bool isKeyDown(int _key)
        {
            return this->curState[_key];
        }

        bool isKeyPressed(int _key)
        {
            // TODO, return false if this is the first frame
            //return k->curState[(int)_key] && !k->prevState[(int)_key] && engine->GetFrame() != 0;
            return this->curState[_key] && !this->prevState[_key];
        }

        bool isKeyReleased(int _key)
        {
            return !this->curState[_key] && this->prevState[_key];
        }

        char getKeyTyped()
        {

        }
    };

    struct mouse
    {
        int _cursorScreenx;
        int _cursorScreeny;
        int _cursorClientx;
        int _cursorClienty;
        float _cursorWorldx;
        float _cursorWorldy;
        int _cursorDeltax;
        int _cursorDeltay;
        short _wheel;
        int _rawMouseDeltax;
        int _rawMouseDeltay;

        void init()
        {
            util::zero(this);
        }

        /// <summary>
        /// can pass null for camera but you wont get 2D world x and y
        /// </summary>
        void update(system::window* w, gl::camera* c)
        {
            POINT p;
            GetCursorPos(&p);
            this->_cursorDeltax = p.x - this->_cursorScreenx;
            this->_cursorDeltay = p.y - this->_cursorScreeny;
            this->_cursorScreenx = p.x;
            this->_cursorScreeny = p.y;
            this->_wheel = system::wheelDelta;
            this->_rawMouseDeltax = system::rawMouseDeltax;
            this->_rawMouseDeltay = system::rawMouseDeltay;

            p = { this->_cursorScreenx, this->_cursorScreeny };
            ScreenToClient(w->handle, &p);
            this->_cursorClientx = p.x;
            this->_cursorClienty = p.y;

            if (c)
            {
                this->_cursorWorldx = ((float)this->_cursorClientx - w->width / 2) / w->width / c->scale * c->aspectRatio * 2 + c->x;
                this->_cursorWorldy = ((float)this->_cursorClienty - w->height / 2) / w->height / c->scale * 2 + c->y;
            }
        }

        // this is relative to monitor's upper left corner
        void getCursorScreenPos(int* x, int* y)
        {
            *x = this->_cursorScreenx;
            *y = this->_cursorScreeny;
        }

        // this is relative to client's upper left corner 
        void getCursorClientPos(int* x, int* y)
        {
            *x = this->_cursorClientx;
            *y = this->_cursorClienty;
        }

        void getCursorWorldPos(float* x, float* y)
        {
            *x = this->_cursorWorldx;
            *y = this->_cursorWorldy;
        }

        void getCursorScreenDelta(int* x, int* y)
        {
            *x = this->_cursorDeltax;
            *y = this->_cursorDeltay;
        }

        void getCursorDeltaRaw(int* x, int* y)
        {
            *x = this->_rawMouseDeltax;
            *y = this->_rawMouseDeltay;
        }

        /// <summary>
        /// Is cursor position different that last update
        /// </summary>
        bool moved()
        {
            return (this->_cursorDeltax + this->_cursorDeltay) != 0;
        }

        short getWheelDelta()
        {
            return this->_wheel;
        }
    };
}

namespace vi::net
{
    uint uid = 1;
    WSAData wsadata;
    char lastWinsockError[300];

    void _getLastWinsockErrorMessage(DWORD errorCode)
    {
        ZeroMemory(lastWinsockError, 300);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, errorCode,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), lastWinsockError, 300, 0);
    }

    void initNetwork()
    {
        int res = WSAStartup(MAKEWORD(2, 2), &wsadata);

#ifdef VI_VALIDATE
        if (res != 0)
        {
            _getLastWinsockErrorMessage(res);
            fprintf(stderr, lastWinsockError);
        }
#endif
    }

    void uninitNetwork()
    {
        int res = WSACleanup();
#ifdef VI_VALIDATE
        if (res == SOCKET_ERROR)
        {
            _getLastWinsockErrorMessage(WSAGetLastError());
            fprintf(stderr, lastWinsockError);
        }
#endif
    }

    struct endpoint
    {
        sockaddr_in address;
        float lastMessage;
        bool isConnected;

        void getAddress(char* dst, uint maxSize)
        {
            inet_ntop(AF_INET, &this->address.sin_addr, dst, maxSize);
        }
    };

    struct server
    {
        SOCKET s;
        sockaddr_in address;
        unsigned short port;
        uint id;

        void init(ushort port)
        {
            int result = 0;

            this->port = port;
            this->id = uid++;
            this->s = ::socket(AF_INET, SOCK_DGRAM, 0);

#ifdef VI_VALIDATE
            if (this->s == INVALID_SOCKET)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                return;
            }
#endif

            this->address.sin_port = htons(port);
            this->address.sin_family = AF_INET;
            this->address.sin_addr.s_addr = htonl(INADDR_ANY);
            result = bind(this->s, (sockaddr*)&this->address, (int)sizeof(sockaddr));

#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                return;
            }
#endif        

            // set non blocking
            ULONG mode = 1;
            result = ioctlsocket(this->s, FIONBIO, &mode);
#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                return;
            }
#endif
        }

        void send(const byte* data, uint len, endpoint* ep)
        {
            int result = sendto(this->s, (const char*)data, len, 0, (const sockaddr*)&ep->address, (int)sizeof(sockaddr));
#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
            }
#endif
        }

        /// <summary>
        /// Returns true if message received. False otherwise.
        /// It's false if function was called but there was nothing to receive.
        /// </summary>
        bool receive(byte* data, uint limit, endpoint* ep)
        {
            sockaddr_in address;
            int len = sizeof(sockaddr_in);
            int result = recvfrom(this->s, (char*)data, limit, 0, (sockaddr*)&address, &len);
#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                exit(0);
            }
#endif
            memcpy(&ep->address, &address, sizeof(sockaddr_in));
            ep->isConnected = true;
            return ep->isConnected;
        }

        void destroyServer()
        {
        }
    };

    struct client
    {
        SOCKET s;
        sockaddr_in serverAddress;
        unsigned short serverPort;
        uint id;

        void init(const char* address, uint port)
        {
            vi::util::zero<client>(this);
            this->serverPort = port;
            this->id = uid++;
            this->s = ::socket(AF_INET, SOCK_DGRAM, 0);
#ifdef VI_VALIDATE
            if (this->s == INVALID_SOCKET)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                return;
            }
#endif
            this->serverAddress.sin_port = htons(port);
            this->serverAddress.sin_family = AF_INET;
            inet_pton(AF_INET, address, &this->serverAddress.sin_addr);
            // connect is done so you dont have to pass address in sendto and recvfrom
            // client communicates with only one address
            int res = connect(this->s, (sockaddr*)&this->serverAddress, sizeof(sockaddr));
#ifdef VI_VALIDATE
            if (res == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                return;
            }
#endif

            // set non blocking
            ULONG mode = 1;
            res = ioctlsocket(this->s, FIONBIO, &mode);
#ifdef VI_VALIDATE
            if (res == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
            }
#endif
        }

        void send(byte* data, uint len)
        {
            int result = ::send(this->s, (const char*)data, len, 0);
#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                exit(0);
            }
#endif
        }

        void receive(byte* data, uint limit)
        {
            int len = sizeof(sockaddr_in);
            int result = recv(this->s, (char*)data, limit, 0);
#ifdef VI_VALIDATE
            if (result == SOCKET_ERROR)
            {
                _getLastWinsockErrorMessage(WSAGetLastError());
                fprintf(stderr, lastWinsockError);
                exit(0);
            }
#endif
        }

        void destroyClient()
        {
        }
    };

    bool compareEndpoints(const endpoint* a, const endpoint* b)
    {
        return memcmp(&a->address, &b->address, 8) == 0;
    }
}

namespace vi::fn
{
    struct routine
    {
        std::function<int()> fn;
        float timeout;
        float interval;
        float duration;
        float lastUpdate;
        float started;
        uint id;
        bool destroy;
    };

    struct queue
    {
        time::timer* t;
        uint idNext;

        void init(time::timer* t)
        {
            this->t = t;
            this->idNext = 0;
        }

        // duration == 0 means run forever
        void initRoutine(routine* r, std::function<int()> fn, float timeout, float interval, float duration)
        {
            util::zero<routine>(r);
            r->duration = duration;
            r->fn = fn;
            r->interval = interval;
            r->started = this->t->getGameTimeSec();
            r->lastUpdate = this->t->getGameTimeSec();
            r->timeout = timeout;
            r->id = this->idNext;
            this->idNext++;
        }

        void setTimeout(routine* r, std::function<int()> fn, float timeout)
        {
            this->initRoutine(r, fn, timeout, 0, 0);
        }

        void setInterval(routine* r, std::function<int()> fn, float interval)
        {
            this->initRoutine(r, fn, 0, interval, 0);
        }

        void setDuration(routine* r, std::function<int()> fn, float duration)
        {
            this->initRoutine(r, fn, 0, 0, duration);
        }

        void update(routine* routines, uint count)
        {
            float gameTime = this->t->getGameTimeSec();

            for (int i = count - 1; i >= 0; i--)
            {
                routine* r = routines + i;

                // check if timeout and interval elapsed
                if (!r->destroy &&
                    gameTime - r->started > r->timeout &&
                    gameTime - r->lastUpdate > r->interval &&
                    gameTime - r->started < r->duration)
                {
                    int val = r->fn();
                    r->lastUpdate = gameTime;

                    if (val == 0) r->destroy = true;
                }
                else
                {
                    r->destroy = false;
                }
            }
        }
    };
}

#ifndef VIVA_IMPL
// here should be prototypes and declarations only for compiler
#endif