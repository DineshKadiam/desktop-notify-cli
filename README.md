# FYI

FYI (for your information) is a command line utility to send desktop
notifications to the user via a notification daemon implementing XDG
desktop notifications.

It is a almost a `notify-send` clone, with the following differences:

* `notify-send` does not implement `--close`.
* `notify-send` does not expose activation tokens (needed for window
  focus/activation) in any meaningful way. It prints it as a debug
  message when `G_MESSAGES_DEBUG=all`; `fyi` prints it when you use
  `--print-token`.
* `fyi` has consistent syntax in its `--action` and `--hint`options.
* `fyi` can print the _reason_ a notification was closed, with
  `--print-reason`.
* `fyi` can query the notification daemon for its name and version
  information.
* `fyi` can query the notification daemon for its capabilities.
* `fyi` has shell completions (though only
  [fish](https://fishshell.com/) for now).
* `fyi` has a single run-time dependency:
  [dbus](https://www.freedesktop.org/wiki/Software/dbus/) (the
  original D-Bus implementation).


## Examples

Display a notification:
```sh
fyi 'This is the title' this is the notification message
```

Display a notification with a custom app-name (typically displays an
icon too):
```sh
fyi --app-name firefox notification is this from firefox...
```

Display a notification with a custom app-name and _another_ icon:
```sh
fyi --app-name firefox -i chromium notification firefox or chromium...
```

Print notification ID, and wait for notification to close:
```sh
fyi --print-id --wait title message
```
prints
```
id=7
```

Close the notification sent above:
```sh
fyi --close 7
```
or, press `ctrl+c`; this will send `SIGINT` to `fyi`, which will close
the notification.

Add two actions (then trigger one of them):
```sh
fyi --action default:'Click to activate' --action no:Nope title message
```
prints
```
action=default (or no)
```

Print ID, close reason and activation token:
```sh
fyi --print-id --print-reason --print-token --action default:'Click to activate' \
    title message
```
prints
```
id=5
xdgtoken=24c1d76038357e75ec04f27e30ef46bb
action=default
reason: dismissed
```
