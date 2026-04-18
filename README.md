# Application Code

Real-time color-isolation camera system for Raspberry Pi 3B+.

## Directory Structure

```
application-code/
├── include/
│   ├── camera_buffer.h      # Thread-safe latest-frame slot (camera → sequencer)
│   ├── display_buffer.h     # Thread-safe frame slot (sequencer → display)
│   ├── realtime.h           # SCHED_FIFO and core pinning utilities
│   ├── metrics.h            # Timing/deadline metrics collection
│   ├── hsv_config.h         # Runtime HSV color range configuration
│   └── shared.h             # Global state declarations
├── src/
│   ├── main.cpp             # Orchestration, signal handling
│   ├── camera_thread.cpp    # V4L2 capture (Core 1, async)
│   ├── camera_buffer.cpp    # CameraBuffer implementation
│   ├── sequencer.cpp        # S1→S2→S3 pipeline (Core 2, SCHED_FIFO)
│   ├── display_thread.cpp   # SPI LCD output (Core 3, async)
│   ├── display_buffer.cpp   # DisplayBuffer implementation
│   ├── realtime.cpp         # set_fifo_prio(), pin_to_core()
│   ├── metrics.cpp          # Metrics recording and CSV export
│   └── hsv_config.cpp       # HSV preset handling
├── LCD_Stream/              # Waveshare 2" LCD driver (bcm2835 SPI)
│   ├── Config/
│   │   └── DEV_Config.c/h   # GPIO/SPI hardware abstraction
│   ├── LCD_2inch.c/h        # LCD initialization and display
│   └── Makefile
├── Makefile
└── reference-main.cpp       # Original monolithic reference
```

## Architecture

```
Core 0: main         - keyboard input, debug window
Core 1: camera       - V4L2 capture → CameraBuffer (async, I/O bound)
Core 2: sequencer    - S1(read) → S2(transform) → S3(write) at 30Hz (SCHED_FIFO)
Core 3: display      - DisplayBuffer → SPI LCD (async, I/O bound)
```

## Build

```bash
make
sudo ./capture
```

## Hardware

- Raspberry Pi 3B+
- Logitech C270 webcam (640x480 @ 30fps MJPG)
- Waveshare 2" SPI LCD (240x320)

---

AI Use Disclosure: Claude was used to format and organize this file using the following prompt "Please take the existing repository structure and create a well-structured Directory Structure diagram"
