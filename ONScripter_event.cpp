/* -*- C++ -*-
 * 
 *  ONScripter_event.cpp - Event handler of ONScripter
 *
 *  Copyright (c) 2001-2016 Ogapee. All rights reserved.
 *            (C) 2014-2016 jh10001 <jh10001@live.cn>
 *
 *  ogapee@aqua.dti2.ne.jp
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ONScripter.h"
#include "Utils.h"
#if defined(LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define ONS_TIMER_EVENT   (SDL_USEREVENT)
#define ONS_MUSIC_EVENT   (SDL_USEREVENT+1)
#define ONS_CDAUDIO_EVENT (SDL_USEREVENT+2)
#define ONS_MIDI_EVENT    (SDL_USEREVENT+3)
#define ONS_CHUNK_EVENT   (SDL_USEREVENT+4)
#define ONS_BREAK_EVENT   (SDL_USEREVENT+5)
#define ONS_BGMFADE_EVENT (SDL_USEREVENT+6)

// This sets up the fade event flag for use in bgm fadeout and fadein.
#define BGM_FADEOUT 0
#define BGM_FADEIN  1

#define EDIT_MODE_PREFIX "[EDIT MODE]  "
#define EDIT_SELECT_STRING "MP3 vol (m)  SE vol (s)  Voice vol (v)  Numeric variable (n)"

static SDL_TimerID timer_id = NULL;
SDL_TimerID timer_cdaudio_id = NULL;
SDL_TimerID timer_bgmfade_id = NULL;

typedef SDL_Keycode ONS_Key;

bool ext_music_play_once_flag = false;

/* **************************************** *
 * Callback functions
 * **************************************** */
extern "C" void musicFinishCallback()
{
    SDL_Event event;
    event.type = ONS_MUSIC_EVENT;
    SDL_PushEvent(&event);
}

extern "C" Uint32 SDLCALL timerCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_id );
    timer_id = NULL;

    SDL_Event event;
    event.type = ONS_TIMER_EVENT;
    SDL_PushEvent( &event );

    return 0;
}

extern "C" Uint32 cdaudioCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_cdaudio_id );
    timer_cdaudio_id = NULL;

    SDL_Event event;
    event.type = ONS_CDAUDIO_EVENT;
    SDL_PushEvent( &event );

    return interval;
}

extern "C" Uint32 SDLCALL bgmfadeCallback( Uint32 interval, void *param )
{
    SDL_Event event;
    event.type = ONS_BGMFADE_EVENT;
    event.user.code = (param == NULL) ? BGM_FADEOUT : BGM_FADEIN;
    SDL_PushEvent( &event );

    return interval;
}

/* **************************************** *
 * OS Dependent Input Translation
 * **************************************** */

ONS_Key transKey(ONS_Key key)
{
#if defined(IPODLINUX)
    switch(key){
      case SDLK_m:      key = SDLK_UP;      break; /* Menu                   */
      case SDLK_d:      key = SDLK_DOWN;    break; /* Play/Pause             */
      case SDLK_f:      key = SDLK_RIGHT;   break; /* Fast forward           */
      case SDLK_w:      key = SDLK_LEFT;    break; /* Rewind                 */
      case SDLK_RETURN: key = SDLK_RETURN;  break; /* Action                 */
      case SDLK_h:      key = SDLK_ESCAPE;  break; /* Hold                   */
      case SDLK_r:      key = SDLK_UNKNOWN; break; /* Wheel clockwise        */
      case SDLK_l:      key = SDLK_UNKNOWN; break; /* Wheel counterclockwise */
      default: break;
    }
#endif
#if defined(WINCE)
    switch(key){
      case SDLK_UP:     key = SDLK_UP;      break; /* vkUP    */
      case SDLK_DOWN:   key = SDLK_DOWN;    break; /* vkDOWN  */
      case SDLK_LEFT:   key = SDLK_LCTRL;   break; /* vkLEFT  */
      case SDLK_RIGHT:  key = SDLK_RETURN;  break; /* vkRIGHT */
      case SDLK_KP0:    key = SDLK_q;       break; /* vkStart */
      case SDLK_KP1:    key = SDLK_RETURN;  break; /* vkA     */
      case SDLK_KP2:    key = SDLK_SPACE;   break; /* vkB     */
      case SDLK_KP3:    key = SDLK_ESCAPE;  break; /* vkC     */
      default: break;
    }
#endif
#if defined(MIYOO)
//FIXME: rg28xx joystick will also generate some keys when joystick event happen
    return key;//key = SDLK_UNKNOWN;
#endif
    return key;
}

static int volume = 31; //0 to 31
ONS_Key transJoystickButton(Uint8 button)
{
#if defined(PSP)  
    ONS_Key/*SDLKey*/ button_map[] = { SDLK_ESCAPE, /* TRIANGLE */
                            SDLK_RETURN, /* CIRCLE   */
                            SDLK_SPACE,  /* CROSS    */
                            SDLK_RCTRL,  /* SQUARE   */
                            SDLK_o,      /* LTRIGGER */
                            SDLK_s,      /* RTRIGGER */
                            SDLK_DOWN,   /* DOWN     */
                            SDLK_LEFT,   /* LEFT     */
                            SDLK_UP,     /* UP       */
                            SDLK_RIGHT,  /* RIGHT    */
                            SDLK_0,      /* SELECT   */
                            SDLK_a,      /* START    */
                            SDLK_UNKNOWN,/* HOME     */ /* kernel mode only */
                            SDLK_UNKNOWN,/* HOLD     */};
    return button_map[button];
#elif defined(MIYOO) 
/*
OGA:
button=joystick=keycode=keyname

A=1
B=0
X=2
Y=3

Left=10
Right=11
Up=8
Down=9

I=12
II=13
III=14
IV=15
V=16
VI=17
*/

    ONS_Key/*SDLKey*/ button_map[] = { 
                                                        SDLK_SPACE, /* 0=oga.B*/
                            SDLK_RETURN, /* 1=oga.A   */
                            SDLK_ESCAPE,  /* 2=oga.X    */
                            SDLK_RCTRL,  /* 3=oga.Y   */
                            SDLK_UNKNOWN,      /* 4 */
                            SDLK_UNKNOWN,      /* 5 */
                            SDLK_UNKNOWN,   /* 6     */
                            SDLK_UNKNOWN,   /* 7     */
                            SDLK_UP,     /* 8=oga.Up       */
                            SDLK_DOWN,         /* 9=oga.Down    */
                            SDLK_LEFT,       /* 10=oga.Left   */
                            SDLK_RIGHT,/* 11=oga.Right   */
                            SDLK_a,/* 12=oga.I     */
                            SDLK_0,/* 13=oga.II     */
			    SDLK_UNKNOWN,/* 14=oga.III     */};

if (button == 14) {
	//endCommand();
        SDL_Event event;
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
//        key = SDLK_UNKNOWN;
} 





#if 0
if (button == 14 || button == 0x40000080) {
//volume up
char str[256] = {0};
++volume;
if (volume > 31) volume = 31;
if (volume < 0) volume = 0;
//https://steward-fu.github.io/website/handheld/rg28xx_volume.htm
sprintf(str, "amixer set 'lineout volume' %d", volume);
system(str);
}
if (button == 13 || button == 0x40000081) {
//volume down
char str[256] = {0};
--volume;
if (volume > 31) volume = 31;
if (volume < 0) volume = 0;
//https://steward-fu.github.io/website/handheld/rg28xx_volume.htm
sprintf(str, "amixer set 'lineout volume' %d", volume);
system(str);
}
#endif




    return button_map[button];
#elif defined(__PS3__)    
    ONS_Key/*SDLKey*/ button_map[] = {
        SDLK_0,      /* SELECT   */
        SDLK_UNKNOWN,/* L3       */
        SDLK_UNKNOWN,/* R3       */
        SDLK_a,      /* START    */
        SDLK_UP,     /* UP       */
        SDLK_RIGHT,  /* RIGHT    */
        SDLK_DOWN,   /* DOWN     */
        SDLK_LEFT,   /* LEFT     */
        SDLK_SPACE,  /* L2       */
        SDLK_RETURN, /* R2       */
        SDLK_o,      /* L1       */
        SDLK_s,      /* R1       */
        SDLK_ESCAPE, /* TRIANGLE */
        SDLK_RETURN, /* CIRCLE   */
        SDLK_SPACE,  /* CROSS    */
        SDLK_RCTRL,  /* SQUARE   */
        SDLK_UNKNOWN,/* PS       */
        SDLK_UNKNOWN,
        SDLK_UNKNOWN,
    };
    return button_map[button];
#elif defined(GP2X)
    ONS_Key/*SDLKey*/ button_map[] = {
        SDLK_UP,      /* UP        */
        SDLK_UNKNOWN, /* UPLEFT    */
        SDLK_LEFT,    /* LEFT      */
        SDLK_UNKNOWN, /* DOWNLEFT  */
        SDLK_DOWN,    /* DOWN      */
        SDLK_UNKNOWN, /* DOWNRIGHT */
        SDLK_RIGHT,   /* RIGHT     */
        SDLK_UNKNOWN, /* UPRIGHT   */
        SDLK_a,       /* START     */
        SDLK_0,       /* SELECT    */
        SDLK_o,       /* L         */
        SDLK_s,       /* R         */
        SDLK_RCTRL,   /* A         */
        SDLK_RETURN,  /* B         */
        SDLK_SPACE,   /* X         */
        SDLK_ESCAPE,  /* Y         */
        SDLK_UNKNOWN, /* VOLUP     */
        SDLK_UNKNOWN, /* VOLDOWN   */
        SDLK_UNKNOWN, /* STICK     */
    };
    return button_map[button];
#endif
    return SDLK_UNKNOWN;
}

