# ifndef SIM_COUPE_C_H
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
# define SIM_COUPE_C_H

#ifdef __cplusplus
extern "C" {
# endif

# define SIM_MAX_SAVE_STATE    5

# define SIM_RENDER_NORMAL     0
# define SIM_RENDER_FIT_WIDTH  1
# define SIM_RENDER_FIT_HEIGHT 2
# define SIM_RENDER_FIT        3
# define SIM_RENDER_X125       4
# define SIM_RENDER_MAX_WIDTH  5
# define SIM_RENDER_MAX_HEIGHT 6
# define SIM_RENDER_MAX        7
# define SIM_LAST_RENDER       7

  extern int psp_screenshot_mode;

  extern void  sim_emulator_reset();
  extern char *sim_get_save_name();

  extern int   sim_get_snd_enabled();
  extern int   sim_get_render_mode();
  extern int   sim_get_view_fps();
  extern int   sim_get_speed_limiter();
  extern int   sim_get_display_lr();
  extern int   sim_get_frame_skip();
  extern int   sim_get_psp_cpu_clock();

  extern void  sim_set_snd_enabled(int value);
  extern void  sim_set_render_mode(int value);
  extern void  sim_set_view_fps(int value);
  extern void  sim_set_display_lr(int value);
  extern void  sim_set_speed_limiter(int value);
  extern void  sim_set_frame_skip(int value);
  extern void  sim_set_psp_cpu_clock(int value);

  extern int   sim_state_load(char *filename, int zip_format);
  extern int   sim_disk_load(char *filename, int disk_id, int zip_format);

  extern int   sim_save_configuration(void);
  extern void  sim_audio_resume(void);
  extern void  sim_audio_pause(void);

  extern int   sim_is_save_used(int slot_id);
  extern int   sim_snapshot_load_slot(int slot_id);
  extern int   sim_snapshot_save_slot(int slot_id);
  extern int   sim_snapshot_del_slot(int slot_id);

  extern void  sim_set_psp_reverse_analog(int value);
  extern int   sim_get_psp_reverse_analog();

  extern void  sim_set_psp_screenshot_id(int value);
  extern int   sim_get_psp_screenshot_id();

  extern int sim_state_load(char *filename, int zip_mode);
  extern int sim_disk_load(char *filename, int disk_id, int zip_mode);

  extern void sim_key_down(int sam_key);
  extern void sim_key_up(int sam_key);
  extern void sim_release_all_key();

  extern int sim_load_settings(void);

  extern int sim_update_save_name(char *Name);
  extern char* sim_get_home_dir(void);

#ifndef MAX_PATH
#define MAX_PATH   512
#endif

#ifdef __cplusplus
 }
# endif

# endif
