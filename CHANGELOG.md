# Changelog

* [Unreleased](#unreleased)
* [1.0.4](#1-0-4)
* [1.0.3](#1-0-3)
* [1.0.2](#1-0-2)
* [1.0.1](#1-0-1)
* [1.0.0](#1-0-0)


## Unreleased
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security
### Contributors


## 1.0.4

### Added

* FreeBSD support


### Fixed

* Leading space added to the "body" string ([#5][5]).

[5]: https://codeberg.org/dnkl/fyi/issues/5


### Contributors

* antenore
* Baptiste Daroussin


## 1.0.3

### Fixed

* Missing include, causing compilation errors.
* Missing fish completions for `--image-data` and `--image-size`


### Contributors

* q66


## 1.0.2

### Added

* Bash completions
* Support for inline image data via `--image-data` and
  `--image-size`. `--image-data` points to a file containing raw RGBA
  data, and `--image-size` specifies its dimensions (e.g. `128x128`).

  The image data is sent to the notification daemon using the
  `image-data` hint.


### Changed

* `image-path` hint is no longer set automatically. This aligns `fyi`
  with `notify-send`.
* Ignore `--category` if set to the empty string.
* Ignore empty string `--hint` values.


### Contributors

*  Leon Henrik Plickat


## 1.0.1

### Fixed

* Git tag issue on codeberg


## 1.0.0

First release