SDL_KeyboardEvent transJoystickAxis(SDL_JoyAxisEvent &jaxis)
{
    static int old_axis=-1;

    SDL_KeyboardEvent event;

    ONS_Key axis_map[] = {SDLK_LEFT,  /* AL-LEFT  */
                         SDLK_RIGHT, /* AL-RIGHT */
                         SDLK_UP,    /* AL-UP    */
                         SDLK_DOWN   /* AL-DOWN  */};

    int axis = -1;
    /* rerofumi: Jan.15.2007 */
    /* ps3's pad has 0x1b axis (with analog button) */
    if (jaxis.axis < 2){
        axis = ((3200 > jaxis.value) && (jaxis.value > -3200) ? -1 :
                (jaxis.axis * 2 + (jaxis.value>0 ? 1 : 0) ));
    }

    if (axis != old_axis){
        if (axis == -1){
            event.type = SDL_KEYUP;
            event.keysym.sym = axis_map[old_axis];
        }
        else{
            event.type = SDL_KEYDOWN;
            event.keysym.sym = axis_map[axis];
        }
        old_axis = axis;
    }
    else{
        event.keysym.sym = SDLK_UNKNOWN;
    }
    
    return event;
}

void ONScripter::flushEventSub( SDL_Event &event )
{
    if ( event.type == ONS_MUSIC_EVENT ){
        if ( music_play_loop_flag ||
             (cd_play_loop_flag && !cdaudio_flag ) ){
            stopBGM( true );
            if (music_file_name){
                playSound(music_file_name, SOUND_MUSIC, true);
                Mix_SetMusicPosition( music_loopback_offset );
            }
            else
                playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }
    else if ((event.type == ONS_BGMFADE_EVENT) &&
             (event.user.code == BGM_FADEOUT)){
        Uint32 cur_fade_duration = mp3fadeout_duration_internal;
        if (skip_mode & (SKIP_NORMAL | SKIP_TO_EOP) ||
            ctrl_pressed_status) {
            cur_fade_duration = 0;
            Mix_VolumeMusic( 0 );
        }
        Uint32 tmp = SDL_GetTicks() - mp3fade_start;
        if ( tmp < cur_fade_duration ) {
            tmp = cur_fade_duration - tmp;
            tmp *= music_volume;
            tmp /= cur_fade_duration;

            Mix_VolumeMusic( tmp * MIX_MAX_VOLUME / 100 );
        } else {
            char *ext = NULL;
            if (fadeout_music_file_name) ext = strrchr(fadeout_music_file_name, '.');
            if (ext && (strcmp(ext+1, "OGG") && strcmp(ext+1, "ogg"))){
                // set break event to return to script processing when playing music other than ogg
                SDL_Event event;
                event.type = ONS_BREAK_EVENT;
                SDL_PushEvent( &event );
            }

            stopBGM( false );
        }
    }
    else if ((event.type == ONS_BGMFADE_EVENT) &&
             (event.user.code == BGM_FADEIN)){
        Uint32 cur_fade_duration = mp3fadein_duration_internal;
        if (skip_mode & (SKIP_NORMAL | SKIP_TO_EOP) ||
            ctrl_pressed_status) {
            cur_fade_duration = 0;
            Mix_VolumeMusic( music_volume * MIX_MAX_VOLUME / 100 );
        }
        Uint32 tmp = SDL_GetTicks() - mp3fade_start;
        if ( tmp < cur_fade_duration ) {
            tmp *= music_volume;
            tmp /= cur_fade_duration;

            Mix_VolumeMusic( tmp * MIX_MAX_VOLUME / 100 );
        } else {
            if (timer_bgmfade_id) SDL_RemoveTimer( timer_bgmfade_id );
            timer_bgmfade_id = NULL;
            mp3fadeout_duration_internal = 0;

            char *ext = NULL;
            if (music_file_name) ext = strrchr(music_file_name, '.');
            if (ext && (strcmp(ext+1, "OGG") && strcmp(ext+1, "ogg"))){
                // set break event to return to script processing when playing music other than ogg
                SDL_Event event;
                event.type = ONS_BREAK_EVENT;
                SDL_PushEvent( &event );
            }
        }
    }
    else if ( event.type == ONS_CDAUDIO_EVENT ){
        if ( cd_play_loop_flag ){
            stopBGM( true );
            playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }
    else if ( event.type == ONS_MIDI_EVENT ){
        ext_music_play_once_flag = !midi_play_loop_flag;
        Mix_FreeMusic( midi_info );
        playMIDI(midi_play_loop_flag);
    }
    else if ( event.type == ONS_CHUNK_EVENT ){ // for processing btntim2 and automode correctly
        if ( wave_sample[event.user.code] ){
            Mix_FreeChunk( wave_sample[event.user.code] );
            wave_sample[event.user.code] = NULL;
            if (event.user.code == MIX_LOOPBGM_CHANNEL0 && 
                loop_bgm_name[1] &&
                wave_sample[MIX_LOOPBGM_CHANNEL1])
                Mix_PlayChannel(MIX_LOOPBGM_CHANNEL1, 
                                wave_sample[MIX_LOOPBGM_CHANNEL1], -1);
        }
    }
}

void ONScripter::flushEvent()
{
    SDL_Event event;
    while( SDL_PollEvent( &event ) )
        flushEventSub( event );
}

void ONScripter::removeEvent(int type)
{
    SDL_Event event;

    while(SDL_PeepEvents( &event, 1, SDL_GETEVENT, type, type) > 0);
}

void ONScripter::removeBGMFadeEvent()
{
    removeEvent(ONS_BGMFADE_EVENT);
}

void ONScripter::waitEventSub(int count)
{
    next_time = count;
    timerEvent(true);

    runEventLoop();
    removeEvent( ONS_BREAK_EVENT );
}

bool ONScripter::waitEvent( int count )
{
    if (count > 0) count += SDL_GetTicks();
    
    while(1){
        waitEventSub( count );
        if ( system_menu_mode == SYSTEM_NULL ) break;
        int ret = executeSystemCall();
        if      (ret == 1) return true;
        else if (ret == 2) return false;
    }

    return false;
}

void midiCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif
    if ( !ext_music_play_once_flag ){
        SDL_Event event;
        event.type = ONS_MIDI_EVENT;
        SDL_PushEvent(&event);
    }
}

extern "C" void waveCallback( int channel )
{
    SDL_Event event;
    event.type = ONS_CHUNK_EVENT;
    event.user.code = channel;
    SDL_PushEvent(&event);
}

bool ONScripter::trapHandler()
{
    if (event_mode & WAIT_BUTTON_MODE || 
        event_mode & WAIT_TEXT_MODE) return false;

    if (trap_mode & TRAP_STOP){
        trap_mode |= TRAP_CLICKED;
        return false;
    }
    
    trap_mode = TRAP_NONE;
    stopAnimation( clickstr_state );
    setCurrentLabel( trap_dist );
    current_button_state.button = 0; // to escape from screen effect

    return true;
}

/* **************************************** *
 * Event handlers
 * **************************************** */
bool ONScripter::mouseMoveEvent( SDL_MouseMotionEvent *event )
{
    current_button_state.x = event->x * screen_scale_ratio1;
    current_button_state.y = event->y * screen_scale_ratio2;

    if ( event_mode & WAIT_BUTTON_MODE ){
        mouseOverCheck( current_button_state.x, current_button_state.y );
        if (getmouseover_flag &&
            current_over_button >= getmouseover_lower &&
            current_over_button <= getmouseover_upper){
            current_button_state.button = current_over_button;
            return true;
        }
    }

    return false;
}

bool ONScripter::mousePressEvent( SDL_MouseButtonEvent *event )
{
    if ( variable_edit_mode ) return false;
    
    if ( automode_flag ){
        automode_flag = false;
        return false;
    }

    if ( (event->button == SDL_BUTTON_RIGHT && trap_mode & TRAP_RIGHT_CLICK) ||
         (event->button == SDL_BUTTON_LEFT  && trap_mode & TRAP_LEFT_CLICK) ){
        if (trapHandler()) return true;
    }

    current_button_state.x = event->x * screen_scale_ratio1;
    current_button_state.y = event->y * screen_scale_ratio2;
    current_button_state.down_flag = false;
    skip_mode &= ~SKIP_NORMAL;

    if ( event->button == SDL_BUTTON_RIGHT &&
         event->type == SDL_MOUSEBUTTONUP &&
         ((rmode_flag && event_mode & WAIT_TEXT_MODE) ||
          (event_mode & (WAIT_BUTTON_MODE | WAIT_RCLICK_MODE))) ){
        current_button_state.button = -1;
        sprintf(current_button_state.str, "RCLICK");
        if (event_mode & WAIT_TEXT_MODE){
            if (root_rmenu_link.next)
                system_menu_mode = SYSTEM_MENU;
            else
                system_menu_mode = SYSTEM_WINDOWERASE;
        }
    }
    else if ( event->button == SDL_BUTTON_LEFT &&
              ( event->type == SDL_MOUSEBUTTONUP || btndown_flag ) ){
        current_button_state.button = current_over_button;
        if (current_over_button == -1){
            if (!bexec_flag) current_button_state.button = 0;
            sprintf(current_button_state.str, "LCLICK");
        }
        else{
            sprintf(current_button_state.str, "S%d", current_over_button);
            if (bexec_flag && current_button_link){
                ButtonLink *cbl = current_button_link;
                if ( current_button_link->exbtn_ctl[2] ){
                    SDL_Rect check_src_rect = cbl->image_rect;
                    SDL_Rect check_dst_rect = {0, 0, 0, 0};
                    decodeExbtnControl( cbl->exbtn_ctl[2], &check_src_rect, &check_dst_rect );
                }
                else{
                    sprite_info[ cbl->sprite_no ].visible = true;
                    sprite_info[ cbl->sprite_no ].setCell(2);
                    dirty_rect.add( cbl->image_rect );
                }
                flush( refreshMode() );
            }
        }
            
        if ( event->type == SDL_MOUSEBUTTONDOWN )
            current_button_state.down_flag = true;
    }
    else if ( getmclick_flag && event->button == SDL_BUTTON_MIDDLE ){
        current_button_state.button = -70;
        sprintf(current_button_state.str, "MCLICK");
    }
    else return false;

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ){
        if (!(event_mode & (WAIT_TEXT_MODE)))
            skip_mode |= SKIP_TO_EOL;
        playClickVoice();
        stopAnimation( clickstr_state );

        return true;
    }

    return false;
}

