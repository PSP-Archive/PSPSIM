// Part of SimCoupe - A SAM Coupe emulator
//
// Input.cpp: SDL keyboard, mouse and joystick input
//
//  Copyright (c) 1999-2006  Simon Owen
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "SimCoupe.h"

#include "Action.h"
#include "Display.h"
#include "Frame.h"
#include "GUI.h"
#include "Input.h"
#include "IO.h"
#include "Util.h"
#include "Options.h"
#include "Mouse.h"
#include "UI.h"
#include "psp_kbd.h"


typedef struct
{
    int     nChar;                  // Symbol character (if any)

    eSamKey nSamKey, nSamModifiers; // Up to 2 SAM keys needed to generate the above symbol

    SDLKey  nKey;                   // SDL key
    int     nMods;                  // Modifier(s) to use with above key
}
COMBINATION_KEY;

typedef struct
{
    int     nChar;
    SDLKey  nKey;
}
SIMPLE_KEY;

typedef struct
{
    SDLKey  nKey;
    eSamKey nSamKey, nSamModifiers;
}
MAPPED_KEY;


SDL_Joystick *pJoystick1, *pJoystick2;

SDLKey nComboKey;
SDLMod nComboModifiers;
DWORD dwComboTime;

bool fMouseActive;
int nMouseX, nMouseY;

bool afKeyStates[SDLK_LAST], afKeys[SDLK_LAST];
inline bool IsPressed(int nKey_)    { return afKeyStates[nKey_]; }
inline void PressKey(int nKey_)     { afKeyStates[nKey_] = true; }
inline void ReleaseKey(int nKey_)   { afKeyStates[nKey_] = false; }
inline void ToggleKey(int nKey_)    { afKeyStates[nKey_] = !afKeyStates[nKey_]; }
inline void SetMasterKey(int nKey_, bool fHeld_=true) { afKeys[nKey_] = fHeld_; }


SIMPLE_KEY asSamKeys [SK_MAX] =
{
    {0,SDLK_LSHIFT},{'z'},         {'x'},        {'c'},        {'v'},        {0,SDLK_KP1},   {0,SDLK_KP2},{0,SDLK_KP3},
    {'a'},          {'s'},         {'d'},        {'f'},        {'g'},        {0,SDLK_KP4},   {0,SDLK_KP5},{0,SDLK_KP6},
    {'q'},          {'w'},         {'e'},        {'r'},        {'t'},        {0,SDLK_KP7},   {0,SDLK_KP8},{0,SDLK_KP9},
    {'1',SDLK_1},   {'2',SDLK_2},  {'3',SDLK_3}, {'4',SDLK_4}, {'5',SDLK_5}, {0,SDLK_ESCAPE},{0,SDLK_TAB},{0,SDLK_CAPSLOCK},
    {'0',SDLK_0},   {'9',SDLK_9},  {'8',SDLK_8}, {'7',SDLK_7}, {'6',SDLK_6}, {0},            {0},         {0,SDLK_BACKSPACE},
    {'p'},          {'o'},         {'i'},        {'u'},        {'y'},        {0},            {0},         {0,SDLK_KP0},
    {0,SDLK_RETURN},{'l'},         {'k'},        {'j'},        {'h'},        {0},            {0},         {0},
    {' '},          {0,SDLK_LCTRL},{'m'},        {'n'},        {'b'},        {0},            {0},         {0,SDLK_INSERT},
    {0,SDLK_RCTRL}, {0,SDLK_UP},   {0,SDLK_DOWN},{0,SDLK_LEFT},{0,SDLK_RIGHT}
};

