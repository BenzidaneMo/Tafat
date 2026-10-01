# Tafat artwork

Vector sources of the Tafat logo and icons, drawn with the colors of the
Tafat theme (`core/src/BrandTheme.h`).

| File | Use |
|---|---|
| `tafat-logo.svg` | Main logo: a T giving light to three student laptops |
| `tafat-logo-dark.svg` | Main logo for dark backgrounds |
| `tafat-master.svg` | Master app icon, 48 px and larger |
| `tafat-master-small.svg` | Master app icon, 16–32 px (window, tray) |
| `tafat-configurator.svg` | Configurator app icon, 48 px and larger |
| `tafat-configurator-small.svg` | Configurator app icon, 16–32 px |

After changing a source file, run `tools/render-artwork.sh` (needs
`rsvg-convert` and ImageMagick). It regenerates the PNG, ICO, XPM and BMP
files used by the build, the Linux desktop integration and the Windows
installer. The generated files keep their upstream Veyon file names, so the
build system does not need to change.
