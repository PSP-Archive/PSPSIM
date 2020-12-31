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

# ifndef _KBD_H_
# define _KBD_H_

#ifdef __cplusplus
extern "C" {
# endif

# define PSP_ALL_BUTTON_MASK 0xFFFF

 enum sim_keys_emum {
   SIM_UNDERSCORE, // _
   SIM_1,          // 1
   SIM_2,          // 2
   SIM_3,          // 3
   SIM_4,          // 4
   SIM_5,          // 5
   SIM_6,          // 6
   SIM_7,          // 7
   SIM_8,          // 8
   SIM_9,          // 9
   SIM_0,          // 0
   SIM_SEMICOLON,  // ;
   SIM_MINUS    ,  // -
   SIM_DELETE,     // DELETE
   SIM_POUND,      // POUND
   SIM_EXCLAMATN,  // !
   SIM_DBLQUOTE,   // "
   SIM_HASH,       // #
   SIM_DOLLAR,     // $
   SIM_PERCENT,    // %
   SIM_AMPERSAND,  // &
   SIM_QUOTE,      // '
   SIM_LEFTPAREN,  // (
   SIM_RIGHTPAREN, // )
   SIM_PLUS,       // +
   SIM_EQUAL,      // =
   SIM_TAB,        // TAB  
   SIM_a,          // a
   SIM_b,          // b
   SIM_c,          // c
   SIM_d,          // d
   SIM_e,          // e
   SIM_f,          // f
   SIM_g,          // g
   SIM_h,          // h
   SIM_i,          // i
   SIM_j,          // j
   SIM_k,          // k
   SIM_l,          // l
   SIM_m,          // m
   SIM_n,          // n
   SIM_o,          // o
   SIM_p,          // p
   SIM_q,          // q
   SIM_r,          // r
   SIM_s,          // s
   SIM_t,          // t
   SIM_u,          // u
   SIM_v,          // v
   SIM_w,          // w
   SIM_x,          // x
   SIM_y,          // y
   SIM_z,          // z
   SIM_A,          // A
   SIM_B,          // B
   SIM_C,          // C
   SIM_D,          // D
   SIM_E,          // E
   SIM_F,          // F
   SIM_G,          // G
   SIM_H,          // H
   SIM_I,          // I
   SIM_J,          // J
   SIM_K,          // K
   SIM_L,          // L
   SIM_M,          // M
   SIM_N,          // N
   SIM_O,          // O
   SIM_P,          // P
   SIM_Q,          // Q
   SIM_R,          // R
   SIM_S,          // S
   SIM_T,          // T
   SIM_U,          // U
   SIM_V,          // V
   SIM_W,          // W
   SIM_X,          // X
   SIM_Y,          // Y
   SIM_Z,          // Z
   SIM_RETURN,     // RETURN
   SIM_CONTROL,    // CONTROL
   SIM_SHIFT,      // SHIFT
   SIM_CAPSLOCK,   // CAPSLOCK
   SIM_ESC,        // ESC
   SIM_SPACE,      // SPACE
   SIM_LEFT,       // LEFT
   SIM_UP,         // UP
   SIM_RIGHT,      // RIGHT
   SIM_DOWN,       // DOWN
   SIM_F0,         // F0
   SIM_F1,         // F1
   SIM_F2,         // F2
   SIM_F3,         // F3
   SIM_F4,         // F4
   SIM_F5,         // F5
   SIM_F6,         // F6
   SIM_F7,         // F7
   SIM_F8,         // F8
   SIM_F9,         // F9
   SIM_AT,         // @
   SIM_COLON,      // :
   SIM_COMMA,      // ,
   SIM_PERIOD,     // .
   SIM_SLASH,      // /
   SIM_ASTERISK,   // *
   SIM_LESS,       // <
   SIM_GREATER,    // >
   SIM_QUESTION,   // ?
   SIM_PIPE,       // |
   SIM_RCBRACE,    // }
   SIM_RBRACKET,   // ]
   SIM_LBRACKET,   // [
   SIM_LCBRACE,    // {
   SIM_BACKSLASH,  //  
   SIM_BACKQUOTE,  // `
   SIM_POWER,      // ^
   SIM_C_FPS,
   SIM_C_JOY,
   SIM_C_RENDER,
   SIM_C_INCX  ,
   SIM_C_INCY  ,
   SIM_C_DECX  ,
   SIM_C_DECY  ,
   SIM_C_SCREEN,

   SIM_MAX_KEY 
 };

 struct sim_key_trans {
   int  sim_key;
   int  sam_key;
   int  sam_mod;
   char name[10];
 };
  
 extern struct sim_key_trans psp_sim_key_to_row_col[SIM_MAX_KEY];

# define KBD_UP           0
# define KBD_RIGHT        1
# define KBD_DOWN         2
# define KBD_LEFT         3
# define KBD_TRIANGLE     4
# define KBD_CIRCLE       5
# define KBD_CROSS        6
# define KBD_SQUARE       7
# define KBD_SELECT       8
# define KBD_START        9
# define KBD_HOME        10
# define KBD_HOLD        11
# define KBD_LTRIGGER    12
# define KBD_RTRIGGER    13

# define KBD_MAX_BUTTONS 14

# define KBD_JOY_UP      14
# define KBD_JOY_RIGHT   15
# define KBD_JOY_DOWN    16
# define KBD_JOY_LEFT    17

# define KBD_ALL_BUTTONS 18

# define KBD_UNASSIGNED         -1

# define KBD_LTRIGGER_MAPPING   -2
# define KBD_RTRIGGER_MAPPING   -3
# define KBD_NORMAL_MAPPING     -1

  extern int psp_screenshot_mode;
  extern int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_presses[ KBD_ALL_BUTTONS ];
  extern int kbd_ltrigger_mapping_active;
  extern int kbd_rtrigger_mapping_active;
  extern struct sim_key_trans psp_sim_key_to_sam_key[SIM_MAX_KEY];

  extern int  psp_update_keys(void);
  extern void kbd_wait_start(void);
  extern void psp_init_keyboard(void);
  extern void psp_kbd_wait_no_button(void);
  extern int  psp_kbd_is_danzeff_mode(void);
  extern int psp_kbd_load_mapping(char *kbd_filename);
  extern int psp_kbd_save_mapping(char *kbd_filename);
  extern void psp_kbd_display_active_mapping(void);

#ifdef __cplusplus
}
# endif

# endif
