/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL.h>

#include "global.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "simcoupec.h"
#include "psp_irkeyb.h"

enum eSamKey
{
    SK_MIN=0, SK_SHIFT=SK_MIN, SK_Z, SK_X, SK_C, SK_V, SK_F1, SK_F2, SK_F3,
    SK_A, SK_S, SK_D, SK_F, SK_G, SK_F4, SK_F5, SK_F6,
    SK_Q, SK_W, SK_E, SK_R, SK_T, SK_F7, SK_F8, SK_F9,
    SK_1, SK_2, SK_3, SK_4, SK_5, SK_ESCAPE, SK_TAB, SK_CAPS,
    SK_0, SK_9, SK_8, SK_7, SK_6, SK_MINUS, SK_PLUS, SK_DELETE,
    SK_P, SK_O, SK_I, SK_U, SK_Y, SK_EQUALS, SK_QUOTES, SK_F0,
    SK_RETURN, SK_L, SK_K, SK_J, SK_H, SK_SEMICOLON, SK_COLON, SK_EDIT,
    SK_SPACE, SK_SYMBOL, SK_M, SK_N, SK_B, SK_COMMA, SK_PERIOD, SK_INV,
    SK_CONTROL, SK_UP, SK_DOWN, SK_LEFT, SK_RIGHT, SK_NONE, SK_MAX=SK_NONE
};


# define KBD_MIN_ANALOG_TIME  150000
# define KBD_MIN_START_TIME  3000000
# define KBD_MAX_EVENT_TIME   500000
# define KBD_MIN_PENDING_TIME 100000
# define KBD_MIN_DANZEFF_TIME 150000
# define KBD_MIN_COMMAND_TIME 100000
# define KBD_MIN_BOOT_TIME   1000000

# define KBD_MIN_IR_PENDING_TIME  400000
# define KBD_MIN_IR_TIME          150000

# define KBD_MIN_BATTCHECK_TIME 90000000 

 static SceCtrlData    loc_button_data;
 static unsigned int   loc_last_event_time = 0;
#ifdef USE_PSP_IRKEYB
 static unsigned int   loc_last_irkbd_event_time = 0;
