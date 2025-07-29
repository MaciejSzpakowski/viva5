
#define VI_VALIDATE
#include "viva_impl.h"
#include "DirectXMath.h"

namespace examples
{
    struct resources
    {
        vi::memory::alloctrack a;
        std::vector<vi::gl::texture*> textures;
        std::vector<vi::gl::font*> fonts;
        std::vector<vi::gl::sprite*> sprites;
        std::vector<vi::gl::animation*> animations;
        std::vector<vi::gl::text*> texts;
        std::vector<vi::gl::dynamic*> dynamics;
        std::vector<vi::fn::routine*> routines;

        vi::gl::texture* addTexture()
        {
            uint index = this->textures.size();
            vi::gl::texture* t = this->a.alloc<vi::gl::texture>(1);
            vi::util::zero(t);
            t->index = index;
            this->textures.push_back(t);
            return t;
        }

        vi::gl::font* addFont()
        {
            vi::gl::font* f = this->a.alloc<vi::gl::font>(1);
            vi::util::zero(f);
            this->fonts.push_back(f);
            return f;
        }

        vi::gl::animation* addAnimation()
        {
            vi::gl::animation* a = this->a.alloc<vi::gl::animation>(1);
            vi::util::zero(a);
            this->animations.push_back(a);
            return a;
        }

        vi::gl::text* addText()
        {
            vi::gl::text* t = this->a.alloc<vi::gl::text>(1);
            vi::util::zero(t);
            this->texts.push_back(t);
            return t;
        }

        vi::gl::dynamic* addDynamic()
        {
            vi::gl::dynamic* d = this->a.alloc<vi::gl::dynamic>(1);
            vi::util::zero(d);
            this->dynamics.push_back(d);
            return d;
        }

        vi::gl::sprite* addSprite()
        {
            return this->addSprite(1);
        }

        vi::gl::sprite* addSprite(uint len)
        {
            vi::gl::sprite* s = this->a.alloc<vi::gl::sprite>(len);
            vi::util::zeron(s, len);
            for (uint i = 0; i < len; i++) this->sprites.push_back(s + i);
            return s;
        }

        vi::fn::routine* addRoutine()
        {
            vi::fn::routine* r = this->a.alloc<vi::fn::routine>(1);
            vi::util::zero(r);
            this->routines.push_back(r);
            return r;
        }

        void free()
        {
            this->textures.clear();
            this->fonts.clear();
            this->animations.clear();
            this->dynamics.clear();
            this->routines.clear();
            this->sprites.clear();
            this->texts.clear();
            for (uint i = 0; i < this->a.allocations.size(); i++)
                this->a.free(this->a.allocations[i]);
            this->a.allocations.clear();
        }
    };

    struct vivaInfo
    {
        uint width;
        uint height;
        const char* title;
        uint queueCapacity;
    };

    struct viva
    {
        vi::input::keyboard keyboard;
        vi::input::mouse mouse;
        vi::system::window window;
        vi::gl::renderer graphics;
        vi::memory::alloctrack alloctrack;
        vi::time::timer timer;
        vi::fn::queue queue;
        resources resources;

        void init(vivaInfo* info)
        {
            vi::system::windowInfo wInfo;
            wInfo.width = info->width;
            wInfo.height = info->height;
            wInfo.title = info->title;

            vi::gl::rendererInfo rInfo;
            rInfo.clearColor[0] = 47 / 255.0f;
            rInfo.clearColor[1] = 79 / 255.0f;
            rInfo.clearColor[2] = 79 / 255.0f;
            rInfo.clearColor[3] = 1;
            rInfo.wnd = &this->window;

            this->window.init(&wInfo);
            this->keyboard.init();
            this->mouse.init();
            this->graphics.init(&rInfo);
            this->timer.init();

            // if queue capacity is not set then set it to 1
            if (info->queueCapacity == 0) info->queueCapacity = 1;

            this->queue.init(&this->timer);

#ifdef VI_VALIDATE
            this->alloctrack.track = true;
#endif
        }

        void destroy()
        {
            for (uint i = 0; i < this->resources.textures.size(); i++)
                this->graphics.destroyTexture(this->resources.textures[i]);

            this->resources.free();
#ifdef VI_VALIDATE
            this->alloctrack.report();
#endif // VI_VALIDATE

            this->graphics.destroy();
            this->window.destroy();
        }

        void loop(std::function<void()> userLoop)
        {
            while (this->window.update())
            {
                this->keyboard.update();
                this->mouse.update(&this->window, &this->graphics.camera);
                this->timer.update();

                userLoop();

                for (uint i = 0; i < this->resources.animations.size(); i++)
                    this->resources.animations[i]->update();
                for (uint i = 0; i < this->resources.dynamics.size(); i++)
                    this->resources.dynamics[i]->update();

                this->graphics.beginScene();
                for (uint i = 0; i < this->resources.sprites.size(); i++)
                {
                    vi::gl::sprite* s = this->resources.sprites[i];
                    this->graphics.drawSprite(s);
                }
                this->graphics.endScene();
            }
        }
    };

    void empty() {}
    
    struct user
    {
        char name[20];
        uint id;
        vi::net::endpoint ep;
    };

    void performance()
    {
        const uint count = 10000;
        vivaInfo info;
        viva v;
        info.width = 960;
        info.height = 540;
        info.queueCapacity = 1;
        info.title = "Performance";
        v.init(&info);

        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/0x72_DungeonTilesetII_v1.png");
        vi::util::rng rng;

        for (uint i = 0; i < count; i++)
        {
            vi::gl::sprite* s = v.resources.addSprite();
            s->init(t);
            v.graphics.setUvFromPixels(s, 293.f, 18.f, 6.f, 13.f, 512.f, 512.f);
            v.graphics.setPixelScale(s, 6*2, 13*2);
            s->s1.rot = rng.rnd() * 2.0f * 3.1415926f;
            s->s1.x = rng.rnd() * 2.0f - 1;
            s->s1.y = rng.rnd() * 2.0f - 1;
        }

        uint frames = 0;
        uint fps = 0;
        float lastUpdate = 0;

        auto loop = [&]()
        {
            float gameTime = v.timer.getGameTimeSec();
            frames++;
            if (gameTime - lastUpdate > 1.0f)
            {
                lastUpdate = gameTime;
                fps = frames;
                frames = 0;
                printf("%d\n", fps);
            }
            float tick = v.timer.getTickTimeSec();
            for (uint i = 0; i < v.resources.sprites.size(); i++)
            {
                v.resources.sprites[i]->s1.rot += tick;
            }
        };

        v.loop(loop);
        v.destroy();
    }

