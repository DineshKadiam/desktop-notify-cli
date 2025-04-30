# FYI

[![Packaging status](https://repology.org/badge/vertical-allrepos/fyi.svg?columns=4)](https://repology.org/project/fyi/versions)

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
* `fyi` has shell completions (bash and [fish](https://fishshell.com/)).
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
reason=dismissed
```

Set a custom image, in addition to the application icon (note: not all
notification daemons support showing _both_ an icon and an image, and
will select one of them):
```sh
fyi --icon firefox --hint string:image-path /path/to/image.png title message
```

Set a custom image in-band. Same limitations to icon vs. image as
above. The image file **must** be raw RGBA pixels. I.e. you **cannot**
use a plain PNG, or SVG etc. You can use `convert` (from ImageMagick)
to convert e.g. a PNG. Since the file contains no metadata, you also
need to tell `fyi` the image's dimensions:

```sh
# Get image dimensions
file /path/to/image.png

# Convert to raw RGBA
convert /path/to/image.png /path/to/raw.rgba

# Notify!
fyi --image-data /path/to/raw.rgba --image-size WxH title message
```


# Sponsoring/donations

* Liberapay: https://liberapay.com/dnkl
* GitHub Sponsors: https://github.com/sponsors/dnkl

[![Donate using Liberapay](https://liberapay.com/assets/widgets/donate.svg)](https://liberapay.com/dnkl/donate)
