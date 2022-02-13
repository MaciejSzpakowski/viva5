#pragma once

// Viva engine by Maciej Szpakowski
// IMPORTANT, can compile only in x64
//#define VIVA_IMPL to use the code, otherwise only prototypes and declarations will be used

// #define VI_VULKAN_H for where vulkan.h file is
// #define VI_VULKAN_LIB for where vulkan.lib is
#ifndef VI_VULKAN_H
#define VI_VULKAN_H <vulkan.h>
#endif

#ifndef VI_VULKAN_LIB
#define VI_VULKAN_LIB "vulkan.lib"
#endif

// uses '#pragma comment(lib' to use vulkan.lib for linking

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

#ifdef VIVA_IMPL

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

#undef min
#undef max

#define _max(a,b)            (((a) > (b)) ? (a) : (b))

#define _min(a,b)            (((a) < (b)) ? (a) : (b))

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

            if(this->track) this->allocations.push_back(block);

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
            for(uint i=0;i<this->allocations.size();i++) fprintf(stderr, "Not freed: %p\n", this->allocations[i]);
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

	struct rng
	{
		std::mt19937 mt;
		uint min;
		uint max;

		rng():mt(::time(0)),min(mt.min()),max(mt.max())
		{		
#ifdef VIDBG2
			if (this->min != 0)
			{
				fprintf(stderr, "rng min is not 0");
				exit(1);
			}
#endif
		}

        // returns a random float. range: [0.0, 1.0)
        float rnd()
        {
            return (float)this->mt() / (float)this->max;
        }

        int rndInt(int min, int max)
        {
            return this->mt() % (max - min) + min;
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

    template<uint sz>
    struct stream
    {
        byte data[sz];
        uint capacity;
        uint len;

        stream()
        {
            this->capacity = sz;
            this->len = 0;
        }

        void push(const byte* data, uint len)
        {
#ifdef VI_VALIDATE
            if (this->len + len > this->capacity)
            {
                fprintf(stderr, "stream overflow");
                return;
            }
#endif
            memcpy(this->data + this->len, data, len);
            this->len += len;
        }

        void push(const char* data, uint len)
        {
#ifdef VI_VALIDATE
            if (this->len + len > this->capacity)
            {
                fprintf(stderr, "stream overflow");
                return;
            }
#endif
            memcpy(this->data + this->len, data, len);
            this->len += len;
        }

        void push(uint data)
        {
#ifdef VI_VALIDATE
            if (this->len + 4 > this->capacity)
            {
                fprintf(stderr, "stream overflow");
                return;
            }
#endif
            memcpy(this->data + len, &data, 4);
            this->len += 4;
        }

        void clear()
        {
            this->len = 0;
        }
    };
//
//    template<typename T>
//    struct list
//    {
//        std::vector<T> v;
//
//        void push(T& e)
//        {
//            this->v.push_back(e);
//        }
//
//        void removeAt(uint i)
//        {
//#ifdef VI_VALIDATE
//            if (i < 0 || i >= this->v.size())
//            {
//                fprintf(stderr, "out of bounds\n");
//                return;
//            }
//#endif // VI_VALIDATE
//            if (this->v.size() == 1 || i == this->v.size() - 1)
//            {
//                this->v.pop_back();
//            }
//            else
//            {
//                swap(this->v[i], this->v[this->v.size() - 1]);
//                this->v.pop_back();
//            }
//        }
//
//        uint size()
//        {
//            return this->v.size();
//        }
//
//        T& at(uint i)
//        {
//#ifdef VI_VALIDATE
//            if (i < 0 || i >= this->v.size())
//            {
//                fprintf(stderr, "out of bounds\n");
//                return;
//            }
//#endif // VI_VALIDATE
//            return this->v[i];
//        }
//
//        T& find(std::function<bool(T&)> pred)
//        {
//            for (uint i = 0; i < this->v.size(); i++)
//            {
//                if (pred(this->v[i])) return this->v[i];
//            }
//            return nullptr;
//        }
//
//        T* data()
//        {
//            return this->v.data();
//        }
//
//        void resize(uint len)
//        {
//            this->v.resize(len);
//        }
//    };
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
    int rawMouseDeltay = 0;
    bool quitMessagePosted = false;
    byte* readFile(const char* filename, vi::memory::alloctrack* a, size_t* outSize)
    {
        FILE* file = fopen(filename, "rb");

#ifdef VI_VALIDATE
        if (!file)
        {
            fprintf(stderr, "Could not open %s\n", filename);
            exit(1);
        }
#endif

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

        if (outSize != nullptr)
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
    };

    void initWindow(windowInfo* info, window* w)
    {
        w->width = info->width;
        w->height = info->height;
        w->hinstance = GetModuleHandle(0);
        HBRUSH bg = CreateSolidBrush(RGB(255, 0, 0));

        WNDCLASS wc = { };
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = w->hinstance;
        wc.lpszClassName = WND_CLASSNAME;
        wc.hbrBackground = bg;
        RegisterClass(&wc);

        DWORD wndStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
        RECT r = { 0, 0, (LONG)info->width, (LONG)info->height };
        // this tells you what should be the window size if r is rect for client
        // IMPORTANT. window client, swap chain and VkImages (render target) dimensions must match
        AdjustWindowRect(&r, wndStyle, false);
        w->handle = CreateWindowEx(0, WND_CLASSNAME, info->title, wndStyle, 100, 100,
            r.right - r.left, r.bottom - r.top, 0, 0, w->hinstance, 0);
#ifdef VI_VALIDATE
        if (w->handle == nullptr)
        {
            fprintf(stderr, "Failed to create window");
            exit(1);
        }
#endif
        ShowWindow(w->handle, SW_SHOW);

        USHORT HID_USAGE_PAGE_GENERIC = 1;
        USHORT HID_USAGE_GENERIC_MOUSE = 2;

        RAWINPUTDEVICE Rid;
        Rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
        Rid.usUsage = HID_USAGE_GENERIC_MOUSE;
        Rid.dwFlags = RIDEV_INPUTSINK;
        Rid.hwndTarget = w->handle;
        RegisterRawInputDevices(&Rid, 1, sizeof(RAWINPUTDEVICE));
    }

    void destroyWindow(window* w)
    {
        DestroyWindow(w->handle);
        UnregisterClass(WND_CLASSNAME, w->hinstance);
    }

    bool updateWindow(window* w)
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
}

// d3d11
namespace vi::gl
{
    const uint TEXTURE_INVISIBLE = 999999;
    const uint TEXTURE_BLANK = 999998;

    const char rc_PixelShader[] = R"(
Texture2D textures[1];
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    if(input.Col.a == 0.0f)
    {
        discard;
        return float4(0,0,0,0);
    }
    else if(input.Col.a == 0.5f)
    {
        return float4(input.Col.rgb, 1);
    }
    else
    {
		float4 result = textures[0].Sample(ObjSamplerState, input.TexCoord);
        // discard if alpha is less than 1
        clip(result.a - 1.0f);
		return result * input.Col;
    }
}
)";

    const char rc_PostProcessing[] = R"(
Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    if(input.Col.a < 0.01f)
    {
        discard;
        return float4(0,0,0,0);
    }
    else
    {
	    float4 result = ObjTexture.Sample(ObjSamplerState, input.TexCoord);
	    return result;
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
    float r,g,b;
    uint textureIndex;
    bool fixed;
};