// Symbols with SAM keyboard details
COMBINATION_KEY asSamSymbols[] =
{
    { '!',  SK_SHIFT, SK_1 },       { '@',  SK_SHIFT, SK_2 },       { '#',  SK_SHIFT, SK_3 },
    { '$',  SK_SHIFT, SK_4 },       { '%',  SK_SHIFT, SK_5 },       { '&',  SK_SHIFT, SK_6 },
    { '\'', SK_SHIFT, SK_7 },       { '(',  SK_SHIFT, SK_8 },       { ')',  SK_SHIFT, SK_9 },
    { '~',  SK_SHIFT, SK_0 },       { '-',  SK_MINUS, SK_NONE },    { '/',  SK_SHIFT, SK_MINUS },
    { '+',  SK_PLUS, SK_NONE },     { '*',  SK_SHIFT, SK_PLUS },    { '<',  SK_SYMBOL, SK_Q },
    { '>',  SK_SYMBOL, SK_W },      { '[',  SK_SYMBOL, SK_R },      { ']',  SK_SYMBOL, SK_T },
    { '=',  SK_EQUALS, SK_NONE },   { '_',  SK_SHIFT, SK_EQUALS },  { '"',  SK_QUOTES, SK_NONE },
    { '`',  SK_SHIFT, SK_QUOTES },  { '{',  SK_SYMBOL, SK_F },      { '}',  SK_SYMBOL, SK_G },
    { '^',  SK_SYMBOL, SK_H },      { 163/*GBP*/, SK_SYMBOL, SK_L },{ ';',  SK_SEMICOLON, SK_NONE },
    { ':',  SK_COLON, SK_NONE },    { '?',  SK_SYMBOL, SK_X },      { '.',  SK_PERIOD, SK_NONE },
    { ',',  SK_COMMA, SK_NONE },    { '\\', SK_SHIFT, SK_INV },     { '|',  SK_SYMBOL, SK_9 },

    { '\0', SK_NONE,    SK_NONE,    SDLK_UNKNOWN }
};

// Symbols with Spectrum keyboard details
COMBINATION_KEY asSpectrumSymbols[] =
{
    { '!',  SK_SYMBOL, SK_1 },  { '@',  SK_SYMBOL, SK_2 },       { '#',  SK_SYMBOL, SK_3 },
    { '$',  SK_SYMBOL, SK_4 },  { '%',  SK_SYMBOL, SK_5 },       { '&',  SK_SYMBOL, SK_6 },
    { '\'', SK_SYMBOL, SK_7 },  { '(',  SK_SYMBOL, SK_8 },       { ')',  SK_SYMBOL, SK_9 },
    { '_',  SK_SYMBOL, SK_0 },  { '<',  SK_SYMBOL, SK_R },       { '>',  SK_SYMBOL, SK_T },
    { ';',  SK_SYMBOL, SK_O },  { '"',  SK_SYMBOL, SK_P },       { '-',  SK_SYMBOL, SK_J },
    { '^',  SK_SYMBOL, SK_H },  { '+',  SK_SYMBOL, SK_K },       { '=',  SK_SYMBOL, SK_L },
    { ':',  SK_SYMBOL, SK_Z },  { 163/*GBP*/, SK_SYMBOL, SK_X }, { '?',  SK_SYMBOL, SK_C },
    { '/',  SK_SYMBOL, SK_V },  { '*',  SK_SYMBOL, SK_B },       { ',',  SK_SYMBOL, SK_N },
    { '.',  SK_SYMBOL, SK_M },  { '\b', SK_SHIFT,  SK_0 },

    { '\0', SK_NONE,    SK_NONE,    SDLK_UNKNOWN }
};

// Handy mappings from unused PC keys to a SAM combination
MAPPED_KEY asSamMappings[] =
{
    // Some useful combinations
    { SDLK_DELETE,    SK_DELETE, SK_SHIFT },
    { SDLK_HOME,      SK_LEFT,   SK_CONTROL },
    { SDLK_END,       SK_RIGHT,  SK_CONTROL },
    { SDLK_PAGEUP,    SK_F4,     SK_NONE },
    { SDLK_PAGEDOWN,  SK_F1,     SK_NONE },
    { SDLK_NUMLOCK,   SK_EDIT,   SK_SYMBOL },
    { SDLK_MENU,      SK_EDIT,   SK_NONE },
    { SDLK_KP_PERIOD, SK_QUOTES, SK_SHIFT },
    { SDLK_WORLD_0,   SK_EDIT,   SK_NONE },     // Weird +/- key on the Mac maps to Edit
    { SDLK_UNKNOWN }
};

// Handy mappings from unused PC keys to a Spectrum combination
MAPPED_KEY asSpectrumMappings[] =
{
    // Some useful combinations
    { SDLK_DELETE,    SK_0,      SK_SHIFT },
    { SDLK_HOME,      SK_LEFT,   SK_NONE },
    { SDLK_END,       SK_RIGHT,  SK_NONE },
    { SDLK_PAGEUP,    SK_UP,     SK_NONE },
    { SDLK_PAGEDOWN,  SK_DOWN,   SK_NONE },
    { SDLK_CAPSLOCK,  SK_2,      SK_SHIFT },
    { SDLK_UNKNOWN }
};

