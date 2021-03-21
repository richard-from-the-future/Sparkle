# Sparkle
A tiny interface for the Spark40 guitar amp
Richard Jackett, 2021

This is a work in progress.  There are many bugs still, and it can be a bit janky, but it is functional.

Enormous thanks and recognition to Paul Hamshere for his SparkClass and for the SparkPedalOverride code, which I used as the basis for this program.

This will only work on the M5Stack Core2 hardware, although it could be modified to work on any ESP32 with a touchscreen with enough work.  It will not work on the M5stack Core, as that doesn't have a touch screen.

Sparkle needs these to work:

M5Core2.h --- Install the M5Core2 library. I think I'm using the older version of this 0.01, but hopefully it works with the new version too.

BluetoothSerial.h --- ESP32 bluetooth from Espressif

ArduinoJson.h --- by Benoit Blanchon

The Arduino IDE will want you to put the Sparkle code in a folder with the same title.

Basic operation:
* The mode is changed by vertical swipe gestures on the left hand side of the screen (e.g. AMP, MOD, etc)
* The effect is changed by horizontal swipe gesture over the current effect text at the bottom (in the code effects are called 'items')
* Parameters are changed by dragging up or down within each slider. The value (%) is shown above. Change happens on release.
* Double tap on a slider is supposed to set it alternately to zero, then to 100 (to work like a switch). Currently flaky.
* Sliders that show -1 are not used by the current effect.
* Turn the current effect on/off by the centre button (BtnB).
* Cycle through hardware presets (0 to 3) using the right button (BtnC).
* The left button (BtnA) toggles whether the 'Extra' (hidden) effects are enabled (like extra amps, treble booster, etc).
* The status light bottom right is yellow when connecting, and green when connected to Spark40

The touch screen on the Core2 is more responsive when plugged into USB and/or held in the hand (i.e. grounded).

Bugs:
* Too numerous to list them all, here are the lowlights...
* Doesn't update screen when parameters are changed on amp, and in some other situations (incl. preset change).
* Sometimes the left/right gesture doesn't change the effect until the direction is reversed, like the index increases beyond bounds or something.
* Under some situations Sparkle can get out of sync with the amp. Need to look into this.
* The double tap rarely works.
* Some effects have more than 5 parameters, or have switches. Switches are mapped to sliders, set to > 50 or < 50 to enable/disable.
