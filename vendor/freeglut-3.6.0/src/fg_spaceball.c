/* Spaceball support for Linux.
 * Written by John Tsiombikas <nuclear@member.fsf.org>
 *
 * This code supports 3Dconnexion's 6-dof space-whatever devices.
 * It can communicate with either the proprietary 3Dconnexion daemon (3dxsrv)
 * free spacenavd (http://spacenav.sourceforge.net), through the "standard"
 * magellan X-based protocol.
 */


#include <GL/freeglut.h>
#include "fg_internal.h"

/* This game has no 3Dconnexion input binding.  On the MinGW SDK shipped
 * with Code::Blocks the WM_INPUT declaration can be available while the
 * matching raw-input backend is not, producing unresolved optional symbols.
 * Windows therefore uses FreeGLUT's existing no-device fallback below. */
#if( !defined(_WIN32) && !defined(FREEGLUT_DISABLE_SPACEBALL) )

/* -- PRIVATE FUNCTIONS --------------------------------------------------- */

extern void fgPlatformInitializeSpaceball(void);
extern void fgPlatformSpaceballClose(void);
extern int fgPlatformHasSpaceball(void);
extern int fgPlatformSpaceballNumButtons(void);
extern void fgPlatformSpaceballSetWindow(SFG_Window *window);


int fg_sball_initialized = 0;

void fgInitialiseSpaceball(void)
{
    if(fg_sball_initialized != 0) {
        return;
    }

    fgPlatformInitializeSpaceball();
}

void fgSpaceballClose(void)
{
    fgPlatformSpaceballClose();
}

int fgHasSpaceball(void)
{
    if(fg_sball_initialized == 0) {
        fgInitialiseSpaceball();
        if(fg_sball_initialized != 1) {
            fgWarning("fgInitialiseSpaceball failed\n");
            return 0;
        }
    }

    return fgPlatformHasSpaceball();
}

int fgSpaceballNumButtons(void)
{
    if(fg_sball_initialized == 0) {
        fgInitialiseSpaceball();
        if(fg_sball_initialized != 1) {
            fgWarning("fgInitialiseSpaceball failed\n");
            return 0;
        }
    }

    return fgPlatformSpaceballNumButtons();
}

void fgSpaceballSetWindow(SFG_Window *window)
{
    if(fg_sball_initialized == 0) {
        fgInitialiseSpaceball();
        if(fg_sball_initialized != 1) {
            return;
        }
    }

    fgPlatformSpaceballSetWindow(window);
}

#else

void fgInitialiseSpaceball(void)
{
}

void fgSpaceballClose(void)
{
}

int fgHasSpaceball(void)
{
    return 0;
}

int fgSpaceballNumButtons(void)
{
    return 0;
}

void fgSpaceballSetWindow(SFG_Window *window)
{
}

#endif