struct camera
{
	float aspectRatio;
	float x;
	float y;
	float rotation;
	float scale;
};

cbuffer jedziemy
{
	sprite spr;
};

cbuffer poziomo
{
	camera camObj;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Col : COLOR;
	float2 TexCoord : TEXCOORD;
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
    // fixed means ignore camera except for aspect ratio
    //if(spr.fixed)
    //{
    //   cam = mat4(
	//	1/ubo.cam.aspectRatio, 0, 0, 0,
	//	0, 1, 0, 0,
	//	0, 0, 1, 0,
	//	0, 0, 0, 1
	//);
    //}
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
	output.Col = float4(spr.r, spr.g, spr.b, 1);
    if(spr.textureIndex == 999999) output.Col.a = 0.0f;
    else if(spr.textureIndex == 999998) output.Col.a = 0.5f;
    int u = uv[vid].x;
    int v = uv[vid].y;
    output.TexCoord = float2(spr.uv[u],spr.uv[v]);

	return output;
}
)";

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

    struct renderTarget
    {
        ID3D11Texture2D* texture;
        ID3D11RenderTargetView* rtv;
    };    

    struct vector3
    {
        float x, y, z;
    };

    struct vector2
    {
        float x, y;
    };

    struct color
    {
        float r, g, b;
    };

    struct uv
    {
        float left, top, right, bottom;
    };

    // WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // you updating something ? update shaders as well
    // struct passed to uniform buffer must be multiple of 16bytes
    struct sprite1
    {
        // pos x
        float x;
        // pos y
        float y;
        // pos z
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
        // color red
        float r;
        // color green
        float g;
        // color blue
        float b;

        uint textureIndex;

        // move with camera or fixed to viewport
        bool fixed;
        // padding because this truct must be multiple of 16bytes
        byte padding[15];
    };

    struct sprite2
    {
        vector3 pos;
        vector2 scale;
        float rot;
        vector2 origin;
        uv uv1;
        color col;
        uint textureIndex;
    };

    union sprite
    {
        sprite1 s1;
        sprite2 s2;
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
    };

    struct texture
    {
        int index;
        int width;
        int height;
        ID3D11ShaderResourceView* shaderResource;
    };

    // for now speed must be non negative
    struct animation
    {
        sprite* s;
        time::timer* t;
        uv* uv;
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
    };

    struct font
    {
        texture* tex;
        uv uv[256];

        void init(texture* t)
        {
            this->tex = t;
        }
    };

    struct text
    {
        font* f;
        sprite* s;
        const char* str;
        float horizontalSpace;
        float verticalSpace;
        sprite* last;

        void update()
        {
            const char* it = this->str;
            sprite* s = this->s;
            uv* uv1 = this->f->uv;
            float x = s->s1.x;
            float y = s->s1.y;
            uint textureIndex = this->f->tex->index;

            while (true)
            {
                // iterate untill null terminating character
                if (*it == 0)
                    break;

                if (*it == '\n')
                {
                    x = this->s->s1.x;
                    y += this->s->s1.sy + this->verticalSpace;
                    it++;
                    continue;
                }

                s->s2.uv1 = uv1[*it - ' '];
                s->s1.x = x;
                s->s1.y = y;
                s->s1.textureIndex = textureIndex;
                // set scale and origin equal to the first sprite in the set
                s->s2.scale = this->s->s2.scale;
                s->s2.origin = this->s->s2.origin;

                it++;
                s++;
                x += this->s->s1.sx + this->horizontalSpace;
            }

            // if old text was longer then there will be sprites still drawn after the new text
            // zero them out
            if (this->last != nullptr)
            {
                for (; s < this->last; s++) s->s1.textureIndex = TEXTURE_INVISIBLE;
            }
            this->last = s;
        }
    };

    struct rendererInfo
    {
        system::window* wnd;
        float clearColor[4];
    };

    struct renderer
    {
        system::window* window;
        IDXGISwapChain* swapChain;
        ID3D11RenderTargetView* backBuffer;
        ID3D11Device* device;
        ID3D11DeviceContext* context;
        ID3D11VertexShader* defaultVS;
        ID3D11PixelShader* defaultPS;
        ID3D11PixelShader* defaultPost;
        ID3D11InputLayout* layout; //vertex input layout pos:float[3] col:float[3] uv:float[2]
        ID3D11DepthStencilView* depthStencilView;
        ID3D11Texture2D* depthStencilBuffer;
        ID3D11RasterizerState* wireframe;
        ID3D11RasterizerState* solid;
        ID3D11SamplerState* point;
        ID3D11SamplerState* linear;
        ID3D11Buffer* bufferVS;
        ID3D11Buffer* bufferVS2;
        camera camera;
        float backBufferColor[4];
        double frequency;
        long long startTime;
        long long prevFrameTime;
        double gameTime;
        double frameTime;
        bool fullscreen;
    };

    enum class TextureFilter { Point, Linear };

    void Checkhr(HRESULT hr, int line)
    {
        if (hr == 0)
            return;
        char str[128];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0,
            hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            str, 128, 0);
        fprintf(stderr, str);
    }

    ID3D11SamplerState* CreateSampler(renderer* g, TextureFilter mode)
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
        g->device->CreateSamplerState(&sampDesc, &sampler);
        return sampler;
    }

    ID3D11PixelShader* CreatePixelShaderFromString(renderer* g, const char* str, const char* entryPoint, const char* target)
    {
        ID3D11PixelShader* result;
        ID3D10Blob* ps;
        ID3D10Blob* errorMsg;
        HRESULT hr = D3DCompile(str, strlen(str), 0, 0, 0, entryPoint, target, 0, 0, &ps, &errorMsg);
        if (errorMsg != nullptr)
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
            Checkhr(hr, __LINE__);
        }
        //D3DCompile
        hr = g->device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), 0, &result);
        Checkhr(hr, __LINE__);
        ps->Release();
        return result;
    }

    ID3D11VertexShader* CreateVertexShaderFromString(renderer* g, const char* str, const char* entryPoint, const char* target)
    {
        ID3D11VertexShader* result = nullptr;
        ID3D10Blob* vs;
        ID3D10Blob* errorMsg;
        HRESULT hr = D3DCompile(str, strlen(str), 0, 0, 0, entryPoint, target, 0, 0, &vs, &errorMsg);
        
        if (errorMsg != nullptr)
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
            Checkhr(hr, __LINE__);
        }
        hr = g->device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), 0,
            &result);
        Checkhr(hr, __LINE__);
        vs->Release();
        return result;
    }

    void graphicsInit(rendererInfo* info, renderer* g)
	{        
        HRESULT hr = 0;
        g->window = info->wnd;
        g->fullscreen = false;
        //assign global variable
        memcpy(g->backBufferColor, info->clearColor, sizeof(float) * 4);

        // camera
        g->camera.aspectRatio = g->window->width / (float)g->window->height;
        g->camera.rotation = 0;
        g->camera.scale = 1;
        g->camera.x = 0;
        g->camera.y = 0;

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
            D3D11_SDK_VERSION, &scd, &g->swapChain, &g->device, NULL,
            &g->context); 
        Checkhr(hr, __LINE__);

        ////    BACK BUFFER AS RENDER TARGET, DEPTH STENCIL   ////
        // get the address of the back buffer
        ID3D11Texture2D* buf;
        g->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&buf);
        // use the back buffer address to create the render target
        hr = g->device->CreateRenderTargetView(buf, NULL, &g->backBuffer);
        Checkhr(hr, __LINE__);
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

        hr = g->device->CreateTexture2D(&depthStencilDesc, NULL, &g->depthStencilBuffer);
        Checkhr(hr, __LINE__);
        hr = g->device->CreateDepthStencilView(g->depthStencilBuffer, NULL, &g->depthStencilView);
        Checkhr(hr, __LINE__);

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
        g->context->RSSetViewports(1, &viewport);

        ////    VS and PS    ////
        g->defaultVS = CreateVertexShaderFromString(g, rc_VertexShader, "main", "vs_5_0");
        g->context->VSSetShader(g->defaultVS, 0, 0);
        g->defaultPS = CreatePixelShaderFromString(g, rc_PixelShader, "main", "ps_5_0");
        g->defaultPost = CreatePixelShaderFromString(g, rc_PostProcessing, "main", "ps_5_0");

        //shared vertex shader buffer
