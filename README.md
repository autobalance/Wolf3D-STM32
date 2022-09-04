# Wolf3D-STM32

A port of Wolfenstein 3D to an
[STM32H743/750 development board](https://stm32-base.org/boards/STM32H743VIT6-STM32H7XX-M.html), with
analog VGA and analog audio, PS2 keyboard support, and SD card support for loading the game files.

## Description

This project used [Chocolate-Wolfenstein-3D](https://github.com/fabiensanglard/Chocolate-Wolfenstein-3D)
as a base, but I ended up stripping out the SDL dependency, converted from C++ to C (not necessary
in hindsight), among a lot of other changes. All these changes were done in the pursuit of
exploring the Wolf3D code base and learning more about interfacing with low-level hardware.\
\
This description isn't yet complete, but a here's list of some of the changes made (no particular order):\
* Stripped out the dependency on the Simple DirectMedia Layer (SDL) to target interfacing directly with hardware
   * Changed the key scancodes in [id_in.h](src/wolf3d/id_in.h) to match the original definitions for direct keyboard access
   * Created custom types for a screen buffer, pixel colours, and colour palettes in [id_vl.h](src/wolf3d/id_vl.h) and [id_vh.h](src/wolf3d/id_vh.h)
   * Changed the low and high level draw functions in the corresponding sources above to interface directly with the LCD controller peripheral on the STM32H743/750
   * Changed the way sounds are played (digitized only for now and only one channel, no SoundBlaster) in
     [id_sd.c](src/wolf3d/id_sd.c) by using an interrupt to playback sound pages
* Re-implemented the page manager in [id_pm.h](src/wolf3d/id_pm.h) based on the original Wolf3D source
* Created scripts to convert some objects that were stored in SRAM to be read-only and storable in flash
  (see [scripts](src/wolf3d/scripts))
* Various changes in the rest of the Wolf3D source as needed

### Requirements

For now, this port is a bit specific to the hardware I used:

* An STM32H743/750 development board that exposes the LCD controller (LTDC) pins for RGB565
  output, GPIOA pins for DAC output and USART pins for PS2 keyboard support
   * I used the [MCUDev DevEBox STM32H7](https://stm32-base.org/boards/STM32H743VIT6-STM32H7XX-M.html)
* A flasher such as ST-Link to flash the executable to the development board
* A VGA monitor
* A PS2 compatible keyboard (some USB keyboards still have PS2-compatibility)
* Lots of wires and 100 Ohm, 200 Ohm, and 47 Ohm resistors for the VGA dac
* An audio amplifier for the audio output from the internal DAC

### Wiring

See [ltdc.h](src/drivers/ltdc.h) for the VGA pinout, until I get a proper wiring diagram here.

### Configuration

Modify [version.h](src/wolf3d/version.h) to match your acquired game file versions. Current is set to full
version (GOODTIME and CARMACIZED), but you can set it to use the shareware version. Then copy your game
files to a FAT formatted SD card (SanDisk seems to have issues, will have to check the SD card driver for
why).

### Building

The ARM EABI GCC compiler is used for compilation and OpenOCD is used to flash the executable. For example,
these packages can be acquired in Arch Linux via:
```
pacman -Syu arm-none-eabi-gcc openocd
```

Having configured [version.h](src/wolf3d/version.h), run:
```
make
```

to build into the directory 'build'.\
\
You can flash to your board however you usually do, or try:
```
make flash
```
after checking the provided OpenOCD [configuration file](openocd/devebox_stm32h743.cfg).\
\
If everything is wired correctly (TODO above), insert you prepared SD card and power the dev board
and Wolf3D on!

## TODO

* Implement complete sound playback including SoundBlaster support via the fmopl driver included
  in the Chocolate-Wolfenstein-3D source
* Figure out how to get fullscreen video output using the LCD peripheral to create the required video
  timings
* Consider removing support for using an SD card to hold the game files and use the development board's
  QuadSPI flash instead (should remove the necessity for a page manager, and significantly improve
  performance)
* Standardize the hardware interface to make it easier to port to other platforms
* LOTS of things I've obviously missed but will become apparent as I work on this project

## Demo

TODO

## License

This project is licensed under The MIT License (MIT) - see the [LICENSE](LICENSE) file for details.