bool ONScripter::mouseWheelEvent(SDL_MouseWheelEvent *event)
{
    if (variable_edit_mode) return false;

    if (automode_flag) {
        automode_flag = false;
        return false;
    }

    current_button_state.x = event->x * screen_scale_ratio1;
    current_button_state.y = event->y * screen_scale_ratio2;
    current_button_state.down_flag = false;
    skip_mode &= ~SKIP_NORMAL;

    if (event->y > 0 &&
        (bexec_flag ||
        (event_mode & WAIT_TEXT_MODE) ||
        (usewheel_flag && event_mode & WAIT_BUTTON_MODE) ||
        system_menu_mode == SYSTEM_LOOKBACK)) {
        current_button_state.button = -2;
        sprintf(current_button_state.str, "WHEELUP");
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    } else if (event->y < 0 &&
        (bexec_flag ||
        (enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE) ||
        (usewheel_flag && event_mode & WAIT_BUTTON_MODE) ||
        system_menu_mode == SYSTEM_LOOKBACK)) {
        if (event_mode & WAIT_TEXT_MODE)
            current_button_state.button = 0;
        else
            current_button_state.button = -3;
        sprintf(current_button_state.str, "WHEELDOWN");
    }
    else return false;

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)) {
        if (!(event_mode & (WAIT_TEXT_MODE)))
            skip_mode |= SKIP_TO_EOL;
        playClickVoice();
        stopAnimation(clickstr_state);

        return true;
    }

    return false;
}

void ONScripter::variableEditMode( SDL_KeyboardEvent *event )
{
    int  i;
    const char *var_name;
    char var_index[12];

    switch ( event->keysym.sym ) {
      case SDLK_m:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_MP3_VOLUME_MODE;
        variable_edit_num = music_volume;
        break;

      case SDLK_s:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_SE_VOLUME_MODE;
        variable_edit_num = se_volume;
        break;

      case SDLK_v:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VOICE_VOLUME_MODE;
        variable_edit_num = voice_volume;
        break;

      case SDLK_n:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VARIABLE_INDEX_MODE;
        variable_edit_num = 0;
        break;

        enum {
            ONS_KP_0 = SDLK_KP_0,
            ONS_KP_1 = SDLK_KP_1,
            ONS_KP_2 = SDLK_KP_2,
            ONS_KP_3 = SDLK_KP_3,
            ONS_KP_4 = SDLK_KP_4,
            ONS_KP_5 = SDLK_KP_5,
            ONS_KP_6 = SDLK_KP_6,
            ONS_KP_7 = SDLK_KP_7,
            ONS_KP_8 = SDLK_KP_8,
            ONS_KP_9 = SDLK_KP_9
        };

      case SDLK_9: case ONS_KP_9: variable_edit_num = variable_edit_num * 10 + 9; break;
      case SDLK_8: case ONS_KP_8: variable_edit_num = variable_edit_num * 10 + 8; break;
      case SDLK_7: case ONS_KP_7: variable_edit_num = variable_edit_num * 10 + 7; break;
      case SDLK_6: case ONS_KP_6: variable_edit_num = variable_edit_num * 10 + 6; break;
      case SDLK_5: case ONS_KP_5: variable_edit_num = variable_edit_num * 10 + 5; break;
      case SDLK_4: case ONS_KP_4: variable_edit_num = variable_edit_num * 10 + 4; break;
      case SDLK_3: case ONS_KP_3: variable_edit_num = variable_edit_num * 10 + 3; break;
      case SDLK_2: case ONS_KP_2: variable_edit_num = variable_edit_num * 10 + 2; break;
      case SDLK_1: case ONS_KP_1: variable_edit_num = variable_edit_num * 10 + 1; break;
      case SDLK_0: case ONS_KP_0: variable_edit_num = variable_edit_num * 10 + 0; break;

      case SDLK_MINUS: case SDLK_KP_MINUS:
        if ( variable_edit_mode == EDIT_VARIABLE_NUM_MODE && variable_edit_num == 0 ) variable_edit_sign = -1;
        break;

      case SDLK_BACKSPACE:
        if ( variable_edit_num ) variable_edit_num /= 10;
        else if ( variable_edit_sign == -1 ) variable_edit_sign = 1;
        break;

      case SDLK_RETURN: case SDLK_KP_ENTER:
        switch( variable_edit_mode ){

          case EDIT_VARIABLE_INDEX_MODE:
            variable_edit_index = variable_edit_num;
            variable_edit_num = script_h.getVariableData(variable_edit_index).num;
            if ( variable_edit_num < 0 ){
                variable_edit_num = -variable_edit_num;
                variable_edit_sign = -1;
            }
            else{
                variable_edit_sign = 1;
            }
            break;

          case EDIT_VARIABLE_NUM_MODE:
            script_h.setNumVariable( variable_edit_index, variable_edit_sign * variable_edit_num );
            break;

          case EDIT_MP3_VOLUME_MODE:
            music_volume = variable_edit_num;
            Mix_VolumeMusic( music_volume * MIX_MAX_VOLUME / 100 );
            break;

          case EDIT_SE_VOLUME_MODE:
            se_volume = variable_edit_num;
            for ( i=1 ; i<ONS_MIX_CHANNELS ; i++ )
                if ( wave_sample[i] ) Mix_Volume( i, se_volume * MIX_MAX_VOLUME / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL0] ) Mix_Volume( MIX_LOOPBGM_CHANNEL0, se_volume * MIX_MAX_VOLUME / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL1] ) Mix_Volume( MIX_LOOPBGM_CHANNEL1, se_volume * MIX_MAX_VOLUME / 100 );
            break;

          case EDIT_VOICE_VOLUME_MODE:
            voice_volume = variable_edit_num;
            if ( wave_sample[0] ) Mix_Volume( 0, se_volume * MIX_MAX_VOLUME / 100 );

          default:
            break;
        }
        if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE )
            variable_edit_mode = EDIT_VARIABLE_NUM_MODE;
        else
            variable_edit_mode = EDIT_SELECT_MODE;
        break;

      case SDLK_ESCAPE:
        if ( variable_edit_mode == EDIT_SELECT_MODE ){
            variable_edit_mode = NOT_EDIT_MODE;
            setCaption( DEFAULT_WM_TITLE, DEFAULT_WM_ICON );
            SDL_Delay( 100 );
            setCaption( wm_title_string, wm_icon_string );
            return;
        }
        variable_edit_mode = EDIT_SELECT_MODE;

      default:
        break;
    }

    if ( variable_edit_mode == EDIT_SELECT_MODE ){
        sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
    }
    else if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE ) {
        sprintf( wm_edit_string, "%s%s%d", EDIT_MODE_PREFIX, "Variable Index?  %", variable_edit_sign * variable_edit_num );
    }
    else if ( variable_edit_mode >= EDIT_VARIABLE_NUM_MODE ){
        int p=0;
        
        switch( variable_edit_mode ){

          case EDIT_VARIABLE_NUM_MODE:
            sprintf( var_index, "%%%d", variable_edit_index );
            var_name = var_index; p = script_h.getVariableData(variable_edit_index).num; break;

          case EDIT_MP3_VOLUME_MODE:
            var_name = "MP3 Volume"; p = music_volume; break;

          case EDIT_VOICE_VOLUME_MODE:
            var_name = "Voice Volume"; p = voice_volume; break;

          case EDIT_SE_VOLUME_MODE:
            var_name = "Sound effect Volume"; p = se_volume; break;

          default:
            var_name = "";
        }
        sprintf( wm_edit_string, "%sCurrent %s=%d  New value? %s%d",
                 EDIT_MODE_PREFIX, var_name, p, (variable_edit_sign==1)?"":"-", variable_edit_num );
    }

    setCaption( wm_edit_string, wm_icon_string );
}