#ifdef VI_VALIDATE
        if (sizeof(sprite) % 16 != 0)
        {
            fprintf(stderr, "Sprite is not multiple of 16 bytes");
            return;
        }
#endif
        D3D11_BUFFER_DESC cbbd;
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = sizeof(sprite);
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        g->device->CreateBuffer(&cbbd, NULL, &g->bufferVS);
        g->context->VSSetConstantBuffers(0, 1, &g->bufferVS);
#ifdef VI_VALIDATE
        if (sizeof(camera) % 16 != 0)
        {
            fprintf(stderr, "camera is not multiple of 16 bytes");
            return;
        }
#endif
        ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));
        cbbd.Usage = D3D11_USAGE_DEFAULT;
        cbbd.ByteWidth = sizeof(camera);
        cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbbd.CPUAccessFlags = 0;
        cbbd.MiscFlags = 0;
        g->device->CreateBuffer(&cbbd, NULL, &g->bufferVS2);
        g->context->VSSetConstantBuffers(1, 1, &g->bufferVS2);

        D3D11_RASTERIZER_DESC rd;
        ZeroMemory(&rd, sizeof(rd));
        rd.FillMode = D3D11_FILL_WIREFRAME;
        rd.CullMode = D3D11_CULL_NONE;
        hr = g->device->CreateRasterizerState(&rd, &g->wireframe);
        Checkhr(hr, __LINE__);
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_FRONT;
        hr = g->device->CreateRasterizerState(&rd, &g->solid);
        Checkhr(hr, __LINE__);

        g->context->OMSetRenderTargets(1, &g->backBuffer, g->depthStencilView);
        g->context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g->context->RSSetState(g->solid);
        g->context->PSSetShader(g->defaultPS, 0, 0);

        g->point = CreateSampler(g, TextureFilter::Point);
        g->linear = CreateSampler(g, TextureFilter::Linear);
        g->context->PSSetSamplers(0, 1, &g->point);

        //// *********** PIPELINE SETUP ENDS HERE *********** ////
	}

	void destroyGraphics(renderer* g)
	{
        g->defaultPS->Release();
        g->defaultVS->Release();
        g->swapChain->Release();
        g->depthStencilView->Release();
        g->depthStencilBuffer->Release();
        g->backBuffer->Release();
        g->device->Release();
        g->context->Release();
	}

    // Create texture where pixels are uncompressed, not encoded, 4 bytes per pixel formatted RGBA, stored lineary.
    // insert texture to internal storage
    void createTextureFromBytes(texture* t, renderer* g, byte* data, uint width, uint height)
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

        HRESULT hr = g->device->CreateTexture2D(&desc, &sub, &tex);
        Checkhr(hr, __LINE__);

        D3D11_TEXTURE2D_DESC desc2;
        tex->GetDesc(&desc2);
        ID3D11ShaderResourceView* srv = nullptr;
        hr = g->device->CreateShaderResourceView(tex, 0, &srv);
        Checkhr(hr, __LINE__);
        tex->Release();

        t->shaderResource = srv;
    }

    // Create texture from file in memory.
    // Difference between this and 'createTextureFromFile' is that file is in memory.
    // It's useful because you can have PNG or other encoded image in memory
    // and this can create texture from that. Supports lots of formats.
    void createTextureFromInMemoryFile(texture* t, renderer* g, byte* file, int len)
    {
        int x = -1, y = -1, n = -1;
        const int components = 4; // components means how many elements from 'RGBA'
                                  // you want to return, I want 4 (RGBA) even in not all 4 are present
        byte* data = stbi_load_from_memory(file, len, &x, &y, &n, components);

#ifdef VI_VALIDATE
        if (data == nullptr)
        {
            fprintf(stderr, "createTexture could not open the file\n");
            exit(1);
        }
#endif

        createTextureFromBytes(t, g, data, x, y);
        stbi_image_free(data);
    }

    // Create texture from file on disk. Supports lots of formats.
    void createTextureFromFile(texture* t, renderer* g, const char* filename)
    {
        int x = -1, y = -1, n = -1;
        const int components = 4; // components means how many elements from 'RGBA'
                                  // you want to return, I want 4 (RGBA) even in not all 4 are present
        byte* data = stbi_load(filename, &x, &y, &n, components);

#ifdef VI_VALIDATE
        if (data == nullptr)
        {
            fprintf(stderr, "createTexture could not open the file\n");
            exit(1);
        }
#endif

        createTextureFromBytes(t, g, data, x, y);
        stbi_image_free(data);
    }

    void destroyTexture(texture* t)
    {
        t->shaderResource->Release();
    }

    void beginScene(renderer* g)
    {
        g->context->ClearRenderTargetView(g->backBuffer, g->backBufferColor);
        g->context->ClearDepthStencilView(g->depthStencilView,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        g->context->UpdateSubresource(g->bufferVS2, 0, NULL, &g->camera, 0, 0);
    }

    void drawSprite(renderer* g, sprite* s, texture* t)
    {
        if (t != nullptr) g->context->PSSetShaderResources(0, 1, &t->shaderResource);
        g->context->UpdateSubresource(g->bufferVS, 0, NULL, s, 0, 0);
        g->context->Draw(6, 0);
    }

    void endScene(renderer* g)
    {
        g->swapChain->Present(0, 0);
    }

    // makes minimum changes to make object show when drawn
    void initSprite(sprite* s, uint textureIndex)
    {
        *s = {};
        s->s2.textureIndex = textureIndex;
        s->s2.col = { 1,1,1 };
        s->s2.scale = { 1,1 };
        s->s2.uv1 = { 0,0,1,1 };
    }
    
    // 'stopAfter' stop animation after that many frame changes, 0 = never stop
    void initAnimation(animation* a, sprite* s, time::timer* t, uv* uv, uint frameCount, float secondsPerFrame, uint stopAfter)
    {
        a->t = t;
        a->s = s;
        a->uv = uv;
        a->speed = secondsPerFrame;
        a->frameCount = frameCount;
        a->currentFrame = 0;
        a->stopAfter = stopAfter;
        a->frameChanged = false;
        a->_elapsedTime = 0;
        a->_playing = false;
        a->_frameChanges = 0;
        // update uv to the current frame
        a->s->s2.uv1 = a->uv[a->currentFrame];
    }

	void initDynamic(dynamic* d, sprite* s, time::timer* t)
	{
        *d = {};
        d->s = s;
        d->t = t;
        d->_lastUpdate = t->getGameTimeSec();
	}

    // make 'updateAnimation' animate frames
    void playAnimation(animation* a)
    {
        if (a->_playing) return;

        a->_playing = true;
        a->_lastUpdate = a->t->getGameTimeSec();
        // update uv to the current frame
        a->s->s2.uv1 = { a->uv[a->currentFrame] };
    }

    // animation will stop and 'updateAnimation' will no longer animate frames
    void pauseAnimation(animation* a)
    {
        a->_playing = false;
    }

    // stop and reset animation so it can be played from the beginning
    void resetAnimation(animation* a)
    {
        a->currentFrame = 0;
        a->frameChanged = false;
        a->_elapsedTime = 0;
        a->_playing = false;
        a->_frameChanges = 0;

    }

    // stops playing 'from' starts playing 'to'
    // if 'from' is not playing ot 'to' is playing then nothing happens
    void switchAnimation(animation* from, animation* to)
    {
        if (!from->_playing || to->_playing)
            return;

        resetAnimation(from);
        playAnimation(to);
    }

    // this is true if last 'updateAnimation' changed 'currentFrame'
    bool animationFrameChanged(animation* a)
    {
        return a->frameChanged;
    }

    void animationFlipHorizontally(animation* a)
    {
        for (uint i = 0; i < a->frameCount; i++)
            util::swap(a->uv[i].left, a->uv[i].right);
    }

    void animationFlipVertically(animation* a)
    {
        for (uint i = 0; i < a->frameCount; i++)
            util::swap(a->uv[i].top, a->uv[i].bottom);
    }

    // current algorithm 
    // measure how much time elapsed since last update and add it to total time elapsed
    // if total time elapsed is greater than speed (thus measured in seconds per frame)
    // then reduce total time elapsed by speed and change frame
    void updateAnimation(animation* a)
    {
        // not playing, early break
        if (!a->_playing)
            return;

        // set frame changed to false to invalidate previous true
        a->frameChanged = false;
        float gameTime = a->t->getGameTimeSec();
        // elpased since last update
        float elapsed = gameTime - a->_lastUpdate;
        // update last update
        a->_lastUpdate = gameTime;
        // update elapsed
        a->_elapsedTime += elapsed;

        // see if enough time elapsed to change frame
        if (a->_elapsedTime > a->speed)
        {
            // subtract the duration of one frame
            a->_elapsedTime -= a->speed;
            // update frame index including looping
            a->currentFrame = (a->currentFrame + 1) % a->frameCount;
            a->frameChanged = true;
            a->_frameChanges++;

            // update uv
            uv* uv = a->uv + a->currentFrame;
            a->s->s2.uv1 = *uv;

            // enough frame changed occured so stop playing
            if (a->stopAfter != 0 && a->_frameChanges > a->stopAfter)
                a->_playing = false;
        }
    }

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

            if (x >= width* info->rowLength + offsetx)
            {
                x = offsetx;
                y += height;
            }
        }
    }    

    void updateDynamicSprite(dynamic* d)
    {
        float currentTime = d->t->getGameTimeSec();
        float delta = currentTime - d->_lastUpdate;
        d->_lastUpdate = currentTime;

        d->velx += d->accx * delta;
        d->s->s1.x += d->velx * delta;
        d->vely += d->accy * delta;
        d->s->s1.y += d->vely * delta;
        d->velz += d->accz * delta;
        d->s->s1.z += d->velz * delta;
        d->velrot += d->accrot * delta;
        d->s->s1.rot += d->velrot * delta;
        d->velsx += d->accsx * delta;
        d->s->s1.sx += d->velsx * delta;
        d->velsy += d->accsy * delta;
        d->s->s1.sy += d->velsy * delta;
    }

    // 'pixelWidth' and 'pixelHeight' are dimensions in pixel
    void setPixelScale(renderer* g, uint pixelWidth, uint pixelHeight, float* sx, float* sy)
    {
        *sx = 2.0f / g->window->width / g->camera.scale * pixelWidth * g->camera.aspectRatio;
        *sy = 2.0f / g->window->height / g->camera.scale * pixelHeight;
    }

    // 'pixelx' and 'pixely' are window coordinates in pixels (0,0) is in upper left corner
    void setScreenPos(renderer* g, uint pixelx, uint pixely, float* x, float* y)
    {
        *x = 2.0f / g->window->width * (pixelx - g->window->width / 2.0f) / g->camera.scale * g->camera.aspectRatio;
        *y = 2.0f / g->window->height * (pixely - g->window->height / 2.0f) / g->camera.scale;
    }

    // puts width and height in world coordinates of 1px in f[0] and f[1]
    void getPixelScale(renderer* g, float* f)
    {
        f[0] = 2.0f / g->window->width / g->camera.scale * g->camera.aspectRatio;
        f[1] = 2.0f / g->window->height / g->camera.scale;
    }

    // utli function that will calc uv coords if you know pixel coords
    void setUvFromPixels(float pixelOffsetX, float pixelOffsetY, float pixelWidth,
        float pixelHeight, float pixelTextureWidth, float pixelTextureHeight, uv* uv1)
    {
        uv1->left = pixelOffsetX / pixelTextureWidth;
        uv1->top = pixelOffsetY / pixelTextureHeight;
        uv1->right = uv1->left + pixelWidth / pixelTextureWidth;
        uv1->bottom = uv1->top + pixelHeight / pixelTextureHeight;
    }
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

            this->_cursorWorldx = ((float)this->_cursorClientx - w->width / 2) / w->width / c->scale * c->aspectRatio * 2 + c->x;
            this->_cursorWorldy = ((float)this->_cursorClienty - w->height / 2) / w->height / c->scale * 2 + c->y;
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

