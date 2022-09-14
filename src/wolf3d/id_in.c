//
//  ID Engine
//  ID_IN.c - Input Manager
//  v1.0d1
//  By Jason Blochowiak
//

//
//  This module handles dealing with the various input devices
//
//  Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//              User Mgr (for command line parms)
//
//  Globals:
//      LastScan - The keyboard scan code of the last key pressed
//      LastASCII - The ASCII value of the last key pressed
//  DEBUG - there are more globals
//

#include "wl_def.h"


/*
=============================================================================

                    GLOBAL VARIABLES

=============================================================================
*/


//
// configuration variables
//
boolean MousePresent;
boolean forcegrabmouse;


//  Global variables
volatile boolean    Keyboard[256];
volatile boolean    Paused;
volatile char       LastASCII;
volatile ScanCode   LastScan;

//KeyboardDef   KbdDefs = {0x1d,0x38,0x47,0x48,0x49,0x4b,0x4d,0x4f,0x50,0x51};
static KeyboardDef KbdDefs = {
    sc_Control,             // button0
    sc_Alt,                 // button1
    sc_Home,                // upleft
    sc_UpArrow,             // up
    sc_PgUp,                // upright
    sc_LeftArrow,           // left
    sc_RightArrow,          // right
    sc_End,                 // downleft
    sc_DownArrow,           // down
    sc_PgDn                 // downright
};

/*
=============================================================================

                    LOCAL VARIABLES

=============================================================================
*/
byte        ASCIINames[] =      // Unshifted ASCII for scan codes       // TODO: keypad
{
//   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,9  ,'`',0  ,    // 0
    0  ,0  ,0  ,0  ,0  ,'q','1',0  ,0  ,0  ,'z','s','a','w','2',0  ,    // 1
    0  ,'c','x','d','e','4','3',0  ,0  ,' ','v','f','t','r','5',0  ,    // 2
    0  ,'n','b','h','g','y','6',0  ,0  ,0  ,'m','j','u','7','8',0  ,    // 3
    0  ,',','k','i','o','0','9',0  ,0  ,'.','/','l',';','p',0  ,0  ,    // 4
    0  ,0  ,39 ,0  ,'[','=',0  ,0  ,0  ,0  ,13 ,']',0  ,92 ,0  ,0  ,    // 5
    0  ,0  ,0  ,0  ,0  ,0  ,8  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 6
    0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,'+',0  ,'-','*',0  ,0  ,0  ,    // 7
};
byte ShiftNames[] =     // Shifted ASCII for scan codes
{
//   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,9  ,'~',0  ,    // 0
    0  ,0  ,0  ,0  ,0  ,'Q','!',0  ,0  ,0  ,'Z','S','A','W','@',0  ,    // 1
    0  ,'C','X','D','E','$','#',0  ,0  ,' ','V','F','T','R','%',0  ,    // 2
    0  ,'N','B','H','G','Y','^',0  ,0  ,0  ,'M','J','U','&','*',0  ,    // 3
    0  ,'<','K','I','O',')','(',0  ,0  ,'>','?','L',':','P',0  ,0  ,    // 4
    0  ,0  ,34 ,0  ,'{','+',0  ,0  ,0  ,0  ,13 ,'}',0  ,'|',0  ,0  ,    // 5
    0  ,0  ,0  ,0  ,0  ,0  ,8  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 6
    0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,'+',0  ,'-','*',0  ,0  ,0  ,    // 7
};
byte SpecialNames[] =   // ASCII for 0xe0 prefixed codes
{
//   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 0
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 1
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 2

    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 3
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,'/',0  ,0  ,0  ,0  ,0  ,    // 4
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,13 ,0  ,0  ,0  ,0  ,0  ,    // 5
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,    // 6
    0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0       // 7
};


static  boolean     IN_Started;
static  boolean     CapsLock;

static  Direction   DirTable[] =        // Quick lookup for total direction
{
    dir_NorthWest,  dir_North,  dir_NorthEast,
    dir_West,       dir_None,   dir_East,
    dir_SouthWest,  dir_South,  dir_SouthEast
};

void INL_KeyService(int data)
{
    static boolean special, key_down;
    byte k,c, temp;
    int i;

    k = (byte) data;

    if (k == 0xE0)
        special = true;
    else if (k == 0xF0)
        key_down = false;
    else if (k == 0xE1)
        Paused = true;
    else
    {
        if (!key_down)
        {
            // Keyboard sends e0,f0,{sc_LShift, sc_RShift} if the sequence of 'hold shift key then press special key'
            // is performed. This sequence shouldn't release the shift key, so bypass that case here.
            if (!special || ((k != sc_LShift) && (k != sc_RShift)))
            {
                Keyboard[k] = false;
            }
        }
        else
        {
            LastScan = k;
            Keyboard[k] = true;

            if (special)
                c = SpecialNames[k];
            else
            {
                if (k == sc_CapsLock)
                {
                    CapsLock ^= true;
                }

                if (Keyboard[sc_LShift] || Keyboard[sc_RShift])
                {
                    c = ShiftNames[k];
                    if ((c >= 'A') && (c <= 'Z') && CapsLock)
                        c += 'a' - 'A';
                }
                else
                {
                    c = ASCIINames[k];
                    if ((c >= 'A') && (c <= 'Z') && CapsLock)
                        c += 'a' - 'A';
                }
            }
            if (c)
                LastASCII = c;
        }

        special = false;
        key_down = true;
    }
}