#endif
 static unsigned int   loc_last_analog_time = 0;
 static int            loc_analog_x_released = 0;
 static int            loc_analog_y_released = 0;
 static long           first_time_stamp = -1;
 static char           loc_button_press[ KBD_MAX_BUTTONS ]; 
 static char           loc_button_release[ KBD_MAX_BUTTONS ]; 
 static unsigned int   loc_button_mask[ KBD_MAX_BUTTONS ] =
 {
   PSP_CTRL_UP         , /*  KBD_UP         */
   PSP_CTRL_RIGHT      , /*  KBD_RIGHT      */
   PSP_CTRL_DOWN       , /*  KBD_DOWN       */
   PSP_CTRL_LEFT       , /*  KBD_LEFT       */
   PSP_CTRL_TRIANGLE   , /*  KBD_TRIANGLE   */
   PSP_CTRL_CIRCLE     , /*  KBD_CIRCLE     */
   PSP_CTRL_CROSS      , /*  KBD_CROSS      */
   PSP_CTRL_SQUARE     , /*  KBD_SQUARE     */
   PSP_CTRL_SELECT     , /*  KBD_SELECT     */
   PSP_CTRL_START      , /*  KBD_START      */
   PSP_CTRL_HOME       , /*  KBD_HOME       */
   PSP_CTRL_HOLD       , /*  KBD_HOLD       */
   PSP_CTRL_LTRIGGER   , /*  KBD_LTRIGGER   */
   PSP_CTRL_RTRIGGER   , /*  KBD_RTRIGGER   */
 };

 static char loc_button_name[ KBD_ALL_BUTTONS ][20] =
 {
   "UP",
   "RIGHT",
   "DOWN",
   "LEFT",
   "TRIANGLE",
   "CIRCLE",
   "CROSS",
   "SQUARE",
   "SELECT",
   "START",
   "HOME",
   "HOLD",
   "LTRIGGER",
   "RTRIGGER",
   "JOY_UP",
   "JOY_RIGHT",
   "JOY_DOWN",
   "JOY_LEFT"
 };

 static char loc_button_name_L[ KBD_ALL_BUTTONS ][20] =
 {
   "L_UP",
   "L_RIGHT",
   "L_DOWN",
   "L_LEFT",
   "L_TRIANGLE",
   "L_CIRCLE",
   "L_CROSS",
   "L_SQUARE",
   "L_SELECT",
   "L_START",
   "L_HOME",
   "L_HOLD",
   "L_LTRIGGER",
   "L_RTRIGGER",
   "L_JOY_UP",
   "L_JOY_RIGHT",
   "L_JOY_DOWN",
   "L_JOY_LEFT"
 };
 
  static char loc_button_name_R[ KBD_ALL_BUTTONS ][20] =
 {
   "R_UP",
   "R_RIGHT",
   "R_DOWN",
   "R_LEFT",
   "R_TRIANGLE",
   "R_CIRCLE",
   "R_CROSS",
   "R_SQUARE",
   "R_SELECT",
   "R_START",
   "R_HOME",
   "R_HOLD",
   "R_LTRIGGER",
   "R_RTRIGGER",
   "R_JOY_UP",
   "R_JOY_RIGHT",
   "R_JOY_DOWN",
   "R_JOY_LEFT"
 };

  struct sim_key_trans psp_sim_key_to_sam_key[SIM_MAX_KEY]=
  {
    // SIM            SAM             MOD         KEY NAME 
    { SIM_UNDERSCORE, SK_EQUALS,     SK_SHIFT,     "_" },
    { SIM_1,          SK_1,           SK_NONE,     "1" },
    { SIM_2,          SK_2,           SK_NONE,     "2" },
    { SIM_3,          SK_3,           SK_NONE,     "3" },
    { SIM_4,          SK_4,           SK_NONE,     "4" },
    { SIM_5,          SK_5,           SK_NONE,     "5" },
    { SIM_6,          SK_6,           SK_NONE,     "6" },
    { SIM_7,          SK_7,           SK_NONE,     "7" },
    { SIM_8,          SK_8,           SK_NONE,     "8" },
    { SIM_9,          SK_9,           SK_NONE,     "9" },
    { SIM_0,          SK_0,           SK_NONE,     "0" },
    { SIM_SEMICOLON,  SK_SEMICOLON,   SK_NONE,     ";" },
    { SIM_MINUS    ,  SK_MINUS,       SK_NONE,     "-" },
    { SIM_DELETE,     SK_DELETE,      SK_NONE,     "DELETE" },
    { SIM_POUND,      SK_L,           SK_SYMBOL,   "POUND" },
    { SIM_EXCLAMATN,  SK_1,           SK_SYMBOL,   "!" },
    { SIM_DBLQUOTE,   SK_P,           SK_SYMBOL,   "\"" },
    { SIM_HASH,       SK_3,           SK_SYMBOL,   "#" },
    { SIM_DOLLAR,     SK_4,           SK_SYMBOL,   "$" },
    { SIM_PERCENT,    SK_5,           SK_SYMBOL,   "%" },
    { SIM_AMPERSAND,  SK_6,           SK_SYMBOL,   "&" },
    { SIM_QUOTE,      SK_7,           SK_SHIFT,    "'" },
    { SIM_LEFTPAREN,  SK_8,           SK_SHIFT,    "(" },
    { SIM_RIGHTPAREN, SK_9,           SK_SHIFT,    ")" },
    { SIM_PLUS,       SK_PLUS,        SK_NONE,     "+" },
    { SIM_EQUAL,      SK_EQUALS,      SK_NONE,     "=" },
    { SIM_TAB,        SK_TAB,         SK_NONE,     "TAB  " },
    { SIM_a,          SK_A,           SK_NONE,     "a" },
    { SIM_b,          SK_B,           SK_NONE,     "b" },
    { SIM_c,          SK_C,           SK_NONE,     "c" },
    { SIM_d,          SK_D,           SK_NONE,     "d" },
    { SIM_e,          SK_E,           SK_NONE,     "e" },
    { SIM_f,          SK_F,           SK_NONE,     "f" },
    { SIM_g,          SK_G,           SK_NONE,     "g" },
    { SIM_h,          SK_H,           SK_NONE,     "h" },
    { SIM_i,          SK_I,           SK_NONE,     "i" },
    { SIM_j,          SK_J,           SK_NONE,     "j" },
    { SIM_k,          SK_K,           SK_NONE,     "k" },
    { SIM_l,          SK_L,           SK_NONE,     "l" },
    { SIM_m,          SK_M,           SK_NONE,     "m" },
    { SIM_n,          SK_N,           SK_NONE,     "n" },
    { SIM_o,          SK_O,           SK_NONE,     "o" },
    { SIM_p,          SK_P,           SK_NONE,     "p" },
    { SIM_q,          SK_Q,           SK_NONE,     "q" },
    { SIM_r,          SK_R,           SK_NONE,     "r" },
    { SIM_s,          SK_S,           SK_NONE,     "s" },
    { SIM_t,          SK_T,           SK_NONE,     "t" },
    { SIM_u,          SK_U,           SK_NONE,     "u" },
    { SIM_v,          SK_V,           SK_NONE,     "v" },
    { SIM_w,          SK_W,           SK_NONE,     "w" },
    { SIM_x,          SK_X,           SK_NONE,     "x" },
    { SIM_y,          SK_Y,           SK_NONE,     "y" },
    { SIM_z,          SK_Z,           SK_NONE,     "z" },
    { SIM_A,          SK_A,           SK_SHIFT,    "A" },
    { SIM_B,          SK_B,           SK_SHIFT,    "B" },
    { SIM_C,          SK_C,           SK_SHIFT,    "C" },
    { SIM_D,          SK_D,           SK_SHIFT,    "D" },
    { SIM_E,          SK_E,           SK_SHIFT,    "E" },
    { SIM_F,          SK_F,           SK_SHIFT,    "F" },
    { SIM_G,          SK_G,           SK_SHIFT,    "G" },
    { SIM_H,          SK_H,           SK_SHIFT,    "H" },
    { SIM_I,          SK_I,           SK_SHIFT,    "I" },
    { SIM_J,          SK_J,           SK_SHIFT,    "J" },
    { SIM_K,          SK_K,           SK_SHIFT,    "K" },
    { SIM_L,          SK_L,           SK_SHIFT,    "L" },
    { SIM_M,          SK_M,           SK_SHIFT,    "M" },
    { SIM_N,          SK_N,           SK_SHIFT,    "N" },
    { SIM_O,          SK_O,           SK_SHIFT,    "O" },
    { SIM_P,          SK_P,           SK_SHIFT,    "P" },
    { SIM_Q,          SK_Q,           SK_SHIFT,    "Q" },
    { SIM_R,          SK_R,           SK_SHIFT,    "R" },
    { SIM_S,          SK_S,           SK_SHIFT,    "S" },
    { SIM_T,          SK_T,           SK_SHIFT,    "T" },
    { SIM_U,          SK_U,           SK_SHIFT,    "U" },
    { SIM_V,          SK_V,           SK_SHIFT,    "V" },
    { SIM_W,          SK_W,           SK_SHIFT,    "W" },
    { SIM_X,          SK_X,           SK_SHIFT,    "X" },
    { SIM_Y,          SK_Y,           SK_SHIFT,    "Y" },
    { SIM_Z,          SK_Z,           SK_SHIFT,    "Z" },
    { SIM_RETURN,     SK_RETURN,      SK_NONE,     "RETURN" },
    { SIM_CONTROL,    SK_CONTROL,     SK_NONE,     "CONTROL" },
    { SIM_SHIFT,      SK_SHIFT,       SK_NONE,     "SHIFT" },
    { SIM_CAPSLOCK,   SK_CAPS,        SK_NONE,     "CAPSLOCK" },
    { SIM_ESC,        SK_ESCAPE,      SK_NONE,     "ESC" },
    { SIM_SPACE,      SK_SPACE,       SK_NONE,     "SPACE" },
    { SIM_LEFT,       SK_LEFT,        SK_NONE,     "LEFT" },
    { SIM_UP,         SK_UP,          SK_NONE,     "UP" },
    { SIM_RIGHT,      SK_RIGHT,       SK_NONE,     "RIGHT" },
    { SIM_DOWN,       SK_DOWN,        SK_NONE,     "DOWN" },
    { SIM_F0,         SK_F0,          SK_NONE,     "F0" },
    { SIM_F1,         SK_F1,          SK_NONE,     "F1" },
    { SIM_F2,         SK_F2,          SK_NONE,     "F2" },
    { SIM_F3,         SK_F3,          SK_NONE,     "F3" },
    { SIM_F4,         SK_F4,          SK_NONE,     "F4" },
    { SIM_F5,         SK_F5,          SK_NONE,     "F5" },
    { SIM_F6,         SK_F6,          SK_NONE,     "F6" },
    { SIM_F7,         SK_F7,          SK_NONE,     "F7" },
    { SIM_F8,         SK_F8,          SK_NONE,     "F8" },
    { SIM_F9,         SK_F9,          SK_NONE,     "F9" },
    { SIM_AT,         SK_2,           SK_SHIFT,    "@" },
    { SIM_COLON,      SK_COLON,       SK_NONE,     ":" },
    { SIM_COMMA,      SK_COMMA,       SK_NONE,     "," },
    { SIM_PERIOD,     SK_PERIOD,      SK_NONE,     "." },
    { SIM_SLASH,      SK_MINUS,       SK_SHIFT,    "/" },
    { SIM_ASTERISK,   SK_PLUS,        SK_SHIFT,    "*" },
    { SIM_LESS,       SK_Q,           SK_SYMBOL,   "<" },
    { SIM_GREATER,    SK_W,           SK_SYMBOL,   ">" },
    { SIM_QUESTION,   SK_X,           SK_SYMBOL,   "?" },
    { SIM_PIPE,       SK_9,           SK_SYMBOL,   "|" },
    { SIM_RCBRACE,    SK_G,           SK_SYMBOL,   "}" },
    { SIM_RBRACKET,   SK_T,           SK_SYMBOL,   "]" },
    { SIM_LBRACKET,   SK_R,           SK_SYMBOL,   "[" },
    { SIM_LCBRACE,    SK_F,           SK_SYMBOL,   "{" },
    { SIM_BACKSLASH,  SK_INV,         SK_SHIFT,    "\\" },
    { SIM_BACKQUOTE,  SK_7,           SK_SHIFT,    "`"  },
    { SIM_POWER,      SK_H,           SK_SYMBOL,   "^" },
    { SIM_C_FPS,      0, SK_NONE,     "C_FPS" },
    { SIM_C_JOY,      0, SK_NONE,     "C_JOY" },
    { SIM_C_RENDER,   0, SK_NONE,     "C_RENDER" },
    { SIM_C_INCX  ,   0, SK_NONE,     "C_INCX" },
    { SIM_C_INCY  ,   0, SK_NONE,     "C_INCY" },
    { SIM_C_DECX  ,   0, SK_NONE,     "C_DECX" },
    { SIM_C_DECY  ,   0, SK_NONE,     "C_DECY" },
    { SIM_C_SCREEN,   0, SK_NONE,     "C_SCREEN" }
  };

  static int loc_default_mapping[ KBD_ALL_BUTTONS ] = {
    SIM_UP              , /*  KBD_UP         */
    SIM_RIGHT           , /*  KBD_RIGHT      */
    SIM_DOWN            , /*  KBD_DOWN       */
    SIM_LEFT            , /*  KBD_LEFT       */
    SIM_RETURN          , /*  KBD_TRIANGLE   */
    SIM_ESC             , /*  KBD_CIRCLE     */
    SIM_SPACE           , /*  KBD_CROSS      */
    SIM_DELETE          , /*  KBD_SQUARE     */
    -1                  , /*  KBD_SELECT     */
    -1                  , /*  KBD_START      */
    -1                  , /*  KBD_HOME       */
    -1                  , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    SIM_A               , /*  KBD_JOY_UP     */
    SIM_7               , /*  KBD_JOY_RIGHT  */
    SIM_B               , /*  KBD_JOY_DOWN   */
    SIM_6                 /*  KBD_JOY_LEFT   */
  };

  static int loc_default_mapping_L[ KBD_ALL_BUTTONS ] = {
    SIM_C_DECY          , /*  KBD_UP         */
    SIM_C_INCX          , /*  KBD_RIGHT      */
    SIM_C_INCY          , /*  KBD_DOWN       */
    SIM_C_DECX          , /*  KBD_LEFT       */
    SIM_C_RENDER        , /*  KBD_TRIANGLE   */
    SIM_C_JOY           , /*  KBD_CIRCLE     */
    SIM_C_RENDER        , /*  KBD_CROSS      */
    SIM_C_FPS           , /*  KBD_SQUARE     */
    -1                  , /*  KBD_SELECT     */
    -1                  , /*  KBD_START      */
    -1                  , /*  KBD_HOME       */
    -1                  , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    SIM_A               , /*  KBD_JOY_UP     */
    SIM_7               , /*  KBD_JOY_RIGHT  */
    SIM_B               , /*  KBD_JOY_DOWN   */
    SIM_6                 /*  KBD_JOY_LEFT   */
  };

  static int loc_default_mapping_R[ KBD_ALL_BUTTONS ] = {
    SIM_UP              , /*  KBD_UP         */
    SIM_RIGHT           , /*  KBD_RIGHT      */
    SIM_DOWN            , /*  KBD_DOWN       */
    SIM_LEFT            , /*  KBD_LEFT       */
    SIM_RETURN          , /*  KBD_TRIANGLE   */
    SIM_ESC             , /*  KBD_CIRCLE     */
    SIM_F9              , /*  KBD_CROSS      */
    SIM_SPACE           , /*  KBD_SQUARE     */
    -1                  , /*  KBD_SELECT     */
    -1                  , /*  KBD_START      */
    -1                  , /*  KBD_HOME       */
    -1                  , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    SIM_A               , /*  KBD_JOY_UP     */
    SIM_7               , /*  KBD_JOY_RIGHT  */
    SIM_B               , /*  KBD_JOY_DOWN   */
    SIM_6                 /*  KBD_JOY_LEFT   */
  };

 int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
 int psp_kbd_presses[ KBD_ALL_BUTTONS ];
 int kbd_ltrigger_mapping_active;
 int kbd_rtrigger_mapping_active;