namespace vi::res
{
    struct resources
    {
        memory::alloctrack* a;
        std::vector<gl::texture> textures;
        std::vector<gl::font> fonts;
        std::vector<gl::sprite*> sprites;
        std::vector<gl::animation*> animations;
        std::vector<gl::text*> texts;
        std::vector<gl::dynamic*> dynamics;
        std::vector<fn::routine*> routines;

        resources() { this->a == nullptr; }

        gl::texture* addTexture()
        {
            uint index = this->textures.size();
            this->textures.push_back({});
            this->textures[index].index = index;
            return this->textures.data() + index;
        }

        gl::font* addFont()
        {
            uint index = this->fonts.size();
            this->fonts.push_back({});
            return this->fonts.data() + index;
        }

        gl::animation* addAnimation()
        {
            gl::animation* a = this->a->alloc<gl::animation>(1);
            this->animations.push_back(a);
            return a;
        }

        gl::text* addText()
        {
            gl::text* t = this->a->alloc<gl::text>(1);
            this->texts.push_back(t);
            return t;
        }

        gl::dynamic* addDynamic()
        {
            gl::dynamic* d = this->a->alloc<gl::dynamic>(1);
            this->dynamics.push_back(d);
            return d;
        }

        gl::sprite* addSprite()
        {
            gl::sprite* s = this->a->alloc<gl::sprite>(1);
            this->sprites.push_back(s);
            return s;
        }

