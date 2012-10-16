#include <string.h>
#include <errno.h>
#include <sys/select.h>

#include <glwt_internal.h>

static int xlib_handle_event()
{
    XEvent event;
    int num_handled_events = 0;

    while(XCheckMaskEvent(glwt.x11.display, ~0, &event) ||
        XCheckTypedEvent(glwt.x11.display, ClientMessage, &event))
    {
        ++num_handled_events;

        GLWTWindow *win = 0;
        if(XFindContext(glwt.x11.display, event.xany.window, glwt.x11.xcontext, (XPointer*)&win) != 0 ||
            !win)
        {
            glwtErrorPrintf("XFindContext window not found");
            return -1;
        }

        switch(event.type)
        {
            case ConfigureNotify:
                if(win->win_callbacks.resize_callback)
                    win->win_callbacks.resize_callback(
                        win,
                        event.xconfigure.width, event.xconfigure.height,
                        win->win_callbacks.userdata);
                break;
            case MapNotify:
            case UnmapNotify:
                if(win->win_callbacks.show_callback)
                    win->win_callbacks.show_callback(
                        win,
                        event.type == MapNotify,
                        win->win_callbacks.userdata);
                break;
            case Expose:
                if(win->win_callbacks.expose_callback)
                    win->win_callbacks.expose_callback(
                        win,
                        win->win_callbacks.userdata);
                break;
            case KeyPress:
            case KeyRelease:
                if(win->win_callbacks.key_callback)
                    win->win_callbacks.key_callback(
                        win,
                        event.type == KeyPress,
                        keymap_lookup(
                            &glwt.x11.keymap,
                            XkbKeycodeToKeysym(glwt.x11.display, event.xkey.keycode, 0, 0)),
                        event.xkey.keycode,
                        0, // TODO: mod
                        win->win_callbacks.userdata);
                break;
            case FocusIn:
            case FocusOut:
                if(win->win_callbacks.focus_callback)
                    win->win_callbacks.focus_callback(
                        win,
                        event.type == FocusIn,
                        win->win_callbacks.userdata);
                break;
            case ButtonPress:
            case ButtonRelease:
                if(win->win_callbacks.button_callback)
                    win->win_callbacks.button_callback(
                        win,
                        event.type == ButtonPress,
                        event.xbutton.x, event.xbutton.y,
                        event.xbutton.button, // TODO: make these consistent on different platforms
                        0, // TODO: mod
                        win->win_callbacks.userdata);
                break;
            case MotionNotify:
                if(win->win_callbacks.motion_callback)
                    win->win_callbacks.motion_callback(
                        win,
                        event.xmotion.x, event.xmotion.y,
                        0, // TODO: buttons
                        win->win_callbacks.userdata);
                break;
            case EnterNotify:
            case LeaveNotify:
                if(win->win_callbacks.mouseover_callback)
                    win->win_callbacks.mouseover_callback(
                        win,
                        event.type == EnterNotify,
                        win->win_callbacks.userdata);
                break;
            case ClientMessage:
                if((Atom)event.xclient.data.l[0] == glwt.x11.atoms.WM_DELETE_WINDOW)
                {
                    if(win->win_callbacks.close_callback)
                        win->win_callbacks.close_callback(win, win->win_callbacks.userdata);
                    win->closed = 1;
                } else if((Atom)event.xclient.data.l[0] == glwt.x11.atoms._NET_WM_PING)
                {
                    event.xclient.window = RootWindow(glwt.x11.display, glwt.x11.screen_num);
                    XSendEvent(
                        glwt.x11.display,
                        event.xclient.window,
                        False,
                       SubstructureNotifyMask | SubstructureRedirectMask,
                       &event);
                }
                break;
            default:
                break;
        }
    }

    return num_handled_events;
}

int glwtEventHandle(int wait)
{
    int handled = 0;
    do
    {
        XFlush(glwt.x11.display);

        handled = xlib_handle_event();
        if(handled < 0)
            return -1;

        if(wait && handled == 0)
        {
            int fd;
            fd_set fds;

            fd = ConnectionNumber(glwt.x11.display);

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            int val = select(fd + 1, &fds, NULL, NULL, NULL);
            if(val == -1)
            {
                glwtErrorPrintf("select failed: %s", strerror(errno));
                return -1;
            } else if(val == 0)
                return 0;
        }
    } while(handled == 0 && wait);

    return 0;
}