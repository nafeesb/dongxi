/*! @file flipbook.cpp
  @author Nafees Bin Zafar <nafees.bin.zafar@odw.com.cn>

  @brief Test of "flipbook" drawing speed. Striped pattern
  designed to show tearing artifacts.

  To run it you provide it with three arguments: width, height, and
  'd' for doublebuffer or 's' for singlebuffer:
  flipbook 640 480 d  # typical video playback
  flipbook 1024 554 d # typical 1/2 rez film with letterbox
  flipbook 1024 778 d # typical 1/2 rez full-ap film
  flipbook 2048 1107 d # typical full-rez film with letterbox
  flipbook 2048 1556 d # full rez full-ap film
  flipbook 1828 1371 d # full rez academy frame
  flipbook 1828 685 d  # anamorphic academy images, 1/2 rez vertically

  This can be used to detect tearing of the image due to lack of
  synchronization between the double buffer swaps and the screen. This
  should never produce these artifacts when double buffering is on, no
  matter where the window is placed on the screen.  However particular
  OpenGL implementations often require a driver or environment setting
  to enable redraw sync.

  Compilation instructions on Linux:
  g++ -O2 -DNDEBUG flipbook.cpp -o flipbook -L/usr/X11R6/lib -lGLEW -lGL -lX11 -lXext
*/

// #include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

/*======================================================================*/
#include <GL/glew.h>
#include <GL/glxew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <sys/time.h>
#include <X11/XKBlib.h>

using namespace std;

double elapsed_time(void) {
    static struct timeval prevclock;
    struct timeval newclock;
    double elapsed;
    gettimeofday(&newclock, NULL);
    elapsed = newclock.tv_sec - prevclock.tv_sec +
              (newclock.tv_usec - prevclock.tv_usec)/1000000.0;
    prevclock.tv_sec = newclock.tv_sec;
    prevclock.tv_usec = newclock.tv_usec;
    return elapsed;
}

/* color singlebuffer */
int clist[] = {
    GLX_RGBA,
    GLX_GREEN_SIZE,1,
    None
};

/* color doublebuffer */
int dlist[] = {
    GLX_RGBA,
    GLX_GREEN_SIZE,1,
    GLX_DOUBLEBUFFER,
    None
};

Display *dpy;
XVisualInfo *vis;
Window window;
GLenum format = GL_RGBA;

/* This creates the OpenGL window and makes the current OpenGL context
   draw into it. */
Window make_window(int width, int height, int doublebuf) {
    XSetWindowAttributes attr;
    Colormap cmap;
    GLXContext context;

    dpy = XOpenDisplay(0);
    if (not dpy) {
        throw runtime_error( "Can't open display\n" );
    }
    vis =  glXChooseVisual(dpy,DefaultScreen(dpy),doublebuf ? dlist : clist);
    if (!vis) {
        throw runtime_error("No such visual\n");
    }

    cmap = XCreateColormap(dpy,RootWindow(dpy,vis->screen),vis->visual,AllocNone);
    attr.border_pixel = 0;
    attr.colormap = cmap;
    attr.background_pixel = WhitePixel(dpy,DefaultScreen(dpy));
    attr.event_mask = KeyPressMask;
    window = XCreateWindow(dpy, RootWindow(dpy,DefaultScreen(dpy)),
                           0, 0, width, height, 0,
                           vis->depth, InputOutput, vis->visual,
                           CWBorderPixel|CWColormap|CWEventMask, &attr);
    XMapRaised(dpy,window);

    context = glXCreateContext(dpy, vis, 0, 1);
    glXMakeCurrent(dpy, window, context);

    // glXSwapIntervalEXT( dpy, window, 1 );
    // if (GLX_SGI_swap_control)
    //     if (glXSwapIntervalSGI(1))
    //         throw runtime_error("Failed to set swap interval.");
    
    return window;
}

bool do_system_stuff(void) {
    XEvent event;
    if (XCheckWindowEvent(dpy, window, KeyPressMask, &event)) {
        KeySym key = XkbKeycodeToKeysym( dpy, event.xkey.keycode, 0,0 );
        if( XK_Escape == key )
            return true;
    }
    return false;
}

void swap_buffers(void) {
    glXSwapBuffers(dpy, window);
}

/*======================================================================*/

namespace {
const int NUMIMAGES = 10;

const char* vertex_prog = "\
#version 150\n                                  \
in vec2 position;                               \
void main()                                     \
{                                               \
    gl_Position = vec4(position, 0.5, 1.0);     \
}";

const char* fragment_prog = "\
#version 150\n                                  \
out vec4 outColor;                              \
uniform vec2 dims;                              \
uniform sampler2D tex;                          \
void main() {                                   \
  vec2 uv;                                      \
  uv[0] = gl_FragCoord[0]/dims[0];              \
  uv[1] = 1.0f - (gl_FragCoord[1]/dims[1]);     \
  outColor = texture(tex, uv);                  \
}                                               \
";


struct shader_base {
protected:
    GLenum prog_type;
    GLuint shader_obj;

public:
    shader_base( const char* src, const GLenum& type )
        : prog_type(type),
          shader_obj(GL_FALSE)
    {
        shader_obj = glCreateShader( prog_type );
        glShaderSource( shader_obj, 1, &src, NULL );
        compile();
    }

    ~shader_base() { glDeleteShader( shader_obj ); }
    