        fn::routine* addRoutine()
        {
            fn::routine* r = this->a->alloc<fn::routine>(1);
            this->routines.push_back(r);
            return r;
        }

        void free()
        {
            for (uint i = 0; i < this->animations.size(); i++) this->a->free(this->animations[i]);
            this->animations.clear();
            for (uint i = 0; i < this->dynamics.size(); i++) this->a->free(this->dynamics[i]);
            this->dynamics.clear();
            for (uint i = 0; i < this->routines.size(); i++) this->a->free(this->routines[i]);
            this->routines.clear();
            for (uint i = 0; i < this->sprites.size(); i++) this->a->free(this->sprites[i]);
            this->sprites.clear();
            for (uint i = 0; i < this->texts.size(); i++) this->a->free(this->texts[i]);
            this->texts.clear();
        }
    };
}

namespace vi
{
    struct vivaInfo
    {
        uint width;
        uint height;
        const char* title;
        uint queueCapacity;
    };

    struct viva
    {
        input::keyboard keyboard;
        input::mouse mouse;
        system::window window;
        gl::renderer graphics;
        memory::alloctrack alloctrack;
        time::timer timer;
        fn::queue queue;
        res::resources resources;

        void init(vivaInfo* info)
        {
            system::windowInfo wInfo;
            wInfo.width = info->width;
            wInfo.height = info->height;
            wInfo.title = info->title;

            gl::rendererInfo rInfo;
            rInfo.clearColor[0] = 47/255.0f;
            rInfo.clearColor[1] = 79/255.0f;
            rInfo.clearColor[2] = 79/255.0f;
            rInfo.clearColor[3] = 1;
            rInfo.wnd = &this->window;

            system::initWindow(&wInfo, &this->window);
            this->keyboard.init();
            this->mouse.init();
            gl::graphicsInit(&rInfo, &this->graphics);
            this->timer.init();

            // if queue capacity is not set then set it to 1
            if (info->queueCapacity == 0) info->queueCapacity = 1;

            this->queue.init(&this->timer);
            this->resources.a = &this->alloctrack;

#ifdef VI_VALIDATE
            this->alloctrack.track = true;
#endif
        }

