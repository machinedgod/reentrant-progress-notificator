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

How to plug into xmonad (via helper script and pamixer dependence):
`volume-notification` ->
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

`xmonad.hs` ->
```
, ((noModMask, 0x1008ff13), spawn "pamixer --increase 2 && volume-notification up") -- More volume
, ((noModMask, 0x1008ff11), spawn "pamixer --decrease 2 && volume-notification down") -- Less volume
```

Notice that the increment unit has to be the same (`2` in this case).


TODO:
- nice fadeout as life approaches end
- runtime configurable icon, per value (eg. 0 -> mute, 100 -> roof blown off)
