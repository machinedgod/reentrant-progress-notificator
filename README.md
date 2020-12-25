# Reentrant progress indicator

A simple dialog that displays a progress bar, and can be adjusted in
preset increments using SIGUSR1 and SIGUSR2. It lives for 2 seconds, then dies,
or resets the timer if it receives a signal.

Depends on librsvg (to draw the icon), x11 and Cairo.


Usage:
```
reentrant-progress-notificator <mod-val> <init-val>  -- To run initial instance
reentrant-progress-notificator (+|-) <pid>           -- A simple wrapper around `kill(3)`
                                          				to prevent accidentally sending a
                                          				signal to a wrong process
```


Example:
```
~$ reentrant-progress-notificator 2 50
...
~$ reentrant-progress-notificator + $(pidof reentrant-progress-notificator)
```

Check man page or `--help` for all the options.

How it looks like:

![Screenshot](https://github.com/machinedgod/reentrant-progress-notificator/blob/master/screenshot.png?raw=true)

The muddy looking background is due to the fact that background is set to be
semi-transparent, and the compositor setup I use that blurs the background of
focused windows.


How to plug into `.xmonad.hs` (via `volume-notification` script and pamixer dependency):
```
, ((noModMask, 0x1008ff13), spawn "volume-notification up") -- More volume
, ((noModMask, 0x1008ff11), spawn "volume-notification down") -- Less volume
```
the hotkeys refer to `VolumeUp` and `VolumeDown` standard media keys.
Notice that the increment unit has to be the same (`2` in this case).


Additions to your `manageHook`, necessary to un-tile the window and center
it:
```
, className =? "reentrant-progress-notificator" --> placeHook(fixed (0.5, 0.5)) <+> doFloat
```