    /*void network()
    {
        vi::net::endpoint serverSide;
        vi::net::client client;

        // init network in windows
        vi::net::initNetwork();
        vi::net::server server;
        // start server at port 10000
        server.init(10000);
        // start client and point it at localhost:10000
        client.init("127.0.0.1", 10000);
        // client sends to server
        client.send((byte*)"Hello", 6);
        byte data[20];
        // server receives
        // since this is not blocking it might fail if send is not fast enough
        // but I guess it is in localhost
        server.receive(data, 20, &serverSide);
        char clientip[20];
        memset(clientip, 0, 20);
        // server determines ip of client
        serverSide.getAddress(clientip, 20);
        printf("server side, %s said: %s\n", clientip, data);
        // server sends to client
        server.send((const byte*)"Hi", 3, &serverSide);
        // client receives
        // since this is not blocking it might fail if send is not fast enough
        // but I guess it is in localhost
        client.receive(data, 20);
        printf("client side, server said: %s\n", data);
        // release resources
        client.destroyClient();
        server.destroyServer();
        // uninit network in windows
        vi::net::uninitNetwork();
    }*/

    // top chest will blink from start
    // middle chest will start blinking after 5s
    // bottom chest will stop blinking after 5s
    /*void queue()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 960;
        winfo.height = 540;
        winfo.title = "Queue";
        vi::system::window wnd;
        vi::system::initWindow(&winfo, &wnd);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        float clearColor[] = { 0,0,1,1 };
        memcpy(ginfo.clearColor, clearColor, sizeof(float) * 4);
        vi::gl::renderer g;
        g.init(&ginfo);
        vi::time::timer timer;
        timer.init();
        vi::fn::queue queue;
        queue.init(&timer);

        auto loop = [&](uint i)
        {
            data.v.timer.update();
            data.v.queue.update();
            vi::graphics::drawScene(&data.v.graphics, data.sprites, 10, &data.v.camera);
        };
        
        vi::vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Queue";
        // set a queue capacity
        info.queueCapacity = 10;
        data.v.init(&info);

        vi::graphics::createTextureFromFile(&data.v.graphics, "textures/0x72_DungeonTilesetII_v1.png", data.tex);
        vi::graphics::pushTextures(&data.v.graphics, data.tex, 1);

#define MAKE_SPRITE(n,x,y)  vi::graphics::initSprite(data.sprites + n, data.tex[0].index); \
        vi::graphics::setUvFromPixels(240, 208, 16, 16, 512, 512, &data.sprites[n].s2.uv1); \
        vi::graphics::setPixelScale(&data.v.graphics, &data.v.camera, 16 * 5, 16 * 5, \
            &data.sprites[n].s1.sx, &data.sprites[n].s1.sy); \
        data.sprites[n].s2.pos = {x,y,0};

        MAKE_SPRITE(0, 1, -0.4f)
        MAKE_SPRITE(1, 1, 0)
        MAKE_SPRITE(2, 1, 0.4f)

        data.v.queue.setInterval([&]() 
        {
            if (data.sprites[0].s1.x > 0)
                data.sprites[0].s1.x = -10;
            else
                data.sprites[0].s1.x = 1;

            return 1;
        }, 0.15f);

        data.v.queue.initRoutine([&]()
        {
            if (data.sprites[1].s1.x > 0)
                data.sprites[1].s1.x = -10;
            else
                data.sprites[1].s1.x = 1;

            return 1;
        }, 5, 0.12f, 0);

        data.v.queue.initRoutine([&]()
        {
            if (data.sprites[2].s1.x > 0)
                data.sprites[2].s1.x = -10;
            else
                data.sprites[2].s1.x = 1;

            return 1;
        }, 0, 0.1f, 5);

#undef MAKE_SPRITE

        vi::system::loop<uint>(loop, 0);

        vi::graphics::destroyTexture(&data.v.graphics, data.tex);
        data.v.destroy();
    }*/
        