# define KBD_MAX_ENTRIES   114


  int kbd_layout[KBD_MAX_ENTRIES][2] = {
    /* Key            Ascii */
    { SIM_0,          '0' },
    { SIM_1,          '1' },
    { SIM_2,          '2' },
    { SIM_3,          '3' },
    { SIM_4,          '4' },
    { SIM_5,          '5' },
    { SIM_6,          '6' },
    { SIM_7,          '7' },
    { SIM_8,          '8' },
    { SIM_9,          '9' },
    { SIM_A,          'A' },
    { SIM_B,          'B' },
    { SIM_C,          'C' },
    { SIM_D,          'D' },
    { SIM_E,          'E' },
    { SIM_F,          'F' },
    { SIM_G,          'G' },
    { SIM_H,          'H' },
    { SIM_I,          'I' },
    { SIM_J,          'J' },
    { SIM_K,          'K' },
    { SIM_L,          'L' },
    { SIM_M,          'M' },
    { SIM_N,          'N' },
    { SIM_O,          'O' },
    { SIM_P,          'P' },
    { SIM_Q,          'Q' },
    { SIM_R,          'R' },
    { SIM_S,          'S' },
    { SIM_T,          'T' },
    { SIM_U,          'U' },
    { SIM_V,          'V' },
    { SIM_W,          'W' },
    { SIM_X,          'X' },
    { SIM_Y,          'Y' },
    { SIM_Z,          'Z' },
    { SIM_a,          'a' },
    { SIM_b,          'b' },
    { SIM_c,          'c' },
    { SIM_d,          'd' },
    { SIM_e,          'e' },
    { SIM_f,          'f' },
    { SIM_g,          'g' },
    { SIM_h,          'h' },
    { SIM_i,          'i' },
    { SIM_j,          'j' },
    { SIM_k,          'k' },
    { SIM_l,          'l' },
    { SIM_m,          'm' },
    { SIM_n,          'n' },
    { SIM_o,          'o' },
    { SIM_p,          'p' },
    { SIM_q,          'q' },
    { SIM_r,          'r' },
    { SIM_s,          's' },
    { SIM_t,          't' },
    { SIM_u,          'u' },
    { SIM_v,          'v' },
    { SIM_w,          'w' },
    { SIM_x,          'x' },
    { SIM_y,          'y' },
    { SIM_z,          'z' },
    { SIM_DELETE,     DANZEFF_DEL },
    { SIM_SPACE,      ' '         },
    { SIM_F0,         DANZEFF_F0  },
    { SIM_F1,         DANZEFF_F1  },
    { SIM_F2,         DANZEFF_F2  },
    { SIM_F3,         DANZEFF_F3  },
    { SIM_F4,         DANZEFF_F4  },
    { SIM_F5,         DANZEFF_F5  },
    { SIM_F6,         DANZEFF_F6  },
    { SIM_F7,         DANZEFF_F7  },
    { SIM_F8,         DANZEFF_F8  },
    { SIM_F9,         DANZEFF_F9  },
    { SIM_CAPSLOCK,   DANZEFF_CAPSLOCK },
    { SIM_RETURN,     DANZEFF_RETURN   },
    { SIM_SHIFT,      DANZEFF_SHIFT    },
    { SIM_TAB,        DANZEFF_TAB      },
    { SIM_AMPERSAND,  '&' },
    { SIM_ASTERISK,   '*' },
    { SIM_AT,         '@' },
    { SIM_COLON,      ':' },
    { SIM_COMMA,      ',' },
    { SIM_CONTROL,    DANZEFF_CONTROL  },
    { SIM_DOWN,       -1  },
    { SIM_LEFT,       -1  },
    { SIM_RIGHT,      -1  },
    { SIM_UP,         -1  },
    { SIM_DBLQUOTE,   '"' },
    { SIM_QUOTE,      '\'' },
    { SIM_DOLLAR,     '$' },
    { SIM_EQUAL,      '=' },
    { SIM_ESC,        DANZEFF_ESC   },
    { SIM_EXCLAMATN,  '!' },
    { SIM_GREATER,    '>' },
    { SIM_HASH,       '#' },
    { SIM_LEFTPAREN,  '(' },
    { SIM_LESS,       '<' },
    { SIM_MINUS,      '-' },
    { SIM_PERCENT,    '%' },
    { SIM_PERIOD,     '.' },
    { SIM_PLUS,       '+' },
    { SIM_QUESTION,   '?' },
    { SIM_RIGHTPAREN, ')' },
    { SIM_SEMICOLON,  ';' },
    { SIM_SLASH,      '/' },
    { SIM_UNDERSCORE, '_'  },
    { SIM_PIPE,       '|' },
    { SIM_RCBRACE,    '}' },
    { SIM_RBRACKET,   ']' },
    { SIM_LBRACKET,   '[' },
    { SIM_LCBRACE,    '{' },
    { SIM_BACKSLASH,  '\\' },
    { SIM_POWER,      '^' }
  };

        int psp_kbd_mapping[ KBD_ALL_BUTTONS ];

 static int danzeff_sim_key     = 0;
 static int danzeff_sim_pending = 0;
 static int danzeff_mode        = 0;