void ONScripter::shiftCursorOnButton( int diff )
{
    int num;
    ButtonLink *button = root_button_link.next;
    for (num=0 ; button ; num++) 
        button = button->next;

    shortcut_mouse_line += diff;
    if      (shortcut_mouse_line < 0)    shortcut_mouse_line = num-1;
    else if (shortcut_mouse_line >= num) shortcut_mouse_line = 0;

    button = root_button_link.next;
    for (int i=0 ; i<shortcut_mouse_line ; i++) 
        button  = button->next;
    
    if (button){
        int x = button->select_rect.x + button->select_rect.w/2;
        int y = button->select_rect.y + button->select_rect.h/2;
        if      (x < 0)             x = 0;
        else if (x >= screen_width) x = screen_width-1;
        if      (y < 0)              y = 0;
        else if (y >= screen_height) y = screen_height-1;
        x = x * screen_device_width / screen_width;
        y = y * screen_device_width / screen_width;
        shift_over_button = button->no;
        warpMouse(x, y);
    }
}

bool ONScripter::keyDownEvent( SDL_KeyboardEvent *event )
{
    if (event->keysym.sym == SDLK_ESCAPE){
        current_button_state.event_type = SDL_MOUSEBUTTONDOWN;
        current_button_state.event_button = SDL_BUTTON_RIGHT;
    }
    else if (event->keysym.sym == SDLK_KP_ENTER){
        current_button_state.event_type = SDL_MOUSEBUTTONDOWN;
        current_button_state.event_button = SDL_BUTTON_LEFT;
    }
    else if (event->keysym.sym == SDLK_LEFT){
        current_button_state.event_type = SDL_MOUSEBUTTONDOWN;
        current_button_state.event_button = SDL_MOUSEWHEEL;
        current_button_state.y = 1;
    }
    else if (event->keysym.sym == SDLK_RIGHT){
        current_button_state.event_type = SDL_MOUSEBUTTONDOWN;
        current_button_state.event_button = SDL_MOUSEWHEEL;
        current_button_state.y = -1;
    }

    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  |= 0x01;
        sprintf(current_button_state.str, "CTRL");
        goto ctrl_pressed;
      case SDLK_LCTRL:
        ctrl_pressed_status  |= 0x02;
        sprintf(current_button_state.str, "CTRL");
        goto ctrl_pressed;
      case SDLK_RSHIFT:
        shift_pressed_status |= 0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status |= 0x02;
        break;
      default:
        break;
    }

    return false;

  ctrl_pressed:
    current_button_state.button  = 0;
    playClickVoice();
    stopAnimation( clickstr_state );

    return true;
}

void ONScripter::keyUpEvent( SDL_KeyboardEvent *event )
{
    if (event->keysym.sym == SDLK_ESCAPE){
        current_button_state.event_type = SDL_MOUSEBUTTONUP;
        current_button_state.event_button = SDL_BUTTON_RIGHT;
    }
    else if (event->keysym.sym == SDLK_KP_ENTER){
        current_button_state.event_type = SDL_MOUSEBUTTONUP;
        current_button_state.event_button = SDL_BUTTON_LEFT;
    }
    else if (event->keysym.sym == SDLK_LEFT){
        current_button_state.event_type = SDL_MOUSEBUTTONUP;
        current_button_state.event_button = SDL_MOUSEWHEEL;
        current_button_state.y = 1;
    }
    else if (event->keysym.sym == SDLK_RIGHT){
        current_button_state.event_type = SDL_MOUSEBUTTONUP;
        current_button_state.event_button = SDL_MOUSEWHEEL;
        current_button_state.y = -1;
    }

    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  &= ~0x01;
        break;
      case SDLK_LCTRL:
        ctrl_pressed_status  &= ~0x02;
        break;
      case SDLK_RSHIFT:
        shift_pressed_status &= ~0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status &= ~0x02;
        break;
      default:
        break;
    }
}