    virtual void compile() {
        glCompileShader(shader_obj);

        GLint status;
        glGetShaderiv( shader_obj, GL_COMPILE_STATUS, &status );
        if (GL_TRUE != status) {
            char buffer[512];
            glGetShaderInfoLog( shader_obj, 512, NULL, buffer );
            throw runtime_error(buffer);
        }
    }

    virtual void attach( const GLuint program ) {
        glAttachShader( program, shader_obj );
    }
};

struct fragment_shader : public shader_base {
    fragment_shader( const char* src ) : shader_base( src, GL_FRAGMENT_SHADER ) {}
};

struct vertex_shader : public shader_base {
    vertex_shader( const char* src ) : shader_base( src, GL_VERTEX_SHADER ) {}
};


struct shader_program {
    GLuint handle;
    
    shader_program() {
        handle = glCreateProgram();
    }

    ~shader_program() { glDeleteProgram( handle ); }

    shader_program& attach( vertex_shader& vp ) {
        vp.attach( handle );
        return *this;
    }
    shader_program& attach( fragment_shader& fp ) {
        fp.attach( handle );
        glBindFragDataLocation( handle, 0, "outColor" );
        return *this;
    }
    
    shader_program& link() {
        glLinkProgram( handle );
        return *this;
    }

    void use() {
        glUseProgram( handle );
    }
};


struct framebuffer {
    GLuint handle;
    int width, height;
    
    framebuffer( int _w, int _h) : width(_w), height(_h) {
        glGenTextures( 1, &handle );
    }

    ~framebuffer() { glDeleteTextures( 1, &handle ); }
    
    void bind() {
        glBindTexture( GL_TEXTURE_2D, handle );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void draw(unsigned* img) {
        glTexImage2D( GL_TEXTURE_2D,
                      0, GL_RGBA, width, height,
                      0,
                      GL_RGBA, GL_UNSIGNED_BYTE, img );
    }
};

}



int main(int argc, char **argv) {

    int width;
    int height;
    int doublebuf;
    int i;
    unsigned* images;

    if (argc > 1) width = atoi(argv[1]);
    if (argc > 2) height = atoi(argv[2]);
    if (argc > 3) doublebuf = argv[3][0] == 'd';

    if (argc != 4 || width < 100 || height < 100) {
        cerr << "Usage: " << argv[0]
             << " <width> <height> <d or s for double or single buffer>"
             << endl;
        return 1;
    }

    // build the images
    {
        int i,x,y;
        unsigned* p;
        images = new unsigned[width * height * NUMIMAGES * 4];
        p = images;
        for (i = 0; i < NUMIMAGES; i++) {
            int bar = width*i/NUMIMAGES;
            int barw = width/NUMIMAGES;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if (x >= bar && x <= bar+barw) *p++ = 0xffffffff;
                    else *p++ = 0;
                }
            }
        }
    }

    
    try {
        make_window(width, height, doublebuf);

        glewExperimental = GL_TRUE;
        if( GLEW_OK != glewInit() )
            throw runtime_error("Failed to initialize GLEW");

        cout << "GL Vendor = "     << glGetString(GL_VENDOR)
             << "\nGL Renderer = " << glGetString(GL_RENDERER)
             << "\nGL Version = "  << glGetString(GL_VERSION)
             << endl;


        GLuint vao;
        glGenVertexArrays( 1, &vao );
        glBindVertexArray( vao );

        // vertex positions (X,Y)
        float vertices[] = {
            -1.f,  1.f,
             1.f,  1.f,
             1.f, -1.f,
            -1.f, -1.f,
        };

        // 2 triangles for the quad
        GLuint elements[] = {
            0, 1, 2,
            2, 3, 0
        };

        GLuint vbo;
        glGenBuffers( 1, &vbo );
        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW );

        GLuint ebo;
        glGenBuffers( 1, &ebo );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW );
        
        // compile shaders
        vertex_shader   vp(vertex_prog);
        fragment_shader fp(fragment_prog);
        
        // build program
        shader_program program;
        program.attach( vp )
            .attach(fp)
            .link()
            .use();
        
        // setup position attribute
        GLint pos_attr = glGetAttribLocation( program.handle, "position" );
        glEnableVertexAttribArray( pos_attr );
        glVertexAttribPointer( pos_attr, 2, GL_FLOAT, GL_FALSE,
                               0, 0 );

        // image dims so the fragment shader can calculate UVs
        GLint dim_attr = glGetUniformLocation( program.handle, "dims" );
        glUniform2f(dim_attr, float(width), float(height) );
        
        // texture buffer
        framebuffer frame(width, height);
        frame.bind();
        
        if (doublebuf) glDrawBuffer(GL_BACK);

        glClearColor( 0.f, 0.f, 0.f, 1.f );
        glClear( GL_COLOR_BUFFER_BIT );

        elapsed_time();

        for (i = 0; ; i++) {
            if (do_system_stuff()) break;
            
            if ((i%100)==99)
                cout << "FPS = " << 100.0/elapsed_time() << "\r"  << flush;

            frame.draw(images+(i%NUMIMAGES)*width*height);         // send texture
            glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 ); // draw geometry
            
            if (doublebuf) swap_buffers();
        }
        cout << endl;
        
        glDeleteBuffers( 1, &ebo );
        glDeleteBuffers( 1, &vbo );
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    delete [] images;
    return 0;
}