#ifdef USE_PSP_IRKEYB
 static int irkeyb_sim_key      = 0;
 static int irkeyb_sim_pending  = 0;
# endif


       char command_keys[ 128 ];
 static int command_mode        = 0;
 static int command_index       = 0;
 static int command_size        = 0;
 static int command_sim_pending = 0;
 static int command_sim_key     = 0;

int
sim_key_event(int sim_idx, int press)
{
  int sam_key;
  int sam_mod;
  int shift;

  if ((sim_idx >= SIM_C_FPS) &&
      (sim_idx <= SIM_C_SCREEN)) {
    if (press) {
      sim_treat_command_key(sim_idx);
    }
    return 0;
  }

  if ((sim_idx >=          0) && 
      (sim_idx < SIM_MAX_KEY)) {
    sam_key = psp_sim_key_to_sam_key[sim_idx].sam_key;
    sam_mod = psp_sim_key_to_sam_key[sim_idx].sam_mod;


    if (press) {
      sim_key_down(sam_mod);
      sim_key_down(sam_key);
# if 0 
      if ((sim_idx != SIM_CAPSLOCK) && 
          (sim_idx != SIM_SHIFT   )) {
        if (shift) SimKeyDown(0,0);
      }
# endif
    } else {
      sim_key_up(sam_mod);
      sim_key_up(sam_key);
# if 0
      if ((sim_idx != SIM_CAPSLOCK) && 
          (sim_idx != SIM_SHIFT   )) {
        if (shift) SimKeyUp(0,0);
      }
# endif
    }
  }
  return 0;
}
int 
sim_kbd_reset()
{
  sim_release_all_key();
  return 0;
}