bool ONScripter::keyPressEvent( SDL_KeyboardEvent *event )
{
    current_button_state.button = 0;
    current_button_state.down_flag = false;
    if ( automode_flag ){
        automode_flag = false;
        return false;
    }
    
    if ( event->type == SDL_KEYUP ){
#if !defined(WINRT) && (defined(WIN32) || defined(_WIN32))
      if ((event->keysym.mod & KMOD_ALT) && event->keysym.sym == SDLK_RETURN) {
        setFullScreen(!fullscreen_mode);
        return true;
      }
#endif
        if ( variable_edit_mode ){
            variableEditMode( event );
            return false;
        }

        if ( edit_flag && event->keysym.sym == SDLK_z ){
            variable_edit_mode = EDIT_SELECT_MODE;
            variable_edit_sign = 1;
            variable_edit_num = 0;
            sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
            setCaption( wm_edit_string, wm_icon_string );
        }
    }
    
    if (event->type == SDL_KEYUP)
        skip_mode &= ~SKIP_NORMAL;
    
    if ( shift_pressed_status && event->keysym.sym == SDLK_q && current_mode == NORMAL_MODE ){
        endCommand();
    }

    if ( (trap_mode & TRAP_LEFT_CLICK) && 
         (event->keysym.sym == SDLK_RETURN ||
          event->keysym.sym == SDLK_KP_ENTER ||
          event->keysym.sym == SDLK_SPACE ) ){
        if (trapHandler()) return true;
    }
    else if ( (trap_mode & TRAP_RIGHT_CLICK) && 
              (event->keysym.sym == SDLK_ESCAPE) ){
        if (trapHandler()) return true;
    }
    
    if ( event_mode & WAIT_BUTTON_MODE &&
         (((event->type == SDL_KEYUP || btndown_flag) &&
           ((!getenter_flag && event->keysym.sym == SDLK_RETURN) ||
            (!getenter_flag && event->keysym.sym == SDLK_KP_ENTER))) ||
          ((spclclk_flag || !useescspc_flag) && event->keysym.sym == SDLK_SPACE)) ){
        if ( event->keysym.sym == SDLK_RETURN ||
             event->keysym.sym == SDLK_KP_ENTER ||
             (spclclk_flag && event->keysym.sym == SDLK_SPACE) ){
            current_button_state.button = current_over_button;
            if (current_over_button == -1){
                if (!bexec_flag) current_button_state.button = 0;
                sprintf(current_button_state.str, "RETURN");
            }
            else{
                sprintf(current_button_state.str, "S%d", current_over_button);
                if (bexec_flag && current_button_link){
                    ButtonLink *cbl = current_button_link;
                    if ( current_button_link->exbtn_ctl[2] ){
                        SDL_Rect check_src_rect = cbl->image_rect;
                        SDL_Rect check_dst_rect = {0, 0, 0, 0};
                        decodeExbtnControl( cbl->exbtn_ctl[2], &check_src_rect, &check_dst_rect );
                    }
                    else{
                        sprite_info[ cbl->sprite_no ].visible = true;
                        sprite_info[ cbl->sprite_no ].setCell(2);
                        dirty_rect.add( cbl->image_rect );
                    }
                    flush( refreshMode() );
                }
            }
            if ( event->type == SDL_KEYDOWN )
                current_button_state.down_flag = true;
        }
        else{
            current_button_state.button = -1;
            if (!bexec_flag) current_button_state.button = 0;
            sprintf(current_button_state.str, "SPACE");
        }
        playClickVoice();
        stopAnimation( clickstr_state );

        return true;
    }

    if ( event->type == SDL_KEYDOWN ) return false;
    
    if ( ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ) &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if ( !useescspc_flag && event->keysym.sym == SDLK_ESCAPE){
            current_button_state.button  = -1;
            sprintf(current_button_state.str, "RCLICK");
            if (rmode_flag && event_mode & WAIT_TEXT_MODE){
                if (root_rmenu_link.next)
                    system_menu_mode = SYSTEM_MENU;
                else
                    system_menu_mode = SYSTEM_WINDOWERASE;
            }
        }
        else if ( useescspc_flag && event->keysym.sym == SDLK_ESCAPE ){
            current_button_state.button  = -10;
        }
        else if ( !spclclk_flag && useescspc_flag && event->keysym.sym == SDLK_SPACE ){
            current_button_state.button  = -11;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_LEFT) ||
                  event->keysym.sym == SDLK_h) &&
                 (event_mode & WAIT_TEXT_MODE ||
                  (usewheel_flag && !getcursor_flag && event_mode & WAIT_BUTTON_MODE) || 
                  system_menu_mode == SYSTEM_LOOKBACK)){
            current_button_state.button = -2;
            sprintf(current_button_state.str, "WHEELUP");
            if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_RIGHT) ||
                  event->keysym.sym == SDLK_l) &&
                 ((enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE) ||
                  (usewheel_flag && event_mode & WAIT_BUTTON_MODE) || 
                  system_menu_mode == SYSTEM_LOOKBACK)){
            if (event_mode & WAIT_TEXT_MODE)
                current_button_state.button = 0;
            else
                current_button_state.button = -3;
            sprintf(current_button_state.str, "WHEELDOWN");
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_UP) ||
                  event->keysym.sym == SDLK_k ||
                  event->keysym.sym == SDLK_p) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(1);
            return false;
        }
        else if (((!getcursor_flag && event->keysym.sym == SDLK_DOWN) ||
                  event->keysym.sym == SDLK_j ||
                  event->keysym.sym == SDLK_n) &&
                 event_mode & WAIT_BUTTON_MODE){
            shiftCursorOnButton(-1);
            return false;
        }
        else if ( getpageup_flag && event->keysym.sym == SDLK_PAGEUP ){
            current_button_state.button  = -12;
            sprintf(current_button_state.str, "PAGEUP");
        }
        else if ( getpagedown_flag && event->keysym.sym == SDLK_PAGEDOWN ){
            current_button_state.button  = -13;
            sprintf(current_button_state.str, "PAGEDOWN");
        }
        else if ( (getenter_flag && event->keysym.sym == SDLK_RETURN) ||
                  (getenter_flag && event->keysym.sym == SDLK_KP_ENTER) ){
            current_button_state.button  = -19;
        }
        else if ( gettab_flag && event->keysym.sym == SDLK_TAB ){
            current_button_state.button  = -20;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_UP ){
            current_button_state.button  = -40;
            sprintf(current_button_state.str, "UP");
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_RIGHT ){
            current_button_state.button  = -41;
            sprintf(current_button_state.str, "RIGHT");
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_DOWN ){
            current_button_state.button  = -42;
            sprintf(current_button_state.str, "DOWN");
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_LEFT ){
            current_button_state.button  = -43;
            sprintf(current_button_state.str, "LEFT");
        }
        else if ( getinsert_flag && event->keysym.sym == SDLK_INSERT ){
            current_button_state.button  = -50;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_z ){
            current_button_state.button  = -51;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_x ){
            current_button_state.button  = -52;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_c ){
            current_button_state.button  = -53;
        }
        else if ( getfunction_flag && 
                  event->keysym.sym >= SDLK_F1 && event->keysym.sym <= SDLK_F12 ){
            current_button_state.button = -21-(event->keysym.sym - SDLK_F1);
            sprintf(current_button_state.str, "F%d", event->keysym.sym - SDLK_F1+1);
        }
        else if ( bexec_flag && 
                  event->keysym.sym >= SDLK_0 && event->keysym.sym <= SDLK_9 ){
            current_button_state.button = -1; // dummy
            sprintf(current_button_state.str, "%d", event->keysym.sym - SDLK_0);
        }
        else if ( bexec_flag && 
                  event->keysym.sym >= SDLK_a && event->keysym.sym <= SDLK_z ){
            current_button_state.button = -1; // dummy
            sprintf(current_button_state.str, "%c", 'A' + event->keysym.sym - SDLK_a);
        }
        else if ( bexec_flag && 
                  (event->keysym.sym == SDLK_RSHIFT || event->keysym.sym == SDLK_LSHIFT) ){
            current_button_state.button = -1; // dummy
            sprintf(current_button_state.str, "SHIFT");
        }
        
        if ( current_button_state.button != 0 ){
            stopAnimation( clickstr_state );

            return true;
        }
    }

    if ( event_mode & WAIT_INPUT_MODE &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if (event->keysym.sym == SDLK_RETURN || 
            event->keysym.sym == SDLK_KP_ENTER ||
            event->keysym.sym == SDLK_SPACE ){
            if (!(event_mode & WAIT_TEXT_MODE))
                skip_mode |= SKIP_TO_EOL;
            playClickVoice();
            stopAnimation( clickstr_state );

            return true;
        }
    }
    
    if ( event_mode & WAIT_INPUT_MODE ){
        if (event->keysym.sym == SDLK_s && !automode_flag ){
            skip_mode |= SKIP_NORMAL;
            utils::printInfo("toggle skip to true\n");
            stopAnimation( clickstr_state );

            return true;
        }
        else if (event->keysym.sym == SDLK_o){
            if (skip_mode & SKIP_TO_EOP)
                skip_mode &= ~SKIP_TO_EOP;
            else
                skip_mode |= SKIP_TO_EOP;
            utils::printInfo("toggle draw one page flag to %s\n", (skip_mode & SKIP_TO_EOP?"true":"false") );
            if ( skip_mode & SKIP_TO_EOP ){
                stopAnimation( clickstr_state );

                return true;
            }
        }
        else if ( event->keysym.sym == SDLK_a && !automode_flag ){
            automode_flag = true;
            skip_mode &= ~SKIP_NORMAL;
            utils::printInfo("change to automode\n");
            stopAnimation( clickstr_state );

            return true;
        }
        else if ( event->keysym.sym == SDLK_0 ){
            if (++text_speed_no > 2) text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_1 ){
            text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_2 ){
            text_speed_no = 1;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_3 ){
            text_speed_no = 2;
            sentence_font.wait_time = -1;
        }
    }

    if ( event_mode & ( WAIT_INPUT_MODE | WAIT_BUTTON_MODE ) ){
        if ( event->keysym.sym == SDLK_f ){
            if ( fullscreen_mode ) menu_windowCommand();
            else                   menu_fullCommand();
        }
    }

    return false;
}

void ONScripter::timerEvent(bool init_flag)
{
    int current_time = SDL_GetTicks();
    int remaining_time = next_time;
    if (next_time > 0){
        remaining_time -= current_time;
        if (remaining_time < 0) remaining_time = 0;
    }

    //trick to increase frame rate
    if (remaining_time == 0 && !init_flag){
        SDL_Event event;
        event.type = ONS_BREAK_EVENT;
        SDL_PushEvent(&event);
        return;
    }
    
    int duration = 0;
    if (event_mode & WAIT_TIMER_MODE){
        proceedAnimation(current_time);
        duration = calcDurationToNextAnimation() - current_time;
        if (duration < 0) duration = 0;
    }

    // trick to increase frame rate
    if (remaining_time == 0 && init_flag) {
        SDL_Event event;
        event.type = ONS_BREAK_EVENT;
        SDL_PushEvent(&event);
        return;
    }

    if (duration > 0){
        if (duration > remaining_time && remaining_time > 0)
            duration = remaining_time;

        if (timer_id) SDL_RemoveTimer(timer_id);
        timer_id = SDL_AddTimer(duration, timerCallback, NULL);
    }
    else if (remaining_time > 0){
        if (timer_id) SDL_RemoveTimer(timer_id);
        timer_id = SDL_AddTimer(remaining_time, timerCallback, NULL);
    }
}

#if (defined(IOS) || defined(ANDROID) || defined(WINRT))
//TODO: �������Ҽ�ģ��
SDL_MouseWheelEvent transTouchKey(SDL_TouchFingerEvent &finger) {
    static struct FingerPoint {
        float x, y;
    } finger_start;

    static Sint32 old_key = 0;
    SDL_MouseWheelEvent mw;
    if (finger.type == SDL_FINGERDOWN) {
        finger_start.x = finger.x;
        finger_start.y = finger.y;
    } else if (finger.type == SDL_FINGERMOTION) {
        float dtfinger = finger.y - finger_start.y;
        Sint32 key = 0;
        if (dtfinger > 0.5) key = -1;
        else if (dtfinger < -0.5) key = 1;
        if (old_key != key) {
            mw.y = key;
            old_key = key;
            return mw;
        }
    }
    mw.y = 0;
    return mw;
}

bool ONScripter::convTouchKey(SDL_TouchFingerEvent &finger) {
    SDL_MouseWheelEvent mw = transTouchKey(finger);
    if (mw.y != 0) return mouseWheelEvent(&mw);
    return false;
}
#endif

#if defined(MIYOO)
//code originally from SDL2-2.24.0.tar.gz/test/checkkeys.c

