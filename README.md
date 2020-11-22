# Reentrant progress indicator

A simple dialog that displays a progress bar, and can be adjusted in
preset increments using SIGUSR1 and SIGUSR2. It lives for 2 seconds, then dies,
or resets the timer if it receives a signal.

Depends on librsvg (to draw the icon), x11 and Cairo.


Usage:
```
notification-box <mod-val> <init-val>  -- To run initial instance
notification-box (+|-) <pid>           -- A simple wrapper around `kill(3)`
                                          to prevent accidentally sending a
                                          signal to a wrong process
```


Example:
```
~$ notification-box 2 50
...
~$ notification-box + $(pidof notification-box)
```


How it looks like:

![Screenshot](https://github.com/machinedgod/reentrant-progress-notificator/blob/master/screenshot.png?raw=true)

The muddy looking background is due to the fact that background is set to be
semi-transparent, and the compositor setup I use that blurs the background of
focused windows.


How to plug into xmonad (via `volume-notification` script and pamixer dependence):
```
#!/bin/zsh

pidof notification-box

if [[ $? == 1 ]]
then
    notification-box 2 $(pamixer --get-volume)
else
    if [[ $1 == "up" ]]
    then
        notification-box + $(pidof notification-box)
    elif [[ $1 == "down" ]]
    then
        notification-box - $(pidof notification-box)
    fi
fi
```

`xmonad.hs` additions to your `keys` setup:
```
, ((noModMask, 0x1008ff13), spawn "pamixer --increase 2 && volume-notification up") -- More volume
, ((noModMask, 0x1008ff11), spawn "pamixer --decrease 2 && volume-notification down") -- Less volume
```
the hotkeys refer to `VolumeUp` and `VolumeDown` standard media keys.
Notice that the increment unit has to be the same (`2` in this case).


Additions to your `manageHook`, necessary to un-tile the window and center
it:
```
, className =? "notification-box" --> placeHook(fixed (0.5, 0.5)) <+> doFloat
```


TODO:
- nice fadeout as life approaches end
- runtime configurable icon, per value (eg. 0 -> mute, 100 -> roof blown off)
