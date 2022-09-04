//
//  ID Engine
//  ID_IN.h - Header file for Input Manager
//  v1.0d1
//  By Jason Blochowiak
//

#ifndef __ID_IN__
#define __ID_IN__

#ifdef  __DEBUG__
#define __DEBUG_InputMgr__
#endif

typedef byte     ScanCode;
#define sc_None         0
#define sc_Bad          0xFF
#define sc_Return       0x5A
#define sc_Enter        sc_Return
#define sc_Escape       0x76
#define sc_Space        0x29
#define sc_BackSpace    0x66
#define sc_Tab          0x0D
#define sc_Alt          0x11
#define sc_Control      0x14
#define sc_CapsLock     0x58
#define sc_LShift       0x12
#define sc_RShift       0x59
#define sc_UpArrow      0x75
#define sc_DownArrow    0x72
#define sc_LeftArrow    0x6B
#define sc_RightArrow   0x74
#define sc_Insert       0x70
#define sc_Delete       0x71
#define sc_Home         0x6C
#define sc_End          0x69
#define sc_PgUp         0x7D
#define sc_PgDn         0x7A
#define sc_F1           0x05
#define sc_F2           0x06
#define sc_F3           0x04
#define sc_F4           0x0C
#define sc_F5           0x03
#define sc_F6           0x0B
#define sc_F7           0x83
#define sc_F8           0x0A
#define sc_F9           0x01
#define sc_F10          0x09
#define sc_F11          0x78
#define sc_F12          0x07

#define sc_ScrollLock       sc_None
#define sc_PrintScreen      sc_None

#define sc_1            0x16
#define sc_2            0x1E
#define sc_3            0x26
#define sc_4            0x25
#define sc_5            0x2E
#define sc_6            0x36
#define sc_7            0x3D
#define sc_8            0x3E
#define sc_9            0x46
#define sc_0            0x45

#define sc_A            0x1C
#define sc_B            0x32
#define sc_C            0x21
#define sc_D            0x23
#define sc_E            0x24
#define sc_F            0x2B
#define sc_G            0x34
#define sc_H            0x33
#define sc_I            0x43
#define sc_J            0x3B
#define sc_K            0x42
#define sc_L            0x4B
#define sc_M            0x3A
#define sc_N            0x31
#define sc_O            0x44
#define sc_P            0x4D
#define sc_Q            0x15
#define sc_R            0x2D
#define sc_S            0x1B
#define sc_T            0x2C
#define sc_U            0x3C
#define sc_V            0x2A
#define sc_W            0x1D
#define sc_X            0x22
#define sc_Y            0x35
#define sc_Z            0x1A

#define key_None        0

typedef enum        {
                        demo_Off,demo_Record,demo_Playback,demo_PlayDone
                    } Demo;
typedef enum        {
                        ctrl_Keyboard,
                        ctrl_Keyboard1 = ctrl_Keyboard,ctrl_Keyboard2,
                        ctrl_Mouse
                    } ControlType;
typedef enum        {
                        motion_Left = -1,motion_Up = -1,
                        motion_None = 0,
                        motion_Right = 1,motion_Down = 1
                    } Motion;
typedef enum        {
                        dir_North,dir_NorthEast,
                        dir_East,dir_SouthEast,
                        dir_South,dir_SouthWest,
                        dir_West,dir_NorthWest,
                        dir_None
                    } Direction;
typedef struct      {
                        boolean     button0,button1,button2,button3;
                        short       x,y;
                        Motion      xaxis,yaxis;
                        Direction   dir;
                    } CursorInfo;
typedef CursorInfo  ControlInfo;
typedef struct      {
                        ScanCode    button0,button1,
                                    upleft,     up,     upright,
                                    left,               right,
                                    downleft,   down,   downright;
                    } KeyboardDef;
// Global variables
extern  volatile boolean    Keyboard[];;
extern           boolean    MousePresent;
extern  volatile boolean    Paused;
extern  volatile char       LastASCII;
extern  volatile ScanCode   LastScan;


// Function prototypes
#define IN_KeyDown(code)    (Keyboard[(code)])
#define IN_ClearKey(code)   {Keyboard[code] = false;\
                            if (code == LastScan) LastScan = sc_None;}

// DEBUG - put names in prototypes
extern  void        IN_Startup(void),IN_Shutdown(void);
extern  void        IN_ClearKeysDown(void);
extern  void        IN_ReadControl(int,ControlInfo *);
extern  void        IN_StopDemo(void),IN_FreeDemoBuffer(void),
                    IN_Ack(void);
extern  boolean     IN_UserInput(longword delay);
extern  const char *IN_GetScanName(ScanCode);

int     IN_MouseButtons (void);

void    IN_StartAck(void);
boolean IN_CheckAck (void);
void    IN_CenterMouse();

#endif