////////////////////////////////////////////////////////////////////////////////

bool Input::Init (bool fFirstInit_/*=false*/)
{
    Exit(true);

    // Initialise the joysticks if any are
    if ((*GetOption(joydev1) || *GetOption(joydev2)) && !SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    {
        // Loop through the available devices for the ones to use (if any)
        for (int i = 0 ; i < SDL_NumJoysticks() ; i++)
        {
            if (!strcasecmp(SDL_JoystickName(i), GetOption(joydev1)))
                pJoystick1 = SDL_JoystickOpen(i);
            else if (!strcasecmp(SDL_JoystickName(i), GetOption(joydev2)))
                pJoystick2 = SDL_JoystickOpen(i);
        }
    }

    SDL_EnableUNICODE(1);
    fMouseActive = false;
    Mouse::Init(fFirstInit_);
    Purge();

    return true;
}

void Input::Exit (bool fReInit_/*=false*/)
{
    if (!fReInit_)
    {
        if (SDL_WasInit(SDL_INIT_JOYSTICK))
        {
            if (pJoystick1) {SDL_JoystickClose(pJoystick1); pJoystick1 = NULL; }
            if (pJoystick2) {SDL_JoystickClose(pJoystick2); pJoystick2 = NULL; }

            SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        }
    }

    Mouse::Exit(fReInit_);
}


void Input::Acquire (bool fMouse_/*=true*/, bool fKeyboard_/*=true*/)
{
    // Flush out any buffered data if we're changing the acquisition state
    Purge();

    // Emulation mode doesn't use key repeats
    SDL_EnableKeyRepeat(fKeyboard_ ? 0 : 250, fKeyboard_ ? 0 : 30);

    // Set the mouse acquisition state
    if (fMouseActive = fMouse_ && GetOption(mouse))
    {
        // Move the mouse to the centre of the window, to stop it escaping
        nMouseX = Frame::GetWidth() >> 1;
        nMouseY = Frame::GetHeight() >> 1;
        SDL_WarpMouse(nMouseX, nMouseY);
    }
}


// Purge pending keyboard and/or mouse events
void Input::Purge (bool fMouse_/*=true*/, bool fKeyboard_/*=true*/)
{
    SDL_Event event;
    int n;

    if (fKeyboard_)
    {
        // Remove any queued key events and reset all key modifiers and 
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYDOWNMASK|SDL_KEYUPMASK) > 0);
        SDL_SetModState(KMOD_NONE);

        // Release all keys
        memset(afKeyStates, 0, sizeof afKeyStates);
        memset(afKeys, 0, sizeof afKeys);
        ReleaseAllSamKeys();
    }

    if (fMouse_)
    {
        // Remove any queued mouse events and discard any relative mouse motion
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_MOUSEEVENTMASK) > 0);
        SDL_GetRelativeMouseState(&n, &n);

        // Reset the mouse
        Mouse::Init();
    }
}


// Read the current keyboard state, and make any special adjustments needed before it's processed
void ReadKeyboard ()
{
    // Make a copy of the master key table with the real keyboard states
    memcpy(afKeyStates, afKeys, sizeof afKeyStates);

    // Alt-Gr comes through as SDLK_MODE on some platforms and SDLK_RALT on others, so accept both
    // Right-Alt on the Mac somehow appears as SDLK_KP_ENTER, so we'll map that for now too!
    if (IsPressed(SDLK_MODE) || IsPressed(SDLK_KP_ENTER))
        PressKey(SDLK_RALT);

    // If the option is set, Left-ALT does the same as Right-Control: to generate SAM Cntrl
    if (GetOption(altforcntrl) && IsPressed(SDLK_LALT))
        PressKey(SDLK_RCTRL);

    // Alt-Gr can optionally be used for SAM Edit
    if (GetOption(altgrforedit) && IsPressed(SDLK_RALT))
    {
        // Alt-Gr is usually seen with left-control down (NT/W2K), so release it
        ReleaseKey(SDLK_LCTRL);

        // Release Alt-Gr (needed for Win9x it seems) and press the context menu key (also used for SAM Edit)
        ReleaseKey(SDLK_RALT);
        PressKey(SDLK_MENU);
    }

    // A couple of Windows niceties
    if (IsPressed(SDLK_LALT))
    {
        // Alt-Tab for switching apps should not be seen
        if (IsPressed(SDLK_TAB))
            ReleaseKey(SDLK_TAB);
    }
}


