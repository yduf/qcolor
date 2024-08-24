# Color Quantisation

Licensed under the [GNU General Public License, version 3. (GPLv3)](https://www.gnu.org/licenses/gpl-3.0.html)


## Behavior
- image formats are those supported by [libvips](https://www.libvips.org/)
- output results as one json line per file, for easier script integration - using [nlohmann/json](https://github.com/nlohmann/json)

## Compiling
C++17

Install dependencies:
These are found by `pkg-info`
- [libvips](https://www.libvips.org/)

```bash
meson build .
cd build
meson compile
```

## Usage
```bash
$ qcolor ~/Pictures/*.webp
```
