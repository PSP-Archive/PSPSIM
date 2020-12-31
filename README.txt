
    Welcome to PSPSIM

Original Authors of SimCoupe

  Allan Skillman, Simon Owen, Dave Laundon

Author of the PSP port version 

  Ludovic.Jacomme also known as Zx-81 (zx81.zx81@gmail.com)


1. INTRODUCTION
   ------------

  SimCoupe is one of the best emulator of the SamCoupe home computer
  running on Windows, PocketPC, Unix etc ...  The emulator faithfully 
  imitates the SamCoupe model (see http://www.simcoupe.org/)

  PSPSIM is a port on PSP of the cvs version 0.90 beta 4 of SamCoupe.

  Thanks to Danzel and Jeff Chen for their virtual keyboard,
  and to all PSPSDK developpers.

  Special thanks goes to Mr Nick666 for the graphics icons and
  background images, the settings and keyboard mapping files
  for several famous games.

  This package is under GPL Copyright, read COPYING file for
  more information about it.


2. INSTALLATION
   ------------

  Unzip the zip file, and copy the content of the directory fw3.x or fw1.5
  (depending of the version of your firmware) on the psp/game, psp/game150,
  or psp/game3xx if you use custom firmware 3.xx-OE.

  Put your disk image files on "discs" sub-directory.

  It has been developped on linux for Firmware 3.71-m33 and i hope it works
  also for other firmwares.

  For any comments or questions on this version, please visit 
  http://zx81.zx81.free.fr, http://www.dcemu.co.uk


3. CONTROL
   ------------

3.1 - Virtual keyboard

  In the SAM emulator window

  Normal mapping :

    PSP        SAM 

    Square     Delete   
    Triangle   Escape 
    Cross      Space
    Circle     Return

    Up         Up
    Down       Down
    Left       Left 
    Right      Right

    LTrigger   Toogle with L keyboard mapping
    RTrigger   Toggle with R keyboard mapping

  LTrigger mapping :

    PSP        SAM
  
    Square     FPS 
    Triangle   Render
    Cross      Render
    Circle     Swap Joystick

    Up         DecY
    Down       IncY
    Left       DecX 
    Right      IncX 

  RTrigger mapping :

    PSP        SAM
  
    Square     Space   
    Triangle   Escape
    Cross      F9
    Circle     Escape

    Up         Up
    Down       Down
    Left       Left 
    Right      Right


    Analog-Pad Left  7
    Analog-Pad Rigth 6
    Analog-Pad Up    9
    Analog-Pad Down  8
  
  In the main menu

    L+R+Start  Exit the emulator
    R Trigger  Reset the SAM

    Triangle   Go Up directory
    Cross      Valid
    Circle     Valid
    Square     Go Back to the emulator window

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

    Use Analog stick to choose one of the 9 squares, and
    use Triangle, Square, Cross and Circle to choose one
    of the 4 letters of the highlighted square.

    Use LTrigger and RTrigger to see other 9 squares 
    figures.

3.2 - IR keyboard
    
  You can also use IR keyboard. Edit the pspirkeyb.ini file
  to specify your IR keyboard model, and modify eventually
  layout keyboard files in the keymap directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Start
  Ctrl-W        Start

  Escape        Select
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle
  Ctrl-Z        L-trigger
  Ctrl-C        R-trigger

  In the emulator window you can use the IR keyboard to
  enter letters, special characters and digits.


4. LOADING SAM DISK FILES
   ------------

  If you want to load disk image in the virtual drive A of your emulator,
  you have to put your disk file (with .zip, .gz, .sbt, .dsk, .sad, .mgt
  or .td0 file extension) on your PSP memory stick in the 'disk' directory.

  Then, while inside PSPSIM emulator, just press SELECT to enter in 
  the emulator main menu, and then using the file selector choose one 
  disk file to load in the virtual drive of your emulator.

  Back to the emulator window, the disk should stard automatically (if not, 
  press the F9 sam key to reboot).

5. LOADING KEY MAPPING FILES
   ------------

  For given games, the default keyboard mapping between PSP Keys and SAM keys,
  is not suitable, and the game can't be played on PSPSIM.

  To overcome the issue, you can write your own mapping file. Using notepad for
  example you can edit a file with the .kbd extension and put it in the kbd 
  directory.

  For the exact syntax of those mapping files, have a look on sample files already
  presents in the kbd directory (default.kbd etc ...).

  After writting such keyboard mapping file, you can load them using the main menu
  inside the emulator.

  If the keyboard filename is the same as the disk filename (.zip etc ...)
  then when you load this snapshot file or this disk, the corresponding keyboard 
  file is automatically loaded !

  You can now use the Keyboard menu and edit, load and save your
  keyboard mapping files inside the emulator. The Save option save the .kbd
  file in the kbd directory using the "Game Name" as filename. The game name
  is displayed on the right corner in the emulator menu.

6. COMPILATION
   ------------

  It has been developped under Linux using gcc with PSPSDK. 
  To rebuild the homebrew run the Makefile in the src archive.