// Update a combination key table with a symbol
bool UpdateKeyTable (SIMPLE_KEY* asKeys_, SDL_keysym* pKey_)
{
    // Convert upper-case symbols to lower-case without shift
    if (pKey_->unicode >= 'A' && pKey_->unicode <= 'Z')
    {
        pKey_->unicode += 'a'-'A';
        pKey_->mod = static_cast<SDLMod>(pKey_->mod & ~KMOD_SHIFT);
    }

    // Convert control characters to the base key, as it will be needed for SAM Symbol combinations
    else if ((pKey_->mod & KMOD_CTRL) && pKey_->unicode < ' ')
        pKey_->unicode += 'a'-1;


    for (int i = 0 ; i < SK_MAX ; i++)
    {
        // Is there a mapping entry for the symbol?
        if (asKeys_[i].nChar == pKey_->unicode)
        {
            // Log if the mapping is new
            if (!asKeys_[i].nKey)
                TRACE("%c maps to %d\n", pKey_->unicode, pKey_->sym);

            // Update the key mapping
            asKeys_[i].nKey = pKey_->sym;
            return true;
        }
    }

    return false;
}

// Update a combination key table with a symbol
bool UpdateKeyTable (COMBINATION_KEY* asKeys_, SDL_keysym* pKey_)
{
    for (int i = 0 ; asKeys_[i].nSamKey != SK_NONE ; i++)
    {
        // Is there a mapping entry for the symbol?
        if (asKeys_[i].nChar == pKey_->unicode)
        {
            // Log if the mapping is new
            if (!asKeys_[i].nKey)
                TRACE("%c maps to %d with mods of %#02x\n", pKey_->unicode, pKey_->sym, pKey_->mod);

            // Convert right-shift to left-shift
            if (pKey_->mod & KMOD_RSHIFT)
                pKey_->mod = static_cast<SDLMod>((pKey_->mod & ~KMOD_SHIFT) | KMOD_LSHIFT);

            // Update the key mapping
            asKeys_[i].nKey = pKey_->sym;
            asKeys_[i].nMods = pKey_->mod;
            return true;
        }
    }

    return false;
}


// Process simple key presses
void ProcessKeyTable (SIMPLE_KEY* asKeys_)
{
    // Build the rest of the SAM matrix from the simple non-symbol PC keys
    for (int i = 0 ; i < SK_MAX ; i++)
    {
        if (asKeys_[i].nKey && IsPressed(asKeys_[i].nKey))
            PressSamKey(i);
    }
}

// Process the additional keys mapped from PC to SAM, ignoring shift state
void ProcessKeyTable (MAPPED_KEY* asKeys_)
{
    // Build the rest of the SAM matrix from the simple non-symbol PC keys
    for (int i = 0 ; asKeys_[i].nKey != SDLK_UNKNOWN ; i++)
    {
        if (IsPressed(asKeys_[i].nKey))
        {
            PressSamKey(asKeys_[i].nSamKey);
            PressSamKey(asKeys_[i].nSamModifiers);
        }
    }
}