    /// <summary>
    /// layer sprites based on z coordinate
    /// if objects have the same z coord then they layered based
    /// on order in which they are drawn which is not good
    /// if you want to put something at the top
    /// Here, upper sprites should be all above lower sprites
    /// so they are layered independently from order.
    /// </summary>
    void zindex()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 960;
        winfo.height = 540;
        winfo.title = "Z Index";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);
        vi::gl::texture t;
        g.createTextureFromFile(&t, "textures/0x72_DungeonTilesetII_v1.png");
        t.index = 0;
        vi::gl::sprite s[10];

        for (uint i = 0; i < 10; i++)
        {
            s[i].init(&t);
            s[i].s2.pos = { -0.8f + i * 0.2f, i % 2 ? 0.2f : 0.1f, i % 2 ? 0.5f : 0.25f };
            g.setUvFromPixels(s + i, 240, 208, 16, 16, 512, 512);
            g.setPixelScale(s + i, 80, 80);
        }

        while (wnd.update())
        {
            g.beginScene();
            for (uint i = 0; i < 10; i++)
                g.drawSprite(s + i);
            g.endScene();
        }

        g.destroyTexture(&t);
        g.destroy();
        wnd.destroy();
    }

    // get some cursor data
    // left click and mouse move to move square
    // right click and mouse move to move square using raw input
    // HOME to return square to 0,0
    // move camera with WSAD zoom Q/E
    // fixed sprite wont be affected by camera transform
    // it will behave like camera was at 0,0 and scale 1
    /*void mouseAndFixedSprite()
    {
        gameData data;

        auto loop = [&](uint i)
        {
            vi::graphics::sprite* sq = data.sprites;
            vi::input::mouse* m = &data.v.mouse;
            data.v.timer.update();
            data.v.mouse.update(&data.v.window, &data.v.camera);
            data.v.keyboard.update();

            int x, y, dx, dy, xraw, yraw;
            float wx, wy;

            data.v.mouse.getCursorClientPos(&x, &y);
            data.v.mouse.getCursorScreenDelta(&dx, &dy);
            data.v.mouse.getCursorDeltaRaw(&xraw, &yraw);
            data.v.mouse.getCursorWorldPos(&wx, &wy);
            short mouseWheel = data.v.mouse.getWheelDelta();
            char str[500];
            // number of sprites to render
            uint count = sprintf(str, "Cursor:\n    Client %d %d\n    Delta %d %d\n    World %f %f\nSquare: %f %f\nMouse Wheel: %d",
                x, y, dx, dy, wx, wy, sq->s1.x, sq->s1.y, mouseWheel);
            // subtract new lines since they dont have glyphs and add back the square
            count = count - 5 + 1;

            data.t[0].str = str;
            data.t[0].update();

            // move square
            if (data.v.keyboard.isKeyDown(vi::input::key::LMOUSE))
            {
                sq->s1.x += (float)dx * 0.003f;
                sq->s1.y += (float)dy * 0.003f;
            }

            if (data.v.keyboard.isKeyDown(vi::input::key::RMOUSE))
            {
                sq->s1.x += (float)xraw * 0.01f;
                sq->s1.y += (float)yraw * 0.01f;
            }

            // reset square
            if (data.v.keyboard.isKeyDown(vi::input::key::HOME))
            {
                sq->s2.pos = { 0,0 };
            }

            float frameTime = data.v.timer.getTickTimeSec();

            if (data.v.keyboard.isKeyDown('A'))
            {
                data.v.camera.x -= frameTime;
            }
            else if (data.v.keyboard.isKeyDown('D'))
            {
                data.v.camera.x += frameTime;
            }

            if (data.v.keyboard.isKeyDown('W'))
            {
                data.v.camera.y -= frameTime;
            }
            else if (data.v.keyboard.isKeyDown('S'))
            {
                data.v.camera.y += frameTime;
            }

            if (data.v.keyboard.isKeyDown('Q'))
            {
                data.v.camera.scale *= 1 - frameTime * .3f;
            }
            else if (data.v.keyboard.isKeyDown('E'))
            {
                data.v.camera.scale *= 1 + frameTime * .3f;
            }

            vi::graphics::drawScene(&data.v.graphics, data.sprites, count, &data.v.camera);
        };

        vi::vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Mouse";
        data.v.init(&info);

        vi::graphics::createTextureFromFile(&data.v.graphics, "textures/font1.png", data.tex);
        byte dot[] = { 255,255,255,255 };
        vi::graphics::createTextureFromBytes(&data.v.graphics, dot, 1, 1, data.tex + 1);
        vi::graphics::pushTextures(&data.v.graphics, data.tex, 2);

        // font is a simple object that combines texture and uv for glyphs together
        vi::graphics::font font1 = { data.tex };
        vi::graphics::uvSplitInfo usi = { 256,36,0,0,8,12,32,96 };
        vi::graphics::uvSplit(&usi, font1.uv);

        // square for moving
        vi::graphics::initSprite(data.sprites, 1);
        data.sprites[0].s2.scale = { 0.3f,0.3f };
        data.sprites[0].s2.col = { 0.1f,0.4f,0.9f };

        // text starts at 2nd sprite
        data.t[0] = { &font1, data.sprites + 1, nullptr, 0, 0 };
        data.sprites[1].s2.pos = { -1, -0.5f };
        data.sprites[1].s2.origin = { -1,-1 };
        vi::graphics::setScreenPos(&data.v.graphics, &data.v.camera, 5, 5, 
            &data.sprites[1].s1.x, &data.sprites[1].s1.y);
        vi::graphics::setPixelScale(&data.v.graphics, &data.v.camera, 16, 24,
            &data.sprites[1].s1.sx, &data.sprites[1].s1.sy);

        // set text color black
        for (uint i = 1; i < 1000; i++)
        {
            data.sprites[i].s2.col = { 0,0,0 };
            data.sprites[i].s1.fixed = true;
        }

        vi::system::loop<uint>(loop, 0);

        vi::graphics::destroyTexture(&data.v.graphics, data.tex);
        data.v.destroy();
    }*/

    /// <summary>
    /// Typing test
    /// </summary>
    void typing()
    {
        char str[1000];
        memset(str, 0, 1000);
        sprintf(str, "Type something_");
        uint len = strlen(str) - 1;

        viva v;
        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Typing";
        v.init(&info);

        vi::gl::texture* t = v.resources.addTexture();
        vi::gl::font* f = v.resources.addFont();
        vi::gl::text* text = v.resources.addText();
        vi::gl::sprite* s = v.resources.addSprite(1000);
        v.graphics.createTextureFromFile(t, "textures/font1.png");

        f->tex = t;
        vi::gl::uvSplitInfo usi = { 256,36,0,0,8,12,32,96 };
        v.graphics.uvSplit(&usi, f->uv);

        text->init(f, s, 1000, str);

        s[0].s2.pos = { -1, -0.75f };
        v.graphics.setPixelScale(s, 16, 24);
        v.graphics.setScreenPos(s, 20, 20);
        text->update();

        auto loop = [&]()
        {
            if (v.keyboard.isKeyPressed(vi::input::BACKSPACE) && len > 0)
            {
                str[--len] = 0;
                strcat(str, "_");
                text->update();
            }
            else if (v.keyboard.isKeyPressed(vi::input::ENTER) && len < 900)
            {
                str[len++] = '\n';
                strcat(str, "_");
                text->update();
            }
            // there is no glyph for tab
            else if (v.keyboard.typedKey != 0 && len < 900 && v.keyboard.typedKey != '\t')
            {
                str[len++] = v.keyboard.typedKey;
                strcat(str, "_");
                text->update();
            }
        };

        v.loop(loop);

        v.destroy();
    }

    // display virtual key codes of keys that are down
    // some keys cause more than one code to be true (like shift,ctrl and alt)
    void inputState()
    {
        char str[1000];        

        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Input state";
        viva v;
        v.init(&info);

        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/font1.png");
        
        vi::gl::font* f = v.resources.addFont();
        f->tex = t;
        vi::gl::uvSplitInfo usi = { 256,36,0,0,8,12,32,96 };
        v.graphics.uvSplit(&usi, f->uv);

        vi::gl::sprite* s = v.resources.addSprite(1000);
        vi::gl::text* text = v.resources.addText();
        text->init(f, s, 1000, str);
        s[0].s2.pos = { -1, -0.75f };
        v.graphics.setPixelScale(s, 16, 24);
        v.graphics.setScreenPos(s, 20, 20);
        text->update();

        auto loop = [&]()
        {
            sprintf(str, "Press key(s) to show codes\n");
            for (int i = 0, c = 0; c < 256; c++)
            {
                if (v.keyboard.isKeyDown(c))
                    sprintf(str + strlen(str), "%d ", c);
            }

            text->update();
        };

        v.loop(loop);

        v.destroy();
    }

    void text()
    {
        viva v;
        bool flag = false;
        const uint capacity = 300;
        char str[capacity];
        const char* cstr = "Some text that\ncontains new line characters.\nEach glyph is just a sprite and\n"
            "can be manipulated individually.\nPress space to toggle";
        const char* extra = "\n more stuff";        

        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Text";
        v.init(&info);

        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/font1.png");
        
        memcpy(str, cstr, strlen(cstr) + 1);
        // font is a simple object that combines texture and uv for glyphs together
        vi::gl::font* f = v.resources.addFont();
        f->tex = t;
        vi::gl::uvSplitInfo usi = {256,36,0,0,8,12,32,96};
        v.graphics.uvSplit(&usi, f->uv);

        // INIT TEXT
        // set position, scale, origin of the first sprite and others will have
        // the same scale and origin and will be advanced starting from that first one's position
        vi::gl::sprite* s = v.resources.addSprite(capacity);
        vi::gl::text* text = v.resources.addText();
        // init does minimum initialization so text will appear
        text->init(f, s, capacity, str);

        s[0].s2.pos = { -1, -0.5f };
        v.graphics.setPixelScale(s, 16, 24);

        // 'vi::gl::text.updateText()' is really a util function to set sprites to look like text
        text->update();

        // find where 'individually' starts and color it red
        const char* ptr = strstr(str, "individually");
        // subtract 3 because new line characters dont have corresponding sprites
        uint index = ptr - str;
        uint len2 = strlen("individually");
        for (uint i = index; i < index + len2; i++) s[i].s2.col = { 1,0,0,1 };
        
        auto loop = [&]()
        {
            if (v.keyboard.isKeyPressed(vi::input::SPACE) && !flag)
            {
                flag = true;
                uint len = strlen(str);
                memcpy(str + len, extra, strlen(extra));
                text->update();

            }
            else if (v.keyboard.isKeyPressed(vi::input::SPACE) && flag)
            {
                flag = false;
                str[strlen(cstr)] = 0;
                text->update();
            }
        };

        v.loop(loop);

        v.destroy();
    }

    // move camera with WSAD zoom Q/E
    // zooming should be towards the center of the screen
    void camera()
    {
        // internal viva camera is worldview
        // the second camera is screen view (x,y and scale never changes)
        vi::time::timer timer;
        vi::input::keyboard keyboard;
        keyboard.init();
        timer.init();
        vi::system::windowInfo winfo = {};
        winfo.width = 960;
        winfo.height = 540;
        vi::gl::camera screenView = {};
        screenView.scale = 1;
        screenView.aspectRatio = 960.0f / 540.0f;
        winfo.title = "Camera";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);
        vi::gl::texture t;
        g.createTextureFromFile(&t, "textures/0x72_DungeonTilesetII_v1.png");
        t.index = 0;
        vi::gl::sprite s[6];
        for (int i = 0; i < 6; i++)
        {
            s[i].init(&t);
            g.setUvFromPixels(s + i, 293.f, 18.f, 6.f, 13.f, 512.f, 512.f);
            g.setPixelScale(s + i, 6 * 10, 13 * 10);
        }
        // move worldview
        s[0].s1.z = 0.5f;
        s[1].s2.pos = { -1,-1,0.5f };
        s[2].s2.pos = { 1,-1,0.5f };
        s[3].s2.pos = { -1, 1,0.5f };
        s[4].s2.pos = { 1,1,0.5f };
        g.setScreenPos2(s + 5, 20, 20);
        s[5].s2.origin = { -0.5f,-0.5f };

        while (wnd.update())
        {
            timer.update();
            float frameTime = timer.getTickTimeSec();
            keyboard.update();

            if (keyboard.isKeyDown('A')) g.camera.x -= frameTime;
            else if (keyboard.isKeyDown('D')) g.camera.x += frameTime;

            if (keyboard.isKeyDown('W')) g.camera.y -= frameTime;
            else if (keyboard.isKeyDown('S')) g.camera.y += frameTime;

            if (keyboard.isKeyDown('Q')) g.camera.scale *= 1 - frameTime * .3f;
            else if (keyboard.isKeyDown('E')) g.camera.scale *= 1 + frameTime * .3f;

            g.beginScene();
            // render worldview first (aka g.camera)
            for (int i = 0; i < 5; i++)
                g.drawSprite(s + i);
            // change camera to screenview (aka camera2)
            g.updateCamera(&screenView);
            g.drawSprite(s + 5);
            g.endScene();
        }

        g.destroyTexture(&t);
        g.destroy();
        wnd.destroy();
    }
    
    // just to make sure they still work
    // with vulkan it was much harder
    void multipleTextures()
    {
        viva v;
        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Multiple textures";
        v.init(&info);
        v.graphics.camera.scale = 0.5f;

        vi::gl::texture* t1 = v.resources.addTexture();
        v.graphics.createTextureFromFile(t1, "textures/bk.png");
        vi::gl::texture* t2 = v.resources.addTexture();
        v.graphics.createTextureFromFile(t2, "textures/elf.png");
        vi::gl::texture* t3 = v.resources.addTexture();
        v.graphics.createTextureFromFile(t3, "textures/sm.png");

        vi::gl::sprite* s1 = v.resources.addSprite();
        s1->init(t1);
        s1->s2.pos = { -1,1 };
        vi::gl::sprite* s2 = v.resources.addSprite();
        s2->init(t2);
        s2->s2.pos = { -1,-1 };
        vi::gl::sprite* s3 = v.resources.addSprite();
        s3->init(t3);
        s3->s2.pos = { 1,-1 };

        v.loop(empty);

        v.destroy();
    }

    // move with WSAD, flip sword with SPACE (this is to test keyPressed)
    // there a lot of noise to make it more interesting
    // state management is done in crapy way just to have quick and dirty example
    // relevant parts are: updateKeyboard, isKeyDown, 
    // animationFlipHorizontally, switchAnimation, moveTo
    void keyboardMultipleAnimationsMath()
    {
        // this variable is kept to keep direction for elf
        // so when it changes, animation is flipped
        float elfDirection = 1;
        float monsterDirection = 1;
        viva v;
        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Keyboard";
        v.init(&info);

        // init textures
        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/0x72_DungeonTilesetII_v1.png");

        // init elf
        struct 
        {
            vi::gl::uv walkUv[4];
            vi::gl::uv idleUv[4];
            vi::gl::sprite* s;
            vi::gl::animation* idle;
            vi::gl::animation* walk;
        } elf;
        elf.s = v.resources.addSprite();
        elf.walk = v.resources.addAnimation();
        elf.idle = v.resources.addAnimation();
        elf.s->init(t);
        v.graphics.setPixelScale(elf.s, 16 * 4, 28 * 4);
        vi::gl::uvSplitInfo usi1 = { 512,512,192,4,16,28,4,4 };
        v.graphics.uvSplit(&usi1, elf.walkUv);
        elf.walk->init(elf.s, &v.timer, elf.walkUv, 4, 0.09f, 0);
        vi::gl::uv elfIdleAni[4];
        vi::gl::uvSplitInfo usi2 = { 512,512,128,4,16,28,4,4 };
        v.graphics.uvSplit(&usi2, elf.idleUv);
        elf.idle->init(elf.s, &v.timer, elf.idleUv, 4, 0.1f, 0);
        elf.idle->play();

        struct
        {
            vi::gl::uv walkUv[4];
            vi::gl::uv idleUv[4];
            vi::gl::sprite* s;
            vi::gl::dynamic* d;
            vi::gl::animation* idle;
            vi::gl::animation* walk;
        } monster;
        monster.s = v.resources.addSprite();
        monster.s->init(t);
        monster.s->s1.x = 1;
        v.graphics.setPixelScale(monster.s, 16 * 4, 20 * 4);
        monster.d = v.resources.addDynamic();
        monster.d->init(monster.s, &v.timer);
        vi::gl::uvSplitInfo usi3 = { 512,512,432,204,16,20,4,4 };
        v.graphics.uvSplit(&usi3, monster.walkUv);
        monster.walk = v.resources.addAnimation();
        monster.walk->init(monster.s, &v.timer, monster.walkUv, 4, 0.09f, 0);
        vi::gl::uvSplitInfo usi4 = { 512,512,368,204,16,20,4,4 };
        v.graphics.uvSplit(&usi4, monster.idleUv);
        monster.idle = v.resources.addAnimation();
        monster.idle->init(monster.s, &v.timer, monster.idleUv, 4, 0.1f, 0);
        monster.idle->play();

        vi::gl::sprite* knife = v.resources.addSprite();
        knife->init(t);
        knife->s1.x = -1;
        v.graphics.setPixelScale(knife, 8 * 4, 19 * 4);
        v.graphics.setUvFromPixels(knife, 310, 124, 8, 19, 512, 512);

        auto loop = [&]()
        {
            // this is not the best way to move objects around but that's not the point of this example
            float frameTime = v.timer.getTickTimeSec();
            // animation management is non trivial
            // this is simple state variable to indicate two states: walk and idle
            // it's used to switch animation between walk and idle
            bool elfIsMoving = false;

            // check for key presses
            // if any is down then elf will move and is moving state will be true
            if (v.keyboard.isKeyDown('A'))
            {
                // direction changed, flip elf animations
                if (elfDirection > 0)
                {
                    elfDirection = -1;
                    elf.walk->flipHorizontally();
                    elf.idle->flipHorizontally();
                }

                elfIsMoving = true;
                elf.s->s1.x -= frameTime;
            }
            else if (v.keyboard.isKeyDown('D'))
            {
                // direction changed, flip elf animations
                if (elfDirection < 0)
                {
                    elfDirection = 1;
                    elf.walk->flipHorizontally();
                    elf.idle->flipHorizontally();
                }

                elfIsMoving = true;
                elf.s->s1.x += frameTime;
            }

            if (v.keyboard.isKeyDown('W'))
            {
                elfIsMoving = true;
                elf.s->s1.y -= frameTime;
            }
            else if (v.keyboard.isKeyDown('S'))
            {
                elfIsMoving = true;
                elf.s->s1.y += frameTime;
            }

            float distance = vi::math::distance2Dsq(
                elf.s->s1.x, elf.s->s1.y,
                monster.s->s1.x, monster.s->s1.y);

            // if monster is far enough then start moving towards elf
            if (distance > 0.31f * 0.31f)
            {
                vi::math::moveTo(monster.s->s1.x, monster.s->s1.y,
                    elf.s->s1.x, elf.s->s1.y, 0.9f,
                    &monster.d->velx, &monster.d->vely);
                // switch from idle to walk
                monster.idle->change(monster.walk);

                if (monsterDirection > 0 && monster.d->velx < 0)
                {
                    monsterDirection = -1;
                    monster.walk->flipHorizontally();
                    monster.idle->flipHorizontally();
                }
                else if (monsterDirection < 0 && monster.d->velx > 0)
                {
                    monsterDirection = 1;
                    monster.walk->flipHorizontally();
                    monster.idle->flipHorizontally();
                }
            }

            // if monster is close enough the stop moving
            // this number is smaller than to start moving
            // because weird things might happen at the border
            if (distance < 0.29f * 0.29f)
            {
                // stop moving
                monster.d->velx = 0;
                monster.d->vely = 0;
                // switch from walk to idle
                monster.walk->change(monster.idle);
            }

            // 'switchAnimation' is the most convenient way (as of writing this) to switch animation
            // 'switchAnimation' does nothing if correct animation is already playing
            // i.e. you dont have to check if state actually changed
            if (elfIsMoving) elf.idle->change(elf.walk);
            else elf.walk->change(elf.idle);

            // swap cleaver
            if (v.keyboard.isKeyPressed(vi::input::key::SPACE))
            {
                vi::util::swap(knife->s1.left, knife->s1.right);
            }

        };

        v.loop(loop);
        v.destroy();
    }

    void timerMotionAnimation()
    {
        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "Timer, motion and animation";
        viva v;
        v.init(&info);
        v.graphics.camera.scale = 0.1f;

        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/0x72_DungeonTilesetII_v1.png");

#define MAKE_SPRITE(__x,__y,__rot,__sx,__sy,__r,__g,__b) { vi::gl::sprite* s = v.resources.addSprite(); \
        s->init(t); \
        v.graphics.setUvFromPixels(s, 293.f, 18.f, 6.f, 13.f, 512.f, 512.f); \
        s->s2.col = {__r,__g,__b,1}; \
        s->s2.pos = {__x,__y}; \
        s->s2.rot = __rot; \
        v.graphics.setPixelScale(s, 6 * __sx, 13 * __sy); }

        MAKE_SPRITE(-14, -0, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(-10, -0, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(-6, -0, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(-2, -0, 0, 10, 10, 1, 1, 1)

#undef MAKE_SPRITE

        // make one object spin
        vi::gl::dynamic* d = v.resources.addDynamic();
        d->init(v.resources.sprites[0], &v.timer);
        d->velrot = 1.f;

        // init sprite for animation
        vi::gl::sprite* elf = v.resources.addSprite();
        elf->init(t);
        elf->s2.pos = { 4,2 };
        v.graphics.setPixelScale(elf, 16 * 4, 28 * 4);
        // init uv for animation using convenience function
        vi::gl::uv uvForAni[9];
        vi::gl::uvSplitInfo usi = {};
        usi.frameCount = 4;
        usi.pixelFrameHeight = 28;
        usi.pixelFrameWidth = 16;
        usi.pixelOffsetx = 192;
        usi.pixelOffsety = 4;
        usi.pixelTexHeight = 512;
        usi.pixelTexWidth = 512;
        usi.rowLength = 4;
        v.graphics.uvSplit(&usi, uvForAni);
        // init animation
        vi::gl::animation* ani = v.resources.addAnimation();
        ani->init(elf, &v.timer, uvForAni, 4, 0.1f, 0);
        ani->play();

        auto loop = [&]()
        {
            // manually update some properties scaled by timer
            v.resources.sprites[1]->s1.sx = sinf(v.timer.getGameTimeSec() * 10) / 2 + 1;
            v.resources.sprites[2]->s1.sy = sinf(v.timer.getGameTimeSec() * 7) / 2 + 5;
            v.resources.sprites[3]->s1.r = sinf(v.timer.getGameTimeSec()) / 4 + 0.75f;
            v.resources.sprites[3]->s1.g = sinf(v.timer.getGameTimeSec() + vi::math::FORTH_PI) / 4 + 0.75f;
            v.resources.sprites[3]->s1.b = sinf(v.timer.getGameTimeSec() + vi::math::FORTH_PI * 2) / 4 + 0.75f;
        };

        v.loop(loop);
        v.destroy();
    }

    // more drawing options
    void moreSprites()
    {
        vivaInfo info;
        info.width = 960;
        info.height = 540;
        info.title = "More sprites";
        viva v;
        v.init(&info);
        v.graphics.camera.scale = 0.1f;

        vi::gl::texture* t = v.resources.addTexture();
        v.graphics.createTextureFromFile(t, "textures/0x72_DungeonTilesetII_v1.png");

#define MAKE_SPRITE(__x,__y,__rot,__sx,__sy,__r,__g,__b) { \
        vi::gl::sprite* s = v.resources.addSprite(); \
        s->init(t); \
        v.graphics.setUvFromPixels(s, 293.f, 18.f, 6.f, 13.f, 512.f, 512.f); \
        s->s2.col = {__r,__g,__b,1}; \
        s->s2.pos = {__x,__y}; \
        s->s2.rot = __rot; \
        s->s2.origin = {0,0}; \
        v.graphics.setPixelScale(s, 6 * __sx, 13 * __sy); }

        // different scales
        MAKE_SPRITE(-14, -5, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(-11, -5, 0, 8, 8, 1, 1, 1)
        MAKE_SPRITE(-9, -5, 0, 6, 6, 1, 1, 1)
        MAKE_SPRITE(-7, -5, 0, 4, 4, 1, 1, 1)
        MAKE_SPRITE(-5, -5, 0, 2, 2, 1, 1, 1)
        MAKE_SPRITE(-3, -5, 0, 1, 1, 1, 1, 1)

        //// different colors
        MAKE_SPRITE(-14, 5, 0, 10, 10, 1, 0, 0)
        MAKE_SPRITE(-11, 5, 0, 10, 10, 0, 1, 0)
        MAKE_SPRITE(-8, 5, 0, 10, 10, 0, 0, 1)
        MAKE_SPRITE(-5, 5, 0, 10, 10, 0, 1, 1)
        MAKE_SPRITE(-2, 5, 0, 10, 10, 1, 0, 1)
        MAKE_SPRITE(1, 5, 0, 10, 10, 1, 1, 0)

        //// different rotations
        MAKE_SPRITE(0, -5, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(3, -5, vi::math::THIRD_PI, 10, 10, 1, 1, 1)
        MAKE_SPRITE(6, -5, vi::math::THIRD_PI * 2, 10, 10, 1, 1, 1)
        MAKE_SPRITE(9, -5, vi::math::THIRD_PI * 3, 10, 10, 1, 1, 1)
        MAKE_SPRITE(12, -5, vi::math::THIRD_PI * 4, 10, 10, 1, 1, 1)
        MAKE_SPRITE(15, -5, vi::math::THIRD_PI * 5, 10, 10, 1, 1, 1)

        //// uv swap to flip vertically and horizontally
        MAKE_SPRITE(7, 5, 0, 10, 10, 1, 1, 1)
        MAKE_SPRITE(11, 5, 0, 10, 10, 1, 1, 1)
        vi::util::swap(v.resources.sprites[19]->s1.left, v.resources.sprites[19]->s1.right);
        MAKE_SPRITE(7, 0, 0, 10, 10, 1, 1, 1)
        vi::util::swap(v.resources.sprites[20]->s1.top, v.resources.sprites[20]->s1.bottom);
        MAKE_SPRITE(11, 0, 0, 10, 10, 1, 1, 1)
        vi::util::swap(v.resources.sprites[21]->s1.left, v.resources.sprites[21]->s1.right);
        vi::util::swap(v.resources.sprites[21]->s1.top, v.resources.sprites[21]->s1.bottom);

#undef MAKE_SPRITE

        v.loop(empty);

        v.destroy();
    }

    void basicSprite()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 500;
        winfo.height = 500;
        winfo.title = "Basic Sprite";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);
        vi::gl::texture t;
        g.createTextureFromFile(&t, "textures/0x72_DungeonTilesetII_v1.png");
        t.index = 0;
        vi::gl::sprite s;
        s.init(&t);
        g.setUvFromPixels(&s, 293.f, 18.f, 6.f, 13.f, 512.f, 512.f);
        g.setPixelScale(&s, 6 * 10, 13 * 10);
        vi::gl::sprite blank;
        blank.init(nullptr);
        blank.s1.notexture = 1;
        blank.s2.col = { 0.5f,1.0f,0,1 };
        vi::gl::texture t2;
        byte bytes[] = { 255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,0,255 };
        g.createTextureFromBytes(&t2, bytes, 2, 2);
        vi::gl::sprite s2;
        s2.init(&t2);
        s2.s2.pos = { 0.7f,0.7f,0 };
        s2.s2.scale = { 0.2f,0.2f };

        while (wnd.update())
        {
            g.beginScene();
            g.drawSprite(&s);
            g.drawSprite(&blank);
            g.drawSprite(&s2);
            g.endScene();
        }

        g.destroyTexture(&t);
        g.destroyTexture(&t2);
        g.destroy();
        wnd.destroy();
    }

    void mesh()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 500;
        winfo.height = 500;
        winfo.title = "Cube";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);
        vi::time::timer timer;
        timer.init();

        vi::gl::camera3D cam3d =
        { 1,1,0.001f,1000.0f,{-10,10,-10},{0,0,0},{0,1,0} };
        g.camera3Dptr = &cam3d;

        vi::gl::texture t = {};
        g.createTextureFromFile(&t, "textures/b.png");        
        vi::gl::vertex v[] = {
           {-1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
           {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
           {1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
           {1.0f, -1.0f, -1.0f, 1.0f, 1.0f},
           {-1.0f, -1.0f, 1.0f, 1.0f, 1.0f},
           {1.0f, -1.0f, 1.0f, 0.0f, 1.0f},
           {1.0f,  1.0f, 1.0f, 0.0f, 0.0f},
           {-1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
           {-1.0f, 1.0f, -1.0f, 0.0f, 1.0f},
           {-1.0f, 1.0f,  1.0f, 0.0f, 0.0f},
           {1.0f, 1.0f,  1.0f, 1.0f, 0.0f},
           {1.0f, 1.0f, -1.0f, 1.0f, 1.0f},
           {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f},
           {1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
           {1.0f, -1.0f,  1.0f, 0.0f, 0.0f},
           {-1.0f, -1.0f,  1.0f, 1.0f, 0.0f},
           {-1.0f, -1.0f,  1.0f, 0.0f, 1.0f},
           {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f},
           {-1.0f,  1.0f, -1.0f, 1.0f, 0.0f},
           {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f},
           {1.0f, -1.0f, -1.0f, 0.0f, 1.0f},
           {1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
           {1.0f,  1.0f,  1.0f, 1.0f, 0.0f},
           {1.0f, -1.0f,  1.0f, 1.0f, 1.0f},
        };
        uint index[] = {
            2,1,0,
            3,2,0,
            6,5,4,
            7,6,4,
            10,9,8,
            11,10,8,
            14,13,12,
            15,14,12,
            18,17,16,
            19,18,16,
            22,21,20,
            23,22,20
        };
        vi::gl::vertex v2[] = {
                {0.8f,0.9f,0,1,0,1,1,1},
                {1,0.9f,0,0,1,1,1,1},
                {0.9f,1,0,0,0,1,1,1},
        };
        vi::gl::mesh dynamicMesh = {};
        dynamicMesh.t = &t;
        dynamicMesh.v = v2;

        const uint meshCount = 7;
        vi::gl::mesh m[meshCount];

        for (uint i = 0; i < meshCount; i++)
            g.initMesh(m + i, v, 24, index, 36, &t);

        m[0].pos.z = 5;
        m[1].pos.z = -5;
        m[2].pos.x = 5;
        m[3].pos.x = -5;
        m[4].pos.y = 5;
        m[5].pos.y = -5;
        m[6].sca.x = 2;
        m[6].sca.y = 2;
        m[6].sca.z = 2;
        m[0].data = 2;
        m[1].color = { 1,0,0 };

        vi::input::keyboard k;
        k.init();        

        while (wnd.update())
        {
            timer.update();
            k.update();
            float f1 = 4;
            if (k.isKeyDown('R'))
                cam3d.eye.z += timer.getTickTimeSec() * f1;
            if (k.isKeyDown('F'))
                cam3d.eye.z -= timer.getTickTimeSec() * f1;
            if (k.isKeyDown('A'))
                cam3d.eye.y += timer.getTickTimeSec() * f1;
            if (k.isKeyDown('D'))
                cam3d.eye.y -= timer.getTickTimeSec() * f1;
            if (k.isKeyDown('W'))
                cam3d.eye.x += timer.getTickTimeSec() * f1;
            if (k.isKeyDown('S'))
                cam3d.eye.x -= timer.getTickTimeSec() * f1;

            g.beginScene();
            m[6].rot.x = timer.getGameTimeSec();
            m[6].rot.y = timer.getGameTimeSec();
            m[6].rot.z = timer.getGameTimeSec();

            g.setWireframe();

            // cpu side transform
            // if transform is provided (float[16]) then it is used directly
            DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(0, 0, 15);
            DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH({ cam3d.eye.x, cam3d.eye.y, cam3d.eye.z },
                { cam3d.at.x,cam3d.at.y,cam3d.at.z },
                { cam3d.up.x,cam3d.up.y,cam3d.up.z });
            DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(cam3d.fovy, cam3d.aspectRatio, cam3d.znear, cam3d.zfar);
            DirectX::XMMATRIX transform = world * view * proj;
            g.drawMesh(m, (float*)(&transform));

            g.setSolid();

            for (uint i = 1; i < meshCount; i++)
                g.drawMesh(m + i);
            
            g.drawMeshDynamic(&dynamicMesh, 3);

            g.endScene();
        }

        //g.destroyTexture(&t);
        for (uint i = 0; i < meshCount; i++)
        {
            g.destroyMesh(m + i);
        }
        g.destroy();
        wnd.destroy();
    }

    void mesh2()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 500;
        winfo.height = 500;
        winfo.title = "Mesh Test";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);
        g.setWireframe();

        vi::gl::camera3D cam3d =
        { 1,1,0.001f,1000.0f,{0,5,-5},{0,0,0},{0,1,0} };
        g.camera3Dptr = &cam3d;

        vi::gl::vertex v[3] = {
           {0, -0.5f, -0.5f, 0.0f, 1.0f, 1,1,1,1},
           {0, -0.5f, 0.5f, 0.0f, 0.0f,1,1,1,1},
           {0, 1, 0, 1.0f, 0.0f,1,0,0,1}
        };
        DirectX::XMVECTOR v1[3] = {
            {-0.5f, -0.5f, 0,1},
            {0.5f, -0.5f, 0,1},
            {0, 1, 0,1}
        };
        vi::gl::vertex v2[3] = {
            {0,0,0,0,0,1,1,0,1},
            {0,0,0,0,0,1,1,0,1},
            {0,0,0,0,0,1,1,0,1},
        };

        vi::gl::mesh mesh;
        g.initMesh(&mesh, v, 3, nullptr, 0, nullptr);
        mesh.data = 2 | vi::gl::APPLY_TRANSFORM;
        vi::gl::mesh dynMesh = {};
        dynMesh.v = v2;
        dynMesh.data = 2;

        vi::input::keyboard k;
        k.init();
        vi::input::mouse m;
        m.init();

        while (wnd.update())
        {
            k.update();
            m.update(&wnd, nullptr);

            static int angle = 0;
            int dx, dy;
            m.getCursorScreenDelta(&dx, &dy);
            angle += dx;

            g.beginScene();

            DirectX::XMMATRIX world = DirectX::XMMatrixRotationY((angle + 0) * 3.141592f / 180.0f);
            DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH({ cam3d.eye.x, cam3d.eye.y, cam3d.eye.z },
                { cam3d.at.x,cam3d.at.y,cam3d.at.z },
                { cam3d.up.x,cam3d.up.y,cam3d.up.z });
            DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(cam3d.fovy, cam3d.aspectRatio, cam3d.znear, cam3d.zfar);
            DirectX::XMMATRIX transform = (world * view * proj);

            g.drawMesh(&mesh, (float*)(&transform));

            for (uint i = 0; i < 3; i++)
            {
                auto transformed = DirectX::XMVector4Transform(v1[i], transform);
                v2[i].pos.x = transformed.m128_f32[0] / transformed.m128_f32[3];
                v2[i].pos.y = transformed.m128_f32[1] / transformed.m128_f32[3];
                v2[i].pos.z = transformed.m128_f32[2] / transformed.m128_f32[3];
            }

            g.drawMeshDynamic(&dynMesh, 3);

            g.endScene();
        }

        g.destroyMesh(&mesh);
        g.destroy();
        wnd.destroy();
    }

    void blendState()
    {
        vi::system::windowInfo winfo = {};
        winfo.width = 500;
        winfo.height = 500;
        winfo.title = "Blend State";
        vi::system::window wnd;
        wnd.init(&winfo);
        vi::gl::rendererInfo ginfo = {};
        ginfo.wnd = &wnd;
        ginfo.clearColor[0] = 47 / 255.0f;
        ginfo.clearColor[1] = 79 / 255.0f;
        ginfo.clearColor[2] = 79 / 255.0f;
        ginfo.clearColor[3] = 1;
        vi::gl::renderer g;
        g.init(&ginfo);

        vi::gl::texture t1;
        g.createTextureFromFile(&t1, "./textures/b.png");

        vi::gl::texture t2;
        // water is 32bit transparent texture
        g.createTextureFromFile(&t2, "./textures/water.png");

        vi::gl::sprite s1;
        s1.init(&t1);
        s1.s1.z = 0.9f;
        s1.s2.scale = { 1.6f,1.6f };

        vi::gl::sprite s2;
        s2.init(nullptr);
        s2.s1.notexture = 1;
        s2.s2.pos = { -0.6f,-0.6f,0.6f };
        s2.s2.scale = { 0.5f,0.5f };
        s2.s1.a = 0.8f;

        vi::gl::sprite s3;
        s3.init(nullptr);
        s3.s1.notexture = 1;
        s3.s2.pos = { -0.0f,-0.6f,0.6f };
        s3.s2.scale = { 0.5f,0.5f };
        s3.s1.a = 0.5f;

        vi::gl::sprite s4;
        s4.init(nullptr);
        s4.s1.notexture = 1;
        s4.s2.pos = { 0.6f,-0.6f,0.6f };
        s4.s2.scale = { 0.5f,0.5f };
        s4.s1.a = 0.3f;

        vi::gl::sprite s5;
        s5.init(nullptr);
        s5.s1.notexture = 1;
        s5.s2.pos = { -0.4f,-0.4f,0.4f };
        s5.s2.scale = { 0.5f,0.5f };
        s5.s1.a = 0.5f;

        vi::gl::sprite s6;
        s6.init(nullptr);
        s6.s1.notexture = 1;
        s6.s2.pos = { 0.4f,-0.4f,0.4f };
        s6.s2.scale = { 0.5f,0.5f };
        s6.s1.a = 0.5f;

        vi::gl::sprite s7;
        s7.init(&t2);
        s7.s2.pos = { 0,0.5f,0.4f };

        vi::input::keyboard k;
        k.init();
        vi::input::mouse m;
        m.init();

        while (wnd.update())
        {
            k.update();
            m.update(&wnd, nullptr);

            g.beginScene();

            // draw back to front
            g.disableBlendState();
            g.drawSprite(&s1);
            g.enableBlendState();
            g.drawSprite(&s2);
            g.drawSprite(&s3);
            g.drawSprite(&s4);
            g.drawSprite(&s5);
            g.drawSprite(&s6);
            g.drawSprite(&s7);


            g.endScene();
        }

        g.destroy();
        wnd.destroy();
    }

    int main()
    {        
        mesh();
        mesh2();
        basicSprite();
        moreSprites();
        blendState();
        timerMotionAnimation();
        performance();
        keyboardMultipleAnimationsMath();
        multipleTextures();
        camera();
        text();
        inputState();
        typing();
        zindex();
        //mouseAndFixedSprite();
        //queue();
        //network();

        return 0;
    }
}

int main()
{
    return examples::main();
}