static void
print_string(char **text, size_t *maxlen, const char *fmt, ...)
{
    int len;
    va_list ap;

    va_start(ap, fmt);
    len = SDL_vsnprintf(*text, *maxlen, fmt, ap);
    if (len > 0) {
        *text += len;
        if ( ((size_t) len) < *maxlen ) {
            *maxlen -= (size_t) len;
        } else {
            *maxlen = 0;
        }
    }
    va_end(ap);
}

static void
print_modifiers(char **text, size_t *maxlen)
{
    int mod;
    print_string(text, maxlen, " modifiers:");
    mod = SDL_GetModState();
    if (!mod) {
        print_string(text, maxlen, " (none)");
        return;
    }
    if (mod & KMOD_LSHIFT)
        print_string(text, maxlen, " LSHIFT");
    if (mod & KMOD_RSHIFT)
        print_string(text, maxlen, " RSHIFT");
    if (mod & KMOD_LCTRL)
        print_string(text, maxlen, " LCTRL");
    if (mod & KMOD_RCTRL)
        print_string(text, maxlen, " RCTRL");
    if (mod & KMOD_LALT)
        print_string(text, maxlen, " LALT");
    if (mod & KMOD_RALT)
        print_string(text, maxlen, " RALT");
    if (mod & KMOD_LGUI)
        print_string(text, maxlen, " LGUI");
    if (mod & KMOD_RGUI)
        print_string(text, maxlen, " RGUI");
    if (mod & KMOD_NUM)
        print_string(text, maxlen, " NUM");
    if (mod & KMOD_CAPS)
        print_string(text, maxlen, " CAPS");
    if (mod & KMOD_MODE)
        print_string(text, maxlen, " MODE");
//    if (mod & KMOD_SCROLL)
//        print_string(text, maxlen, " SCROLL");
}

static void
PrintKey(SDL_Keysym * sym, SDL_bool pressed, SDL_bool repeat)
{
    char message[512];
    char *spot;
    size_t left;

    spot = message;
    left = sizeof(message);

    /* Print the keycode, name and state */
    if (sym->sym) {
        print_string(&spot, &left,
                "Key %s:  scancode %d = %s, keycode 0x%08X = %s ",
                pressed ? "pressed " : "released",
                sym->scancode,
                SDL_GetScancodeName(sym->scancode),
                sym->sym, SDL_GetKeyName(sym->sym));
    } else {
        print_string(&spot, &left,
                "Unknown Key (scancode %d = %s) %s ",
                sym->scancode,
                SDL_GetScancodeName(sym->scancode),
                pressed ? "pressed " : "released");
    }
    print_modifiers(&spot, &left);
    if (repeat) {
        print_string(&spot, &left, " (repeat)");
    }
    SDL_Log("%s\n", message);
}
#endif

static int do_capture(const char *device, int grab_flag);
static int evtest( void *userdata ) {
#if 0
        while(1){
            cout<<"hello";<<endl;
            SDL_Delay(1);
        }
#else
        do_capture("/dev/input/event2", 1);
#endif	
}

static int runevtest = 0;
void ONScripter::runEventLoop()
{
    SDL_Event event, tmp_event;

if (runevtest == 0) {
runevtest = 1;
//        SDL_Thread* thread_id = SDL_CreateThread( ::evtest, "evtest", this );
}

    while ( SDL_WaitEvent(&event) ) {
#if defined(USE_SMPEG)
        // required to repeat the movie
        if (layer_smpeg_sample)
            SMPEG_status(layer_smpeg_sample);
#endif    
        bool ret = false;
        // ignore continous SDL_MOUSEMOTION
        while (event.type == SDL_MOUSEMOTION || event.type == SDL_FINGERMOTION) {
            if ( SDL_PeepEvents( &tmp_event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT ) == 0 ) break;
            if (tmp_event.type != SDL_MOUSEMOTION && tmp_event.type != SDL_FINGERMOTION) break;
            SDL_PeepEvents( &tmp_event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );

            event = tmp_event;
        }

        switch (event.type) {
#if defined(IOS) || defined(ANDROID) || defined(WINRT)
        case SDL_FINGERMOTION:
        {
#if defined(MIYOO)
SDL_Log("%s\n", "SDL_FINGERMOTION");
#endif
            if (!btndown_flag && convTouchKey(event.tfinger)) return;
            tmp_event.motion.x = device_width * event.tfinger.x - (device_width - screen_device_width) / 2;
            tmp_event.motion.y = device_height * event.tfinger.y - (device_height - screen_device_height) / 2;
            if (mouseMoveEvent( &tmp_event.motion )) return;
            if (btndown_flag){
                event.button.type = SDL_MOUSEBUTTONDOWN;
                event.button.button = SDL_BUTTON_LEFT;
                if (SDL_GetNumTouchFingers(event.tfinger.touchId) >= 2)
                    event.button.button = SDL_BUTTON_RIGHT;
                event.button.x = tmp_event.motion.x;
                event.button.y = tmp_event.motion.y;
                ret = mousePressEvent( &event.button );
                if (ret) return;
            }
        }
            break;
        case SDL_FINGERDOWN:
        {
#if defined(MIYOO)
SDL_Log("%s\n", "SDL_FINGERDOWN");
#endif
            convTouchKey(event.tfinger);
            tmp_event.motion.x = device_width * event.tfinger.x - (device_width - screen_device_width) / 2;
            tmp_event.motion.y = device_height * event.tfinger.y - (device_height - screen_device_height) / 2;
            if (mouseMoveEvent( &tmp_event.motion )) return;       
            if ( btndown_flag ){
                tmp_event.button.type = SDL_MOUSEBUTTONDOWN;
                tmp_event.button.button = SDL_BUTTON_LEFT;
                if (SDL_GetNumTouchFingers(event.tfinger.touchId) >= 2)
                    tmp_event.button.button = SDL_BUTTON_RIGHT;
                tmp_event.button.x = device_width * event.tfinger.x - (device_width - screen_device_width) / 2;
                tmp_event.button.y = device_height * event.tfinger.y - (device_height - screen_device_height) / 2;
                ret = mousePressEvent( &tmp_event.button );
            }
            {
                num_fingers = SDL_GetNumTouchFingers(event.tfinger.touchId);
                if (num_fingers >= 3){
                    tmp_event.key.keysym.sym = SDLK_LCTRL;
                    ret |= keyDownEvent( &tmp_event.key );
                }
            }
            if (ret) return;
        }
            break;
        case SDL_FINGERUP:
        {
#if defined(MIYOO)
SDL_Log("%s\n", "SDL_FINGERUP");
#endif
            if (num_fingers == 0) break;
            {
                tmp_event.button.type = SDL_MOUSEBUTTONUP;
                tmp_event.button.button = SDL_BUTTON_LEFT;
                if (num_fingers == 2)
                    tmp_event.button.button = SDL_BUTTON_RIGHT;
                tmp_event.button.x = device_width * event.tfinger.x - (device_width - screen_device_width) / 2;
                tmp_event.button.y = device_height * event.tfinger.y - (device_height - screen_device_height) / 2;
                ret = mousePressEvent( &tmp_event.button );
            }
            tmp_event.key.keysym.sym = SDLK_LCTRL;
            keyUpEvent( &tmp_event.key );
            num_fingers = 0;
            if (ret) return;
        }
            break;
#endif
#if !defined(ANDROID) && !defined(IOS) && !defined(WINRT)
          case SDL_MOUSEMOTION:
#if defined(MIYOO) && 0
SDL_Log("%s, %d, %d, %d\n", "SDL_MOUSEMOTION", event.motion.state, event.motion.x, event.motion.y); //not used for miyoo a30
#endif
            if (mouseMoveEvent( &event.motion )) return;
            if (btndown_flag){
                if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))
                    tmp_event.button.button = SDL_BUTTON_LEFT;
                else if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT))
                    tmp_event.button.button = SDL_BUTTON_RIGHT;
                else
                    break;

                tmp_event.button.type = SDL_MOUSEBUTTONDOWN;
                ret = mousePressEvent( &tmp_event.button );
                if (ret) return;
            }
            break;
            
          case SDL_MOUSEBUTTONDOWN:
#if defined(MIYOO) && 0
SDL_Log("%s\n", "SDL_MOUSEBUTTONDOWN"); //not used for miyoo a30
#endif
            current_button_state.event_type = event.type;
            current_button_state.event_button = event.button.button;
            if ( !btndown_flag ) break;
          case SDL_MOUSEBUTTONUP:
#if defined(MIYOO) && 0
SDL_Log("%s\n", "SDL_MOUSEBUTTONUP"); //not used for miyoo a30
#endif
            current_button_state.event_type = event.type;
            current_button_state.event_button = event.button.button;
            ret = mousePressEvent( &event.button );
            if (ret) return;
            break;
          case SDL_MOUSEWHEEL:
#if defined(MIYOO) && 0
SDL_Log("%s\n", "SDL_MOUSEWHEEL"); //not used for miyoo a30
#endif
            ret = mouseWheelEvent(&event.wheel);
            if (ret) return;
            break;