// Process more complicated key combinations
void ProcessKeyTable (COMBINATION_KEY* asKeys_)
{
    short nShifts = 0;
    if (IsPressed(SDLK_LSHIFT)) nShifts |= KMOD_LSHIFT;
    if (IsPressed(SDLK_LCTRL)) nShifts |= KMOD_LCTRL;
    if (IsPressed(SDLK_LALT)) nShifts |= KMOD_LALT;
    if (IsPressed(SDLK_RALT)) nShifts |= KMOD_RALT;

    // Have the shift states changed while a combo is in progress?
    if (nComboModifiers != KMOD_NONE && nComboModifiers != nShifts)
    {
        // If the combo key is still pressed, start the timer running to re-press it as we're about to release it
        if (IsPressed(nComboKey))
        {
            TRACE("Starting combo timer\n");
            dwComboTime = OSD::GetTime();
        }

        // We're done with the shift state now, so clear it to prevent the time getting reset
        nComboModifiers = KMOD_NONE;
    }

    // Combo unpress timer active?
    if (dwComboTime)
    {
        TRACE("Combo timer active\n");

        // If we're within the threshold, ensure the key remains released
        if ((OSD::GetTime() - dwComboTime) < 250)
        {
            TRACE("Releasing combo key\n");
            ReleaseKey(nComboKey);
        }

        // Otherwise clear the expired timer
        else
        {
            TRACE("Combo timer expired\n");
            dwComboTime = 0;
        }
    }

    for (int i = 0 ; asKeys_[i].nSamKey != SK_NONE; i++)
    {
        if (IsPressed(asKeys_[i].nKey) && asKeys_[i].nMods == nShifts)
        {
            // Release the PC keys used for the key combination
            ReleaseKey(asKeys_[i].nKey);
            if (nShifts & KMOD_LSHIFT) ToggleKey(SDLK_LSHIFT);
            if (nShifts & KMOD_LCTRL) ToggleKey(SDLK_LCTRL);
            if (nShifts & KMOD_LALT) { ToggleKey(SDLK_LALT); ReleaseKey(SDLK_RCTRL); }

            // Press the SAM key(s) required to generate the symbol
            PressSamKey(asKeys_[i].nSamKey);
            PressSamKey(asKeys_[i].nSamModifiers);

            // Remember the key involved with the shifted state for a combo
            nComboKey = asKeys_[i].nKey;
            nComboModifiers = static_cast<SDLMod>(nShifts);
        }
    }
}


// Build the SAM keyboard matrix from the current PC state
void SetSamKeyState ()
{
    // No SAM keys are pressed initially
    ReleaseAllSamKeys();

    // Return to ignore common Windows ALT- combinations so the SAM doesn't see them
    if (!GetOption(altforcntrl) && (IsPressed(SDLK_LALT) && (IsPressed(SDLK_TAB) || IsPressed(SDLK_ESCAPE) || IsPressed(SDLK_SPACE))))
        return;

    // Ignore the Command key on the Mac, to hide shortcut combinations
    if (IsPressed(SDLK_LMETA))
        return;

    // Left and right shift keys are equivalent, and also complementary!
    bool fShiftToggle = IsPressed(SDLK_LSHIFT) && IsPressed(SDLK_RSHIFT);
    if (IsPressed(SDLK_RSHIFT)) PressKey(SDLK_LSHIFT);

    // Process the key combinations required for the mode we're in
    switch (GetOption(keymapping))
    {
        // SAM keys
        case 1:
            ProcessKeyTable(asSamSymbols);
            ProcessKeyTable(asSamMappings);
            break;

        // Spectrum mappings
        case 2:
            ProcessKeyTable(asSpectrumSymbols);
            ProcessKeyTable(asSpectrumMappings);
            break;
    }

    // Toggle shift if both shift keys are down to allow shifted versions of keys that are
    // shifted on the PC but unshifted on the SAM
    if (fShiftToggle)
        ToggleKey(SDLK_LSHIFT);

    // Process the simple key mappings
    ProcessKeyTable(asSamKeys);

    // Caps/Num Lock act as toggle keys and need releasing here if pressed
    if (IsPressed(SDLK_CAPSLOCK)) SetMasterKey(SDLK_CAPSLOCK, false);
    if (IsPressed(SDLK_NUMLOCK)) SetMasterKey(SDLK_NUMLOCK, false);
}