        void destroy()
        {
            this->resources.free();
#ifdef VI_VALIDATE
            this->alloctrack.report();
#endif // VI_VALIDATE

            gl::destroyGraphics(&this->graphics);
            system::destroyWindow(&this->window);
        }

        void loop(std::function<void()> userLoop)
        {
            while (vi::system::updateWindow(&this->window))
            {
                this->keyboard.update();
                this->mouse.update(&this->window, &this->graphics.camera);
                this->timer.update();

                userLoop();

                for (uint i = 0; i < this->resources.animations.size(); i++)
                    gl::updateAnimation(this->resources.animations[i]);
                for (uint i = 0; i < this->resources.dynamics.size(); i++)
                    gl::updateDynamicSprite(this->resources.dynamics[i]);
                
                gl::beginScene(&this->graphics);
                for (uint i = 0; i < this->resources.sprites.size(); i++)
                {
                    gl::sprite* s = this->resources.sprites[i];
                    gl::texture* t = s->s1.textureIndex < vi::gl::TEXTURE_BLANK ? 
                        this->resources.textures.data() + s->s1.textureIndex : nullptr;
                    gl::drawSprite(&this->graphics, s, t);
                }
                gl::endScene(&this->graphics);
            }
        }
    };

    
}

#endif

#ifndef VIVA_IMPL
// here should be prototypes and declarations only for compiler
#endif