#endif
          case SDL_JOYBUTTONDOWN:
#if defined(MIYOO)
SDL_Log("%s, %d\n", "SDL_JOYBUTTONDOWN", event.jbutton.button);
#endif
            event.key.type = SDL_KEYDOWN;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;
            
          case SDL_KEYDOWN:
#if defined(MIYOO)
		PrintKey(&event.key.keysym, (event.key.state == SDL_PRESSED) ? SDL_TRUE : SDL_FALSE, (event.key.repeat) ? SDL_TRUE : SDL_FALSE);
#endif
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            ret = keyDownEvent( &event.key );
            if ( btndown_flag )
                ret |= keyPressEvent( &event.key );
            if (ret) return;
            break;

          case SDL_JOYBUTTONUP:
#if defined(MIYOO)
SDL_Log("%s\n", "SDL_JOYBUTTONUP");
#endif
            event.key.type = SDL_KEYUP;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;
            
          case SDL_KEYUP:
#if defined(MIYOO)
		PrintKey(&event.key.keysym, (event.key.state == SDL_PRESSED) ? SDL_TRUE : SDL_FALSE, (event.key.repeat) ? SDL_TRUE : SDL_FALSE);
#endif
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyUpEvent( &event.key );
            ret = keyPressEvent( &event.key );
            if (ret) return;
            break;

          case SDL_JOYAXISMOTION:
          {
#if defined(MIYOO)
SDL_Log("%s\n", "SDL_JOYAXISMOTION");
#endif
              SDL_KeyboardEvent ke = transJoystickAxis(event.jaxis);
              if (ke.keysym.sym != SDLK_UNKNOWN){
                  if (ke.type == SDL_KEYDOWN){
                      keyDownEvent( &ke );
                      if (btndown_flag)
                          keyPressEvent( &ke );
                  }
                  else if (ke.type == SDL_KEYUP){
                      keyUpEvent( &ke );
                      keyPressEvent( &ke );
                  }
              }
              break;
          }

		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  
		  

#if defined(MIYOO)
//copy from https://github.com/weimingtom/kirikiroid2-miyoo-a30/blob/master/cocos/platform/desktop/CCGLViewImpl-desktop.cpp
//some init code is in ONScripter.cpp
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP: {

#define JOYSTICK_BUTTON_DPAD_DOWN__ 6
#define JOYSTICK_BUTTON_DPAD_LEFT__ 7
#define JOYSTICK_BUTTON_DPAD_UP__ 8 
#define JOYSTICK_BUTTON_DPAD_RIGHT__ 9
int c2jbutton = -1;
int c2kbutton = -1;


#if 1//defined(USE_DEBUG_INPUT)
SDL_Log("%s, %s, %d\n", 
(event.type == SDL_CONTROLLERBUTTONDOWN) ? 
"SDL_CONTROLLERBUTTONDOWN" : 
"SDL_CONTROLLERBUTTONUP", 
(event.cbutton.state == SDL_PRESSED) ? 
"SDL_PRESSED" : 
"SDL_RELEASED", 
event.cbutton.button);
#endif				
		switch (event.cbutton.state) {
			case SDL_PRESSED:
#if 1						
				//TVPPostInputEvent(new tTVPOnKeyDownInputEvent(TJSNativeInstance, sdl_gamecontrollerbutton_to_vk_key(event.cbutton.button, &isQuit, s, 0, TJSNativeInstance), s));
#endif

//trimui smart pro:
//UP:11
//DOWN:12
//LEFT:13
//RIGHT:14
//A:1(joy 1, only up)
//B:0(joy 0)
//X:3(joy 3)
//Y:2(joy 2)
//L1:9(Joy 4)
//L2:10(Joy 5)
//Start:6(Joy 7)
//Select:4(Joy 6)
//Menu:5(Joy 8)
//joy: joyaxismotion
//key: Power(102, VolumeUp(128), VolumeDown(129)
//===
//plan:
//A=return=kag3.Return
//B=space=kag3.Space
//X=fast=kag3.F(#'F')
//Y=menu=kag3.ESC=message history
//
//Select=????
//Start=auto=kag3.A
//Menu=exit=???kage3.exit
//
//L1=left?=?kag3.B=back?
//R1=right?
//
//?=kage3.S=save?
//?=kage3.L=load?

if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_UP__;
	c2kbutton = SDLK_UP;
printf("emulate SDLK_UP\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_DOWN__;
	c2kbutton = SDLK_DOWN;
printf("emulate SDLK_DOWN\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_LEFT__;
	c2kbutton = SDLK_LEFT;
printf("emulate SDLK_LEFT\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_RIGHT__;
	c2kbutton = SDLK_RIGHT;
printf("emulate SDLK_RIGHT\n");
}
if (c2kbutton >=0) //c2jbutton >= 0)
{
#if 0
	event.key.type = SDL_KEYDOWN;
	event.key.keysym.sym = c2kbutton;//transJoystickButton(c2jbutton);
	if (event.key.keysym.sym != SDLK_UNKNOWN) {
		event.key.keysym.sym = transKey(event.key.keysym.sym);
		ret = keyDownEvent( &event.key );
		if ( btndown_flag )
			ret |= keyPressEvent( &event.key );
		if (ret) return;
	}
#else
//SDL_Event mevent = {0};
//mevent.type = SDL_MOUSEMOTION;
//mevent.motion.x = screen_device_width / 2;
//mevent.motion.y = screen_device_height / 2;
//SDL_PushEvent(&mevent);
//SDL_WarpMouseInWindow(window, screen_device_width / 2, screen_device_height / 2);

SDL_Event ev2 = {0};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)c2kbutton;
ev2.key.keysym.sym = (SDL_Scancode)c2kbutton;
SDL_PushEvent(&ev2);
#endif
}
				break;
				
			case SDL_RELEASED:
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_UP__;
	c2kbutton = SDLK_UP;
printf("emulate SDLK_UP\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_DOWN__;
	c2kbutton = SDLK_DOWN;
printf("emulate SDLK_DOWN\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_LEFT__;
	c2kbutton = SDLK_LEFT;
printf("emulate SDLK_LEFT\n");
}
if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
{
        c2jbutton = JOYSTICK_BUTTON_DPAD_RIGHT__;
	c2kbutton = SDLK_RIGHT;
printf("emulate SDLK_RIGHT\n");
}
if (c2kbutton >=0) //c2jbutton >= 0)
{
#if 0
	event.key.type = SDL_KEYUP;
	event.key.keysym.sym = c2kbutton;//transJoystickButton(c2jbutton);
	if (event.key.keysym.sym != SDLK_UNKNOWN) {
		event.key.keysym.sym = transKey(event.key.keysym.sym);
		keyUpEvent( &event.key );
		ret = keyPressEvent( &event.key );
		if (ret) return;
	}
#else
//SDL_Event mevent = {0};
//mevent.type = SDL_MOUSEMOTION;
//mevent.motion.x = screen_device_width / 2;
//mevent.motion.y = screen_device_height / 2;
//SDL_PushEvent(&mevent);
//SDL_WarpMouseInWindow(window, screen_device_width / 2, screen_device_height / 2);

SDL_Event ev2 = {0};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)c2kbutton;
ev2.key.keysym.sym = (SDL_Scancode)c2kbutton;
SDL_PushEvent(&ev2);
#endif
}
				break;
		}
	}
	break;
#endif //defined(MIYOO)	














































          case ONS_TIMER_EVENT:
            timerEvent(false);
            break;

          case ONS_MUSIC_EVENT:
          case ONS_BGMFADE_EVENT:
          case ONS_CDAUDIO_EVENT:
          case ONS_MIDI_EVENT:
            flushEventSub( event );
            break;

          case ONS_CHUNK_EVENT:
            flushEventSub( event );
            //utils::printInfo("ONS_CHUNK_EVENT %d: %x %d %x\n", event.user.code, wave_sample[0], automode_flag, event_mode);
            if ( event.user.code != 0 ||
                 !(event_mode & WAIT_VOICE_MODE) ) break;

            event_mode &= ~WAIT_VOICE_MODE;

          case ONS_BREAK_EVENT:
            if (event_mode & WAIT_VOICE_MODE && wave_sample[0]){
                next_time = -1;
                timerEvent(false);
                break;
            }

            if (automode_flag || autoclick_time > 0)
                current_button_state.button = 0;
            else if ( usewheel_flag ){
                current_button_state.button = -5;
                sprintf(current_button_state.str, "TIMEOUT");
            }
            else{
                current_button_state.button = -2;
                sprintf(current_button_state.str, "TIMEOUT");
            }

            if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) && 
                ( clickstr_state == CLICK_WAIT || 
                  clickstr_state == CLICK_NEWPAGE ) ){
                playClickVoice(); 
                stopAnimation( clickstr_state ); 
            }

            return;
          case SDL_WINDOWEVENT:  
              switch (event.window.event) {
              case SDL_WINDOWEVENT_EXPOSED: 
#ifdef USE_SDL_RENDERER
			  	  SDL_RenderPresent(renderer); 
#else
                  SDL_UpdateWindowSurface(window);
#endif
				  break;
			  case SDL_WINDOWEVENT_FOCUS_LOST:
                  Mix_Pause(-1);
                  Mix_PauseMusic();
                  // the mouse cursor leaves the window
                  SDL_MouseMotionEvent mevent;
                  mevent.x = screen_device_width;
                  mevent.y = screen_device_height;
                  mouseMoveEvent(&mevent);
                  break;
              case SDL_WINDOWEVENT_FOCUS_GAINED:
                  Mix_ResumeMusic();
                  Mix_Resume(-1);
#ifdef ANDROID
                  if (compatibilityMode) repaintCommand();
                  SDL_SetWindowSize( window, screen_device_width, screen_device_height);
                  repaintCommand();
#endif //ANDROID
                  break;
              }