// Process and SDL event message
void Input::ProcessEvent (SDL_Event* pEvent_)
{
    switch (pEvent_->type)
    {
        case SDL_ACTIVEEVENT:
            // Has the mouse escaped the window when active?
            if (fMouseActive && pEvent_->active.state == SDL_APPMOUSEFOCUS && !pEvent_->active.gain)
            {
                // Move the mouse to the centre of the window, to stop it escaping
                nMouseX = Frame::GetWidth() >> 1;
                nMouseY = Frame::GetHeight() >> 1;
                SDL_WarpMouse(nMouseX, nMouseY);
            }

            Purge();
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_keysym* pKey = &pEvent_->key.keysym;

            bool fPress = pEvent_->type == SDL_KEYDOWN;
            bool fCtrl  = !!(pKey->mod & KMOD_CTRL);
            bool fAlt   = !!(pKey->mod & KMOD_ALT);
            bool fShift = !!(pKey->mod & KMOD_SHIFT);

            // Check for function keys
            if (pKey->sym >= SDLK_F1 && pKey->sym <= SDLK_F12)
            {
                Action::Key(pKey->sym-SDLK_F1+1, fPress, fCtrl, fAlt, fShift);
                break;
            }

            // Some additional function keys
            bool fAction = true;
            switch (pKey->sym)
            {
                case SDLK_RETURN:       fAction = fAlt; if (fAction) Action::Do(actToggleFullscreen, fPress);   break;
                case SDLK_KP_MINUS:     if (GetOption(keypadreset)) Action::Do(actResetButton, fPress);         break;
                case SDLK_KP_DIVIDE:    Action::Do(actDebugger, fPress);          break;
                case SDLK_KP_MULTIPLY:  Action::Do(actNmiButton, fPress);         break;
                case SDLK_KP_PLUS:      Action::Do(actTempTurbo, fPress);         break;
                case SDLK_SYSREQ:       Action::Do(actSaveScreenshot, fPress);    break;
                case SDLK_SCROLLOCK:    // Pause key on some platforms comes through as scoll lock?
                case SDLK_PAUSE:        Action::Do(fCtrl ? actResetButton : fShift ? actFrameStep : actPause, fPress);   break;
                default:                fAction = false; break;
            }

            // Have we processed the key?
            if (fAction)
                break;


            // Strip some modifiers flags to treat them as a normal keys
            pKey->mod = static_cast<SDLMod>(pKey->mod & ~(KMOD_NUM|KMOD_CAPS|KMOD_MODE));

            // Fix any missing symbols that QNX doesn't supply correctly yet
            if (pEvent_->type == SDL_KEYDOWN && !pKey->unicode)
            {
                bool fControlOnly = (pKey->mod & KMOD_CTRL) && !(pKey->mod & (KMOD_SHIFT|KMOD_ALT));

                // Control-letter?
                if (fControlOnly && pKey->sym >= SDLK_a && pKey->sym <= SDLK_z)
                    pKey->unicode = pKey->sym - SDLK_a + 1;
                else
                {
                    // Other special key symbol?
                    switch (pKey->sym)
                    {
                        case SDLK_BACKSPACE:
                        case SDLK_TAB:
                        case SDLK_RETURN:
                        case SDLK_ESCAPE:
                            pKey->unicode = pKey->sym;
                            break;

                        default:    // default is needed to keep gcc happy
                            break;
                    }
                }
            }

            // QNX has Return coming through as a newline, so fix it
            if (pKey->unicode == 10)
                pKey->unicode = SDLK_RETURN;

            // Ignore symbols on the keypad
            if (pKey->sym >= SDLK_KP0 && pKey->sym <= SDLK_KP_EQUALS)
                pKey->unicode = 0;

            // Some keys don't seem to come through properly, so try and fix em
            if (pKey->sym == SDLK_UNKNOWN)
            {
                switch (pKey->scancode)
                {
                    case 0x56:  pKey->sym = SDLK_WORLD_95;  break;  // Use something unlikely to clash
                    case 0xc5:  pKey->sym = SDLK_PAUSE;     break;

                    // Fill some missing keypad symbols on Solaris
                    case 0x77:  pKey->sym = SDLK_KP1;       break;
                    case 0x79:  pKey->sym = SDLK_KP3;       break;
                    case 0x63:  pKey->sym = SDLK_KP5;       break;
                    case 0x4b:  pKey->sym = SDLK_KP7;       break;
                    case 0x4d:  pKey->sym = SDLK_KP9;       break;
                }
            }

#if defined(sun) || defined(SOLARIS)
            // Fix some keypad mappings known to be wrong on Solaris
            else if (pKey->sym == 0x111 && pKey->scancode == 0x4c)
                pKey->sym = SDLK_KP8;
            else if (pKey->sym == 0x112 && pKey->scancode == 0x78)
                pKey->sym = SDLK_KP2;
            else if (pKey->sym == 0x113 && pKey->scancode == 0x64)
                pKey->sym = SDLK_KP6;
            else if (pKey->sym == 0x114 && pKey->scancode == 0x62)
                pKey->sym = SDLK_KP4;
#endif
            // OS X needs a few tweaks
            if (pEvent_->type == SDL_KEYDOWN)
            {
                // Correct the unicode values for Shift-Tab and Backspace, or if a Left-Alt modifier is used
                if (pKey->sym == SDLK_TAB || pKey->sym == SDLK_BACKSPACE || pKey->mod & KMOD_LALT)
                    pKey->unicode = pKey->sym;
            }

            TRACE("Key %s: %d (mods=%d u=%d)\n", (pEvent_->key.state == SDL_PRESSED) ? "down" : "up", pKey->sym, pKey->mod, pKey->unicode);
//          Frame::SetStatus("Key %s: %d (mods=%d u=%d)", (pEvent_->key.state == SDL_PRESSED) ? "down" : "up", pKey->sym, pKey->mod, pKey->unicode);

            // Pass any printable characters to the GUI
            if (GUI::IsActive())
            {
                // Convert the cursor keys to GUI symbols
                if (pKey->sym >= SDLK_KP0 && pKey->sym <= SDLK_KP9)
                    pKey->unicode = GK_KP0 + pKey->sym - SDLK_KP0;

                else if (pKey->sym >= SDLK_UP && pKey->sym <= SDLK_LEFT)
                {
                    int anCursors[] = { GK_UP, GK_DOWN, GK_RIGHT, GK_LEFT };
                    pKey->unicode = anCursors[pKey->sym - SDLK_UP];
                }
                else if (pKey->sym >= SDLK_HOME && pKey->sym <= SDLK_PAGEDOWN)
                {
                    int anMovement[] = { GK_HOME, GK_END, GK_PAGEUP, GK_PAGEDOWN };
                    pKey->unicode = anMovement[pKey->sym - SDLK_HOME];
                }

                // Pass any printable key-down messages to the GUI
                if (pEvent_->type == SDL_KEYDOWN && pKey->unicode <= GK_MAX)
                {
                    int nMods = ((pKey->mod & KMOD_SHIFT) ? GKMOD_SHIFT : 0) |
                                ((pKey->mod & KMOD_CTRL)  ? GKMOD_CTRL  : 0);

                    GUI::SendMessage(GM_CHAR, pKey->unicode, nMods);
                }

                break;
            }

            // Process key presses (Caps/Num Lock are toggle keys, so we much treat a change as a press)
            else if (pEvent_->type == SDL_KEYDOWN || pKey->sym == SDLK_CAPSLOCK || pKey->sym == SDLK_NUMLOCK)
            {
                // Set the pressed key in the master key table
                afKeys[pKey->sym] = true;

                // Update any symbols we see in the combination key table
                if (pKey->unicode && !UpdateKeyTable(asSamKeys, pKey))
                {
                    // The table we update depends on the key mapping being used
                    switch (GetOption(keymapping))
                    {
                        case 1: UpdateKeyTable(asSamSymbols, pKey);      break;
                        case 2: UpdateKeyTable(asSpectrumSymbols, pKey); break;
                    }
                }
            }

            // Clear released keys from the master table
            else if (pEvent_->type == SDL_KEYUP)
                afKeys[pKey->sym] = false;

            break;
        }

        case SDL_MOUSEMOTION:
        {
            // Fetch the current mouse position
            int nX = pEvent_->motion.x, nY = pEvent_->motion.y;

//          Frame::SetStatus("Mouse: %d,%d", pEvent_->motion.x, pEvent_->motion.y);

            // Show the cursor in windowed mode unless the mouse is acquired or the GUI is active
            bool fShowCursor = !fMouseActive && !GUI::IsActive() && !GetOption(fullscreen);
            SDL_ShowCursor(fShowCursor ? SDL_ENABLE : SDL_DISABLE);

            // Mouse in use by the GUI?
            if (GUI::IsActive())
            {
                Display::DisplayToSamPoint(&nX, &nY);
                GUI::SendMessage(GM_MOUSEMOVE, nX, nY);
            }

            // Is the mouse captured?
            else if (fMouseActive)
            {
                // Calculate the relative movement in native pixels
                nX = -nMouseX;
                nY = -nMouseY;
                SDL_GetMouseState(&nMouseX, &nMouseY);
                nX += nMouseX;
                nY += nMouseY;

                // Has it moved at all?
                if (nX || nY)
                {
                    // We need to track partial units, as we're higher resolution than SAM
                    static int nXX, nYY;

                    // Add on the new movement
                    nXX += nX, nYY += nY;

                    // How far has the mouse moved in SAM units?
                    nX = nXX, nY = nYY;
                    Display::DisplayToSamSize(&nX, &nY);

                    // Update the SAM mouse position
                    Mouse::Move(nX, -nY);
//                  TRACE("Mouse move: X:%-03d Y:%-03d\n", nX, nY);

                    // How far is the SAM mouse movement in native units?
                    Display::SamToDisplaySize(&nX, &nY);

                    // Subtract the used portion of the movement, and leave the remainder for next time
                    nXX -= nX, nYY -= nY;

                    // Move the mouse back to the centre to stop it escaping
                    nMouseX = Frame::GetWidth() >> 1;
                    nMouseY = Frame::GetHeight() >> 1;
                    SDL_WarpMouse(nMouseX, nMouseY);

                }
            }
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            int nX = pEvent_->button.x, nY = pEvent_->button.y;

            // Button presses go to the GUI if it's active
            if (GUI::IsActive())
            {
                Display::DisplayToSamPoint(&nX, &nY);

                switch (pEvent_->button.button)
                {
                    // Mouse wheel up and down
                    case 4:  GUI::SendMessage(GM_MOUSEWHEEL, -1); break;
                    case 5:  GUI::SendMessage(GM_MOUSEWHEEL,  1); break;

                    // Any other mouse button
                    default: GUI::SendMessage(GM_BUTTONDOWN, nX, nY); break;
                }
            }

            // Grab the mouse on a left-click, if not already active (don't let the emulation see the click either)
            else if (fMouseActive)
            {
                Mouse::SetButton(pEvent_->button.button, true);
                TRACE("Mouse button %d pressed\n", pEvent_->button.button);
            }

            // If the mouse is enabled, a left-click acquires it
            else if (GetOption(mouse) && pEvent_->button.button == 1)
                Acquire();

            break;
        }

        case SDL_MOUSEBUTTONUP:
            // Button presses go to the GUI if it's active
            if (GUI::IsActive())
            {
                int nX = pEvent_->button.x, nY = pEvent_->button.y;
                Display::DisplayToSamPoint(&nX, &nY);
                GUI::SendMessage(GM_BUTTONUP, nX, nY);
            }
            else if (fMouseActive)
            {
                TRACE("Mouse button %d released\n", pEvent_->button.button);
                Mouse::SetButton(pEvent_->button.button, false);
            }
            break;

        case SDL_JOYAXISMOTION:
        {
            SDL_JoyAxisEvent* pJoy = &pEvent_->jaxis;
            int nDeadZone = 0x7fffL * GetOption(deadzone1) / 100;

            SetMasterKey(SDLK_6, !pJoy->axis && pJoy->value <= -nDeadZone);
            SetMasterKey(SDLK_7, !pJoy->axis && pJoy->value >=  nDeadZone);
            SetMasterKey(SDLK_8,  pJoy->axis && pJoy->value >=  nDeadZone);
            SetMasterKey(SDLK_9,  pJoy->axis && pJoy->value <= -nDeadZone);

            break;
        }

        case SDL_JOYHATMOTION:
        {
            int nHat = pEvent_->jhat.value;

            SetMasterKey(SDLK_6, (nHat & SDL_HAT_LEFT) != 0);
            SetMasterKey(SDLK_7, (nHat & SDL_HAT_RIGHT) != 0);
            SetMasterKey(SDLK_8, (nHat & SDL_HAT_DOWN) != 0);
            SetMasterKey(SDLK_9, (nHat & SDL_HAT_UP) != 0);

            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            afKeys[SDLK_0] = pEvent_->type == SDL_JOYBUTTONDOWN;
            break;
    }
}


void Input::Update ()
{
    // Read the keyboard and update the SAM keyboard matrix from the current key states (including joystick movement)
    ReadKeyboard();
    SetSamKeyState();
    psp_update_keys();
}
