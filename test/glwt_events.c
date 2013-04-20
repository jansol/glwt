#include <stdio.h>

#include <GLWT/glwt.h>

#ifdef GLWT_TESTS_GLES
#include <GLXW/glxw_es2.h>
#else
#include <GLXW/glxw.h>
#endif

static GLWTWindow *windows[16] = {0};
static int windowsOpen = 0;

static int newWindow();

static void error_callback(const char *msg, void *userdata)
{
    (void)userdata;
    fprintf(stderr, "%s\n", msg);
}

static void window_callback(GLWTWindow *window, const GLWTWindowEvent *event, void *userdata)
{
    (void)userdata;

    switch(event->type)
    {
        case GLWT_WINDOW_CLOSE:
            printf("Window %p closed\n", window);
            break;
        case GLWT_WINDOW_EXPOSE:
            printf("Window %p exposed\n", window);
            {
                glwtMakeCurrent(window);
                glClearColor(0.2f, 0.4f, 0.7f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glwtSwapBuffers(window);
                glwtMakeCurrent(NULL);
            }
            break;
        case GLWT_WINDOW_RESIZE:
            printf("Window %p resized  width: %d  height: %d\n", window, event->resize.width, event->resize.height);
            break;
        case GLWT_WINDOW_SHOW:
        case GLWT_WINDOW_HIDE:
            printf("Window %p %s\n", window, (event->type == GLWT_WINDOW_SHOW) ? "show" : "hide");
            break;
        case GLWT_WINDOW_FOCUS_IN:
        case GLWT_WINDOW_FOCUS_OUT:
            printf("Window %p focus %s\n", window, (event->type == GLWT_WINDOW_FOCUS_IN) ? "in" : "out");
            break;
        case GLWT_WINDOW_KEY_UP:
        case GLWT_WINDOW_KEY_DOWN:
            printf("Window %p key %s  keysym: 0x%x  scancode: %d  mod: %X\n", window,
                (event->type == GLWT_WINDOW_KEY_DOWN) ? "down" : "up",
                event->key.keysym, event->key.scancode, event->key.mod);
            if(event->type == GLWT_WINDOW_KEY_DOWN && event->key.keysym == GLWT_KEY_RETURN)
            {
                newWindow();
            }
            break;
        case GLWT_WINDOW_BUTTON_UP:
        case GLWT_WINDOW_BUTTON_DOWN:
            printf("Window %p button %s  x: %d  y: %d  button: %d  mod: %X\n", window,
                (event->type == GLWT_WINDOW_BUTTON_DOWN) ? "down" : "up",
                event->button.x, event->button.y, event->button.button, event->button.mod);
            break;
        case GLWT_WINDOW_MOUSE_MOTION:
            printf("Window %p motion  x: %d  y: %d  buttons: %X\n", window,
                event->motion.x, event->motion.y, event->motion.buttons);
            break;
        case GLWT_WINDOW_MOUSE_ENTER:
        case GLWT_WINDOW_MOUSE_LEAVE:
            printf("Window %p mouse %s\n", window, (event->type == GLWT_WINDOW_MOUSE_ENTER) ? "enter" : "leave");
            break;
        default:
            break;
    }
}

static int newWindow()
{
    int width, height;
    int id = 0;
    for (int i = 0; i < 16; ++i)
    {
        if(!windows[i])
        {
            windows[i] = glwtWindowCreate("", 400, 300, NULL, window_callback, NULL);
            if(!windows[i])
            {
                printf("Could not open a window!\n");
                return 0;
            }
            id = i;
            break;
        }
    }

    ++windowsOpen;

    glwtWindowSetTitle(windows[id], "GLWT Events test");

    glwtWindowShow(windows[id], 1);
    glwtMakeCurrent(windows[id]);
    glwtSwapInterval(windows[id], 1);

    glxwInit();

    printf("%s\n", (const char*)glGetString(GL_VERSION));

    glwtWindowGetSize(windows[id], &width, &height);
    printf("Window %p size: %d x %d\n", windows[id], width, height);

    return 1;
}

int main(int argc, char *argv[])
{
    int err = -1;
    GLWTConfig glwt_config = {
        0, 0, 0, 0,
        0, 0,
        0, 0,
#ifdef GLWT_TESTS_GLES
        GLWT_API_OPENGL_ES | GLWT_PROFILE_DEBUG,
        2, 0,
#else
        GLWT_API_OPENGL | GLWT_PROFILE_DEBUG,
        0, 0,
#endif
    };

    (void)argc; (void)argv;

    if(glwtInit(&glwt_config, error_callback, NULL) != 0)
        goto error;

    if(!newWindow())
        goto error;

    while(windowsOpen)
    {
        if(glwtEventHandle(1) != 0)
            goto error;

        for (int i = 0; i < 16; ++i)
        {
            if(windows[i] && glwtWindowClosed(windows[i]))
            {
                glwtWindowDestroy(windows[i]);
                --windowsOpen;
                windows[i] = 0;
            }
        }
    }

    glwtMakeCurrent(0);

    err = 0;
error:
    for (int i = 0; i < 16; ++i)
    {
        if(windows[i])
            glwtWindowDestroy(windows[i]);
    }
    glwtQuit();
    return err;
}
