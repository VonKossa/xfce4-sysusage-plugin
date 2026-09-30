# xfce4-sysusage-plugin

`xfce4-sysusage-plugin` is an XFCE panel plugin that displays system usage with compact dual-bar sections for:

- **Memory + swap usage**
- **Network incoming + outgoing throughput**

Each section renders two percentage bars from **0% to 100%**, with tooltips showing live numeric details.

## Features

- Two live sections in the XFCE panel:
  - **Memusage** section
    - Memory usage percentage (`MemTotal` / `MemAvailable` from `/proc/meminfo`)
    - Swap usage percentage (`SwapTotal` / `SwapFree` from `/proc/meminfo`)
  - **Netusage** section
    - Incoming throughput percentage (based on `/sys/class/net/<device>/statistics/rx_bytes`)
    - Outgoing throughput percentage (based on `/sys/class/net/<device>/statistics/tx_bytes`)
- Hover tooltips with detailed values:
  - Memory/swap percentages and used/total GiB
  - Network device, in/out Mbit/s
- Left-click opens plugin properties directly
- Right-click uses standard XFCE panel plugin menu/actions
- Configurable appearance and layout:
  - Memory/swap/net bar colors
  - Background, text, and separator colors
  - Bar thickness, length, spacing
  - Section separator spacing
  - Vertical/horizontal orientation
  - Text side, font size, and text padding
  - Memory/swap label text and visibility toggles
- Configurable network behavior:
  - Network device name
  - Incoming bar name
  - Outgoing bar name
  - Update interval
  - Max incoming/outgoing Mbit/s scaling values

## Requirements

This project targets:

- **XFCE 4.18**
- **GTK 3**
- **CMake** build system

Build dependencies:

- `cmake` (>= 3.16)
- `pkg-config`
- `gtk+-3.0`
- `libxfce4panel-2.0`
- `libxfce4util-1.0`
- C compiler (e.g., `gcc`)

## Build instructions

From the repository root:

```bash
cmake -S . -B build
cmake --build build --verbose
```

Build outputs:

- `build/libsysusage.so`
- `build/panel-plugin/sysusage.desktop`

## Automated tests

A small GLib-based test suite now covers pure helper logic for:

- network device-name validation
- memory/swap percentage math and clamping
- network throughput delta/rate math including counter-wrap handling

Run tests with:

```bash
cmake -S . -B build
cmake --build build --target sysusage-logic-tests
ctest --test-dir build --output-on-failure
```

## Static analysis and sanitizer builds

The default build remains unchanged, but optional CMake targets are available
when the corresponding tools are installed:

```bash
cmake -S . -B build
cmake --build build --target clang-tidy
cmake --build build --target cppcheck
cmake --build build --target static-analysis
cmake --build build --target lint
```

The `static-analysis` and `lint` targets run all configured analysis tools
that were found when CMake configured the build directory. `clang-tidy` uses
the `SYSUSAGE_CLANG_TIDY_CHECKS` cache variable, which defaults to
`-*,clang-analyzer-core.*,clang-analyzer-deadcode.*,clang-analyzer-security.*,clang-analyzer-unix.*`.

For a more exhaustive `cppcheck` run, use the generated compilation database
and pass `--check-level=exhaustive` directly to `cppcheck`:

```bash
cmake -S . -B build
cppcheck --enable=warning,style,performance,portability --check-level=exhaustive \
  --error-exitcode=1 --inline-suppr --project=build/compile_commands.json
```

To build with address and undefined behavior sanitizers, configure a separate
build directory with sanitizers enabled:

```bash
cmake -S . -B build-sanitize -DSYSUSAGE_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
```

## Install instructions

Install to the default prefix (usually `/usr/local`):

```bash
cmake --install build
```

This installs:

- Plugin module to `${CMAKE_INSTALL_LIBDIR}/xfce4/panel/plugins`
- Panel descriptor to `${CMAKE_INSTALL_DATADIR}/xfce4/panel/plugins`
- Icon to `${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps`

For a custom install prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
cmake --install build
```

## Project layout

- `src/sysusage-plugin.c` — plugin lifecycle, widget construction, signal setup, and cleanup
- `src/sysusage-render.c` — drawing, sizing, visibility, and tooltip routing
- `src/sysusage-metrics.c` — memory and network metric collection
- `src/sysusage-config.c` — persisted XFCE panel rc settings load/save logic
- `src/sysusage-properties.c` — properties dialog and settings application
- `src/sysusage-plugin.h` — shared defaults, state, and function declarations
- `panel-plugin/sysusage.desktop.in` — XFCE panel plugin metadata template
- `xfce4-sysusage-plugin.svg` — plugin icon
- `CMakeLists.txt` — build and install configuration

## Notes

- Library
  - Gentoo: /usr/lib64/xfce4/panel/plugins/libsysusage.so
  - Debian: /usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libsysusage.so

- Panel descriptor
  - Gentoo: /usr/share/xfce4/panel/plugins/sysusage.desktop
  - Debian: /usr/share/xfce4/panel/plugins/sysusage.desktop

- Icon
  - Gentoo: /usr/share/icons/hicolor/scalable/apps/xfce4-sysusage-plugin.svg
  - Debian: /usr/share/icons/hicolor/scalable/apps/xfce4-sysusage-plugin.svg
  - run: gtk-update-icon-cache -f /usr/share/icons/hicolor/

- Home folder location
  - Library:          ~/.local/libexec/xfce4/panel-plugins/
                      ~/.local/lib/xfce4/panel-plugins/
  - Panel descriptor: ~/.local/share/xfce4/panel-plugins/
  - Icon:             /usr/share/icons/hicolor/scalable/apps/xfce4-sysusage-plugin.svg
 
- Default colors
  - "separator line color"    hex: #3B5868
  - "memory bar color"	      hex: #00AD00
  - "swap bar color"	      	hex: #FF2C2C
  - "network incoming color"	hex: #00AD00
  - "network outgoing color"	hex: #FF2C2C
  - "bar background color"		hex: #E1DEDB
  - "text color"	        		hex: #FFFFFF