int
sim_get_key_from_ascii(int key_ascii)
{
  int index;
  for (index = 0; index < KBD_MAX_ENTRIES; index++) {
   if (kbd_layout[index][1] == key_ascii) return kbd_layout[index][0];
  }
  return -1;
}

void
psp_kbd_run_command(char *Command)
{
  strncpy(command_keys, Command, 128);
  command_size  = strlen(Command);
  command_index = 0;

  command_sim_key     = 0;
  command_sim_pending = 0;
  command_mode         = 1;
}

int
psp_kbd_reset_mapping(void)
{
  memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));
  return 0;
}

int
psp_kbd_reset_hotkeys(void)
{
  int index;
  int key_id;
  for (index = 0; index < KBD_ALL_BUTTONS; index++) {
    key_id = loc_default_mapping[index];
    if ((key_id >= SIM_C_FPS) && (key_id <= SIM_C_SCREEN)) {
      psp_kbd_mapping[index] = key_id;
    }
    key_id = loc_default_mapping_L[index];
    if ((key_id >= SIM_C_FPS) && (key_id <= SIM_C_SCREEN)) {
      psp_kbd_mapping_L[index] = key_id;
    }
    key_id = loc_default_mapping_R[index];
    if ((key_id >= SIM_C_FPS) && (key_id <= SIM_C_SCREEN)) {
      psp_kbd_mapping_R[index] = key_id;
    }
  }
  return 0;
}