#ifdef USE_SDL_RENDERER
              //SDL_RenderPresent(renderer);
#else
              //SDL_UpdateWindowSurface(window);
#endif
              break;
          case SDL_QUIT:
            endCommand();
            break;
            
          default:
            break;
        }
    }
}






















static unsigned int joypad = 0;


//https://blog.csdn.net/weixin_68016945/article/details/130411193
//https://blog.csdn.net/u011942101/article/details/124732587
#include <stdio.h>
#include <stdint.h>
#if defined(WIN32) || defined(__MINGW32__)
//#include <winsock.h>
#include <Winsock2.h>
struct input_event {
    //unsigned int type, code;

    struct timeval time;
    unsigned int type;
    unsigned int code;
    signed int value;
};
#define EV_KEY 1
int ioctl(int fd, int cmd, void *argp) {
    return 0;
}
#define EVIOCGRAB 0
int getuid() {
    return 1;
}
#else
#include <linux/input.h>
#endif
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
static volatile sig_atomic_t stop = 0;
static void interrupt_handler(int sig)
{
    stop = 1;
}
static int print_events(int fd)
{
	struct input_event ev[64];
	int i, rd;
	fd_set rdfs;

	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);

	while (!stop) {
		::select(fd + 1, &rdfs, NULL, NULL, NULL);
		if (stop)
			break;
		rd = read(fd, ev, sizeof(ev));

		if (rd < (int) sizeof(struct input_event)) {
			SDL_Log("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
			perror("\nevtest: error reading");
			return 1;
		}

		for (i = 0; i < rd / sizeof(struct input_event); i++) {
			unsigned int type, code;

			type = ev[i].type;
			code = ev[i].code;


			if (type == EV_KEY) {
				SDL_Log("Event: time %ld.%06ld, ", ev[i].time.tv_sec, ev[i].time.tv_usec);
				SDL_Log("type %d, code %d, ", type, code);
				SDL_Log("value %d\n", ev[i].value);

/*
evtest, /dev/input/event1
/dev/input/event0:      sunxi-gpadc0
/dev/input/event1:      gpio-keys
Select the device event number [0-1]: 1
1
Input driver version is 1.0.1
Input device ID: bus 0x19 vendor 0x1 product 0x1 version 0x100
Input device name: "gpio-keys"
Supported events:
Event type 0 (EV_SYN)
Event type 1 (EV_KEY)
UP, PE0, left up��type 1 (EV_KEY), code 114 (KEY_VOLUMEDOWN), value 1->0
DOWN, PE7, left down��type 1 (EV_KEY), code 115 (KEY_VOLUMEUP), value 1
LEFT: PE6, left left��type 1 (EV_KEY), code 116 (KEY_POWER), value 1
RIGHT: PE4, left right��type 1 (EV_KEY), code 117 (KEY_KPEQUAL), value 1
X, PE11, right up��type 1 (EV_KEY), code 120 (KEY_SCALE), value 1
B, PE12, right down��type 1 (EV_KEY), code 119 (KEY_PAUSE), value 1
Y, PE2, right left��type 1 (EV_KEY), code 118 (KEY_KPPLUSMINUS), value 1
A, PE13, right right��type 1 (EV_KEY), code 121 (KEY_KPCOMMA), value 1
SELECT, PE8, left 1��type 1 (EV_KEY), code 122 (KEY_HANGUEL), value 1
START, PE9, left 2��type 1 (EV_KEY), code 123 (KEY_HANJA), value 1
MENU, PE3, right 1��type 1 (EV_KEY), code 124 (KEY_YEN), value 1
L, PE1��left top: Event code 125 (KEY_LEFTMETA)
R, PE10, right top: Event code 126 (KEY_RIGHTMETA)





A:Key pressed :  scancode -157945520 = , keycode 0x00000020 = Space  modifiers: (none) (repeat)
*/

#define MY_Key_A (121)//A
#define MY_Key_B (119)//B
#define MY_Key_Select (122)//Select
#define MY_Key_Start (123)//Start
#define MY_Key_Up (114)//Up
#define MY_Key_Down (115)//Down
#define MY_Key_Aeft (116)//Left
#define MY_Key_Right (117)//Right

				if (ev[i].value == 1) {
					switch(code)
					{
						case MY_Key_A:
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							
							joypad &= ~(1);
							joypad |= 1;
						}break;
						case MY_Key_B: //B : B
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<1);
							joypad |= 1<<1;
						}break;
						case MY_Key_Select: //1 : Select
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<2);
							joypad |= 1<<2;
						}break;
						case MY_Key_Start: //2 : Start
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<3);
							joypad |= 1<<3;
						}break;
						case MY_Key_Up: //Up : up
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<4);
							joypad |= 1<<4;
						}break;
						case MY_Key_Down: //Down : Down
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<5);
							joypad |= 1<<5;
						}break;
						case MY_Key_Aeft: //Left : Left
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<6);
							joypad |= 1<<6;
						}break;
						case MY_Key_Right: //Right : Right
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYDOWN;
ev2.key.state = SDL_PRESSED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<7);
							joypad |= 1<<7;
						}break;
						default:break;
					}

					//qDebug()<<"keyPressEvent:"<<joypad;
				} else if (ev[i].value == 0) {
					switch(code)
					{
						case MY_Key_A: //space : A
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1);
						}break;
						case MY_Key_B: //B : B
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
						joypad &= ~(1<<1);
						}break;
						case MY_Key_Select: //1 : Select
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<2);
						}break;
						case MY_Key_Start: //2 : Start
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<3);
						}break;
						case MY_Key_Up: //Up : up
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<4);
						}break;
						case MY_Key_Down: //Down : Down
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<5);
						}break;
						case MY_Key_Aeft: //Left : Left
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<6);
						}break;
						case MY_Key_Right: //Right : Right
						{
SDL_Event ev2 = {};
ev2.type = SDL_KEYUP;
ev2.key.state = SDL_RELEASED;
ev2.key.keysym.scancode = (SDL_Scancode)SDLK_a;
ev2.key.keysym.sym = SDLK_a;
SDL_PushEvent(&ev2);
							joypad &= ~(1<<7);
						}break;
						default:break;
					}
					//qDebug()<<"keyReleaseEvent:"<<joypad;
				}
			}
		}

	}

	ioctl(fd, EVIOCGRAB, (void*)0);
	return EXIT_SUCCESS;
}

static int test_grab(int fd, int grab_flag)
{
	int rc;

	rc = ioctl(fd, EVIOCGRAB, (void*)1);

	if (rc == 0 && !grab_flag)
		ioctl(fd, EVIOCGRAB, (void*)0);

	return rc;
}

static int do_capture(const char *device, int grab_flag)
{
	int fd;
	char *filename = NULL;

	filename = strdup(device);

	if (!filename)
		return EXIT_FAILURE;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		//QString str = QString("evtest : open %1").arg(filename);
		perror("evtest");
		//perror(str.toAscii().constData());
		if (errno == EACCES && getuid() != 0) {
			SDL_Log("You do not have access to %s. Try "
					"running as root instead.\n",
					filename);
		} else {
			SDL_Log("You do not have access to %s.\n",
					filename);
		}
		fflush(stderr);
		goto error;
	}

	if (!isatty(fileno(stdout)))
		setbuf(stdout, NULL);

SDL_Log("*******test_grab\n");
	if (test_grab(fd, grab_flag))
	{
#if 1
		SDL_Log("***********************************************\n");
		SDL_Log("  This device is grabbed by another process.\n");
		SDL_Log("  No events are available to evtest while the\n"
			   "  other grab is active.\n");
		SDL_Log("  In most cases, this is caused by an X driver,\n"
			   "  try VT-switching and re-run evtest again.\n");
		SDL_Log("  Run the following command to see processes with\n"
			   "  an open fd on this device\n"
			   " \"fuser -v %s\"\n", filename);
		SDL_Log("***********************************************\n");
#endif
	}

	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);

	free(filename);

	return print_events(fd);

error:
	free(filename);
	return EXIT_FAILURE;
}