///////////////////////////////////////////////////////////////////////////
//
//  INL_GetMouseButtons() - Gets the status of the mouse buttons from the
//      mouse driver
//
///////////////////////////////////////////////////////////////////////////
static int
INL_GetMouseButtons(void)
{
    int buttons = 0;
    int middlePressed = buttons;
    int rightPressed = buttons;
    if(middlePressed) buttons |= 1 << 2;
    if(rightPressed) buttons |= 1 << 1;

    return buttons;
}

///////////////////////////////////////////////////////////////////////////
//
//  IN_Startup() - Starts up the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Startup(void)
{
    if (IN_Started)
        return;

    IN_ClearKeysDown();

    MousePresent = false;
    
    IN_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//  IN_Shutdown() - Shuts down the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Shutdown(void)
{
    if (!IN_Started)
        return;

    IN_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//  IN_ClearKeysDown() - Clears the keyboard array
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearKeysDown(void)
{
    LastScan = sc_None;
    LastASCII = key_None;
    memset ((void *) Keyboard,0,sizeof(Keyboard));
}


///////////////////////////////////////////////////////////////////////////
//
//  IN_ReadControl() - Reads the device associated with the specified
//      player and fills in the control info struct
//
///////////////////////////////////////////////////////////////////////////
void
IN_ReadControl(int player,ControlInfo *info)
{
    word        buttons;
    int         dx,dy;
    Motion      mx,my;

    dx = dy = 0;
    mx = my = motion_None;
    buttons = 0;

    if (Keyboard[KbdDefs.upleft])
        mx = motion_Left,my = motion_Up;
    else if (Keyboard[KbdDefs.upright])
        mx = motion_Right,my = motion_Up;
    else if (Keyboard[KbdDefs.downleft])
        mx = motion_Left,my = motion_Down;
    else if (Keyboard[KbdDefs.downright])
        mx = motion_Right,my = motion_Down;

    if (Keyboard[KbdDefs.up])
        my = motion_Up;
    else if (Keyboard[KbdDefs.down])
        my = motion_Down;

    if (Keyboard[KbdDefs.left])
        mx = motion_Left;
    else if (Keyboard[KbdDefs.right])
        mx = motion_Right;

    if (Keyboard[KbdDefs.button0])
        buttons += 1 << 0;
    if (Keyboard[KbdDefs.button1])
        buttons += 1 << 1;

    dx = mx * 127;
    dy = my * 127;

    info->x = dx;
    info->xaxis = mx;
    info->y = dy;
    info->yaxis = my;
    info->button0 = (buttons & (1 << 0)) != 0;
    info->button1 = (buttons & (1 << 1)) != 0;
    info->button2 = (buttons & (1 << 2)) != 0;
    info->button3 = (buttons & (1 << 3)) != 0;
    info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

///////////////////////////////////////////////////////////////////////////
//
//  IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
///////////////////////////////////////////////////////////////////////////

boolean btnstate[NUMBUTTONS];

void IN_StartAck(void)
{
//
// get initial state of everything
//
    IN_ClearKeysDown();
    memset(btnstate, 0, sizeof(btnstate));

    int buttons = 0;

    if(MousePresent)
        buttons |= IN_MouseButtons();

    for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
        if(buttons & 1)
            btnstate[i] = true;
}


boolean IN_CheckAck (void)
{
//
// see if something has been pressed
//
    if(LastScan)
        return true;

    int buttons = 0;

    if(MousePresent)
        buttons |= IN_MouseButtons();

    for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
    {
        if(buttons & 1)
        {
            if(!btnstate[i])
            {
                // Wait until button has been released
                do
                {
                    if(MousePresent)
                        buttons |= IN_MouseButtons();
                }
                while(buttons & (1 << i));

                return true;
            }
        }
        else
            btnstate[i] = false;
    }

    return false;
}


void IN_Ack (void)
{
    IN_StartAck ();

    while (!IN_CheckAck ())
    ;
}


///////////////////////////////////////////////////////////////////////////
//
//  IN_UserInput() - Waits for the specified delay time (in ticks) or the
//      user pressing a key or a mouse button. If the clear flag is set, it
//      then either clears the key or waits for the user to let the mouse
//      button up.
//
///////////////////////////////////////////////////////////////////////////
boolean IN_UserInput(longword delay)
{
    longword    lasttime;

    lasttime = GetTimeCount();
    IN_StartAck ();
    do
    {
        if (IN_CheckAck())
            return true;
        delay_ms(5);
    } while (GetTimeCount() - lasttime < delay);
    return(false);
}

//===========================================================================

/*
===================
=
= IN_MouseButtons
=
===================
*/
int IN_MouseButtons (void)
{
    if (MousePresent)
        return INL_GetMouseButtons();
    else
        return 0;
}

void IN_CenterMouse()
{
}