int
psp_kbd_load_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      error = 0;
  
  KbdFile = fopen(kbd_filename, "r");
  error   = 1;

  if (KbdFile != (FILE*)0) {
  psp_kbd_load_mapping_file(KbdFile);
  error = 0;
    fclose(KbdFile);
  }

  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
    
  return error;
}

int
psp_kbd_load_mapping_file(FILE *KbdFile)
{
  char     Buffer[512];
  char    *Scan;
  int      tmp_mapping[KBD_ALL_BUTTONS];
  int      tmp_mapping_L[KBD_ALL_BUTTONS];
  int      tmp_mapping_R[KBD_ALL_BUTTONS];
  int      sim_key_id = 0;
  int      kbd_id = 0;

  memcpy(tmp_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(tmp_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_R));
  memcpy(tmp_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

  while (fgets(Buffer,512,KbdFile) != (char *)0) {
      
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';
      if (Buffer[0] == '#') continue;

      Scan = strchr(Buffer,'=');
      if (! Scan) continue;
    
      *Scan = '\0';
      sim_key_id = atoi(Scan + 1);

      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name[kbd_id])) {
          tmp_mapping[kbd_id] = sim_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_L[kbd_id])) {
          tmp_mapping_L[kbd_id] = sim_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_R[kbd_id])) {
          tmp_mapping_R[kbd_id] = sim_key_id;
          //break;
        }
      }
  }

  memcpy(psp_kbd_mapping, tmp_mapping, sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, tmp_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, tmp_mapping_R, sizeof(psp_kbd_mapping_R));
  
  return 0;
}

int
psp_kbd_save_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      kbd_id = 0;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "w");
  error   = 1;

  if (KbdFile != (FILE*)0) {

    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name[kbd_id], psp_kbd_mapping[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_L[kbd_id], psp_kbd_mapping_L[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_R[kbd_id], psp_kbd_mapping_R[kbd_id]);
    }
    error = 0;
    fclose(KbdFile);
  }

  return error;
}

int
psp_kbd_enter_command()
{
  SceCtrlData  c;

  unsigned int command_key = 0;
  int          sim_key     = 0;
  int          key_event   = 0;

  sceCtrlPeekBufferPositive(&c, 1);

  if (command_sim_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
      loc_last_event_time = c.TimeStamp;
      command_sim_pending = 0;
      sim_key_event(command_sim_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
    loc_last_event_time = c.TimeStamp;

    if (command_index >= command_size) {

      command_mode  = 0;
      command_index = 0;
      command_size  = 0;

      command_sim_pending = 0;
      command_sim_key     = 0;

      return 0;
    }
  
    command_key = command_keys[command_index++];
    sim_key = sim_get_key_from_ascii(command_key);

    if (sim_key != -1) {
      command_sim_key     = sim_key;
      command_sim_pending = 1;
      sim_key_event(command_sim_key, 1);
    }

    return 1;
  }

  return 0;
}

int 
psp_kbd_is_danzeff_mode()
{
  return danzeff_mode;
}

int
psp_kbd_enter_danzeff()
{
  unsigned int danzeff_key = 0;
  int          sim_key     = 0;
  int          key_event   = 0;
  SceCtrlData  c;

  if (! danzeff_mode) {
    psp_init_keyboard();
    danzeff_mode = 1;
  }

  sceCtrlPeekBufferPositive(&c, 1);
  c.Buttons &= PSP_ALL_BUTTON_MASK;

  if (danzeff_sim_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_event_time = c.TimeStamp;
      danzeff_sim_pending = 0;
      sim_key_event(danzeff_sim_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_DANZEFF_TIME) {
    loc_last_event_time = c.TimeStamp;
  
    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
# ifdef USE_PSP_IRKEYB
    psp_irkeyb_set_psp_key(&c);
# endif
    danzeff_key = danzeff_readInput(c);
  }

  if (danzeff_key == DANZEFF_LEFT) {
    danzeff_key = DANZEFF_DEL;
  } else if (danzeff_key == DANZEFF_DOWN) {
    danzeff_key = DANZEFF_ENTER;
  } else if (danzeff_key == DANZEFF_RIGHT) {
  } else if (danzeff_key == DANZEFF_UP) {
  }

  if (danzeff_key > DANZEFF_START) {
    sim_key = sim_get_key_from_ascii(danzeff_key);

    if (sim_key != -1) {
      danzeff_sim_key     = sim_key;
      danzeff_sim_pending = 1;
      sim_key_event(danzeff_sim_key, 1);
    }

    return 1;

  } else if (danzeff_key == DANZEFF_START) {
    danzeff_mode        = 0;
    danzeff_sim_pending = 0;
    danzeff_sim_key     = 0;

    psp_kbd_wait_no_button();

  } else if (danzeff_key == DANZEFF_SELECT) {
    danzeff_mode        = 0;
    danzeff_sim_pending = 0;
    danzeff_sim_key     = 0;
    psp_main_menu();
    psp_init_keyboard();

    psp_kbd_wait_no_button();
  }

  return 0;
}

#ifdef USE_PSP_IRKEYB
int
psp_kbd_enter_irkeyb()
{
  int sim_key   = 0;
  int psp_irkeyb = PSP_IRKEYB_EMPTY;

  SceCtrlData  c;
  sceCtrlPeekBufferPositive(&c, 1);

  if (irkeyb_sim_pending) 
  {
    if ((c.TimeStamp - loc_last_irkbd_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_irkbd_event_time = c.TimeStamp;
      irkeyb_sim_pending = 0;
      sim_key_event(irkeyb_sim_key, 0);
    }
    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_IR_TIME) {
    loc_last_irkbd_event_time = c.TimeStamp;
    psp_irkeyb = psp_irkeyb_read_key();
  }

  if (psp_irkeyb != PSP_IRKEYB_EMPTY) {

    if (psp_irkeyb == 0x8) {
      sim_key = SIM_DELETE;
    } else
    if (psp_irkeyb == 0x9) {
      sim_key = SIM_TAB;
    } else
    if (psp_irkeyb == 0xd) {
      sim_key = SIM_RETURN;
    } else
    if (psp_irkeyb == 0x1b) {
      sim_key = SIM_ESC;
    } else
    if (psp_irkeyb == PSP_IRKEYB_UP) {
      sim_key = SIM_UP;
    } else
    if (psp_irkeyb == PSP_IRKEYB_DOWN) {
      sim_key = SIM_DOWN;
    } else
    if (psp_irkeyb == PSP_IRKEYB_LEFT) {
      sim_key = SIM_LEFT;
    } else
    if (psp_irkeyb == PSP_IRKEYB_RIGHT) {
      sim_key = SIM_RIGHT;
    } else {
      sim_key = sim_get_key_from_ascii(psp_irkeyb);
    }
    if (sim_key != -1) {
      irkeyb_sim_key     = sim_key;
      irkeyb_sim_pending = 1;
      sim_key_event(sim_key, 1);
    }
    return 1;
  }
  return 0;
}
# endif

void
psp_kbd_display_active_mapping()
{
  if (kbd_ltrigger_mapping_active) {
    psp_sdl_fill_rectangle(0, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(0, 0, 10, 3, 0x0, 0);
  }

  if (kbd_rtrigger_mapping_active) {
    psp_sdl_fill_rectangle(470, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(470, 0, 10, 3, 0x0, 0);
  }
}

int
sim_decode_key(int psp_b, int button_pressed)
{
  int wake = 0;
  int reverse_analog = sim_get_psp_reverse_analog();

  if (reverse_analog) {
    if ((psp_b >= KBD_JOY_UP  ) &&
        (psp_b <= KBD_JOY_LEFT)) {
      psp_b = psp_b - KBD_JOY_UP + KBD_UP;
    } else
    if ((psp_b >= KBD_UP  ) &&
        (psp_b <= KBD_LEFT)) {
      psp_b = psp_b - KBD_UP + KBD_JOY_UP;
    }
  }

  if (psp_b == KBD_START) {
     if (button_pressed) psp_kbd_enter_danzeff();
  } else
  if (psp_b == KBD_SELECT) {
    if (button_pressed) {
      psp_main_menu();
      psp_init_keyboard();
    }
  } else {
 
    if (psp_kbd_mapping[psp_b] >= 0) {
      wake = 1;
      if (button_pressed) {
        // Determine which buton to press first (ie which mapping is currently active)
        if (kbd_ltrigger_mapping_active) {
          // Use ltrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_L[psp_b];
          sim_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else
        if (kbd_rtrigger_mapping_active) {
          // Use rtrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_R[psp_b];
          sim_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else {
          // Use standard mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping[psp_b];
          sim_key_event(psp_kbd_presses[psp_b], button_pressed);
        }
      } else {
          // Determine which button to release (ie what was pressed before)
          sim_key_event(psp_kbd_presses[psp_b], button_pressed);
      }

    } else {
      if (psp_kbd_mapping[psp_b] == KBD_LTRIGGER_MAPPING) {
        kbd_ltrigger_mapping_active = button_pressed;
        kbd_rtrigger_mapping_active = 0;
      } else
      if (psp_kbd_mapping[psp_b] == KBD_RTRIGGER_MAPPING) {
        kbd_rtrigger_mapping_active = button_pressed;
        kbd_ltrigger_mapping_active = 0;
      }
    }
  }
  return 0;
}

# define ANALOG_THRESHOLD 60

void 
kbd_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

static int 
kbd_reset_button_status(void)
{
  int b = 0;
  /* Reset Button status */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    loc_button_press[b]   = 0;
    loc_button_release[b] = 0;
  }
  psp_init_keyboard();
  return 0;
}

int
kbd_scan_keyboard(void)
{
  SceCtrlData c;
  long        delta_stamp;
  int         event;
  int         b;

  int new_Lx;
  int new_Ly;
  int old_Lx;
  int old_Ly;

  event = 0;
  sceCtrlPeekBufferPositive( &c, 1 );
  c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
  psp_irkeyb_set_psp_key(&c);
# endif

  if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
      (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
    /* Exit ! */
    psp_sdl_exit(0);
  }

  delta_stamp = c.TimeStamp - first_time_stamp;
  if ((delta_stamp < 0) || (delta_stamp > KBD_MIN_BATTCHECK_TIME)) {
    first_time_stamp = c.TimeStamp;
    if (psp_is_low_battery()) {
      psp_main_menu();
      psp_init_keyboard();
      return 0;
    }
  }

  /* Check Analog Device */
  kbd_get_analog_direction(loc_button_data.Lx,loc_button_data.Ly,&old_Lx,&old_Ly);
  kbd_get_analog_direction( c.Lx, c.Ly, &new_Lx, &new_Ly);

  /* Analog device has moved */
  if (new_Lx > 0) {
    if (old_Lx  > 0) sim_decode_key(KBD_JOY_LEFT, 0);
    sim_decode_key(KBD_JOY_RIGHT, 1);
  
  } else 
  if (new_Lx < 0) {
    if (old_Lx  < 0) sim_decode_key(KBD_JOY_RIGHT, 0);
    sim_decode_key(KBD_JOY_LEFT , 1);
  
  } else {
    if (old_Lx  > 0) sim_decode_key(KBD_JOY_LEFT , 0);
    else
    if (old_Lx  < 0) sim_decode_key(KBD_JOY_RIGHT, 0);
    else {
      sim_decode_key(KBD_JOY_LEFT  , 0);
      sim_decode_key(KBD_JOY_RIGHT , 0);
    }
  }
  
  if (new_Ly < 0) {
    if (old_Ly  > 0) sim_decode_key(KBD_JOY_DOWN , 0);
    sim_decode_key(KBD_JOY_UP   , 1);
  
  } else 
  if (new_Ly > 0) {
    if (old_Ly  < 0) sim_decode_key(KBD_JOY_UP   , 0);
    sim_decode_key(KBD_JOY_DOWN , 1);
  
  } else {
    if (old_Ly  > 0) sim_decode_key(KBD_JOY_DOWN , 0);
    else
    if (old_Ly  < 0) sim_decode_key(KBD_JOY_UP   , 0);
    else {
      sim_decode_key(KBD_JOY_DOWN , 0);
      sim_decode_key(KBD_JOY_UP   , 0);
    }
  }
  
  for (b = 0; b < KBD_MAX_BUTTONS; b++) 
  {
    if (c.Buttons & loc_button_mask[b]) {
# if 0 //GAME MODE !
      if (!(loc_button_data.Buttons & loc_button_mask[b])) 
# endif
      {
        loc_button_press[b] = 1;
        event = 1;
      }
    } else {
      if (loc_button_data.Buttons & loc_button_mask[b]) {
        loc_button_release[b] = 1;
        loc_button_press[b] = 0;
        event = 1;
      }
    }
  }
  memcpy(&loc_button_data,&c,sizeof(SceCtrlData));

  return event;
}

void
kbd_wait_start(void)
{
  while (1)
  {
    SceCtrlData c;
    sceCtrlReadBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
    if (c.Buttons & PSP_CTRL_START) break;
  }
}

void
psp_init_keyboard(void)
{
  sim_kbd_reset();
  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
}

void
psp_kbd_wait_no_button(void)
{
  SceCtrlData c;

  do {
   sceCtrlPeekBufferPositive(&c, 1);
   c.Buttons &= PSP_ALL_BUTTON_MASK;

  } while (c.Buttons != 0);
} 

void
psp_kbd_wait_button(void)
{
  SceCtrlData c;

  do {
   sceCtrlReadBufferPositive(&c, 1);
  } while (c.Buttons == 0);
} 

int
psp_update_keys(void)
{
  SceCtrlData c;
  int         b;

  static char first_time = 1;
  static int release_pending = 0;

  if (first_time) {

    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (first_time_stamp == -1) first_time_stamp = c.TimeStamp;
    if ((c.TimeStamp - first_time_stamp) < KBD_MIN_START_TIME) return 0;
    
    memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
    memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
    memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

    sim_kbd_load();

    first_time      = 0;
    release_pending = 0;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      loc_button_release[b] = 0;
      loc_button_press[b] = 0;
    }
    sceCtrlPeekBufferPositive(&loc_button_data, 1);
    loc_button_data.Buttons &= PSP_ALL_BUTTON_MASK;

    psp_main_menu();
    psp_init_keyboard();

    return 0;
  }

  if (command_mode) {
    return psp_kbd_enter_command();
  }

  if (danzeff_mode) {
    return psp_kbd_enter_danzeff();
  }

# ifdef USE_PSP_IRKEYB
  if (psp_kbd_enter_irkeyb()) {
    return 1;
  }
# endif

  sceCtrlPeekBufferPositive(&c, 1);

  if (release_pending == 1)
  {
    if ((c.TimeStamp - loc_last_event_time) < KBD_MIN_PENDING_TIME) return 0;
    release_pending = -1;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      if (loc_button_release[b]) {
        loc_button_release[b] = 0;
        loc_button_press[b] = 0;
        sim_decode_key(b, 0);
      }
    }
  }

  if (release_pending == -1) {
    if ((c.TimeStamp - loc_last_event_time) < KBD_MIN_PENDING_TIME) return 0;

    release_pending = 0;
  } 

  kbd_scan_keyboard();

  /* check press event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_press[b]) {
      loc_button_press[b] = 0;
      release_pending     = 0;
      sim_decode_key(b, 1);
    }
  }
  /* check release event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_release[b]) {
      release_pending = 1;
      break;
    } 
  }

  return 0;
}
