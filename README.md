<div id="top"></div>
<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/some00/doomtime">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>

<h3 align="center">Stream Doom to PineTime</h3>

  <p align="center">
    Stream Doom from Linux to PineTime smartwatch.
    <br />
    <a href="https://github.com/some00/doomtime">View Demo</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About the Project</a></li>
    <li><a href="#getting-started">Getting Started</a></li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#benchmark">Benchmark</a></li>
    <li><a href="#license">License</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About the Project

[![picture][product-screenshot]](https://github.com/some00/doomtime)

The goal of this project is to stream Doom, Strife, Heretic or Hexen from Linux to a PineTime
smartwatch using BLE and the BlueZ stack. OTA isn't implemented and it isn't planned to be. I
believe this use case for the PineTime doesn't have a partical use and I chose the DevKit only as a
target.
My aim was to find a way to achive this and improve my skills along the way.


<p align="right">(<a href="#top">back to top</a>)</p>


<!-- GETTING STARTED -->
## Getting Started

The repository contains the source for the firmware, modifications needed for Chocolate Doom, Bluez
and a client application. The rest of this section assumes Debian bullseye. Arch Linux was also
tested, but the process isn't documented.


### Clone Chocolate Doom Fork

The fork has this repository added as a submodule.
```sh
git clone -b doomtime https://github.com/some00/chocolate-doom.git
git submodule update --init
```

### Patch and Build BlueZ
BlueZ uses domain sockets for handling BLE writes without response. I needed to know the number of
packets queued in its internal buffer to keep latency low and the FPS high. This patch adds the
functionality to write back the number of queued packets after each transmission on the domain
socket originally intended to be written only.

1. Install BlueZ dependencies
    ```sh
    sudo apt install build-essential automake libtool pkg-config libglib2.0-dev libell-dev \
    libdbus-1-dev udev libudev-dev libical-dev libreadline-dev systemd
    ```
2. Clone BlueZ
    ```sh
    git clone -b 5.55 --depth=1 https://git.kernel.org/pub/scm/bluetooth/bluez.git
    ```
3. Patch
    ```sh
    patch -d bluez -p1 < chocolate-doom/doomtime/patches/bluez.patch
    ```
4. Configure and build
    ```sh
    cd bluez
    ./bootstrap
    ./configure --prefix=$PWD/_install --enable-external-ell
    make
    ```

### Build Chocolate Doom and the Client Application

1. Install dependencies
    ```sh
    sudo apt install cmake libsdl2-dev libsdl2-mixer-dev libsdl2-net-dev libeigen3-dev \
    libboost-dev libopencv-core-dev libopencv-dev libpython3-dev clang python3-dbus python3-numpy
    ```
2. Configure with:
    ```sh
    cmake -S . -B _build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
    ```
3. and build.
    ```sh
    cmake --build _build
    ```

### Build PineTime Firmware

1. Install dependencies. Background is converted to an array with Python during the build.
    ```sh
    apt install python3-numpy
    ```
2. Download and extract [GNU Arm Embedded Toolchain][toolchain]. Version 9-2020-q2 is proven to be
   working.
3. Add `<toolchain-root>/bin` to your `PATH` enviroment variable.
4. Install [newt][newt-install].
5. Upgrade newt.
    ```sh
    cd doomtime
    newt ugprade
    ```
6. Patch apache-mynewt-core. This patch is needed to be able start an SPI DMA write from the
   interrupt handler of a previous write when data is already available.
    ```sh
    patch -d repos/apache-mynewt-core -p1 < patches/mynewt-core.patch
    ```
7. Patch apache-mynewt-nimble. This patch is a compile fix for a nimble header when compiled with
   c++.
    ```sh
    patch -d repos/apache-mynewt-nimble -p1 < patches/mynewt-nimble.patch
    ```
8. Go back to the project root and build bootloader and app.
    ```sh
    newt build boot-pinetime
    newt build doomtime
    ```
9. For flashing I used a Raspberry Pi and bitbanged SWD on GPIO. My scripts for openocd are in the
   `scripts` folder. Pine64 Wiki has descriptions for [wiring][pine64_wiring] and
   [programming][pine64_prog] the watch.


<p align="right">(<a href="#top">back to top</a>)</p>


<!-- USAGE EXAMPLES -->
## Usage

WAD file for the game you want to play (Doom is shown below), a bluetooth adapter and a PineTime
are needed for these steps.


1. Stop the bluetooth deamon.
    ```sh
    sudo systemctl stop bluetooth
    ```
2. Start the patched bluetooth deamon. This won't print any messages on a successful start and keep
   the daemon in the foreground.
    ```sh
    sudo bluez/src/bluetoothd
    ```
3. In a seperate terminal start bluetoothctl, scan for the watch and connect.
    ```sh
    bluetoothctl
    >> power on
    >> scan on
    >> connect <mac-address-of-the-discovered-watch>
    >> scan off
    >> exit
    ```
4. Start Chocolate Doom and wait for the message "waiting for client" before starting the client.
   After 10 seconds of waiting the game times out and exits. WAD files are search in the current
   working directory or can be set with the `-iwad` flag. For more options see the
   [user guide][cd_guide].
    ```sh
    cd chocolate-doom
    ./_build/src/chocolate-doom
    ```
5. In a seperate terminal start the client. Available options are shown with the `--help' flag.
    ```sh
    cd chocolate-doom
    python ./_build/doomtime/host/client/client.py
    ```


<p align="right">(<a href="#top">back to top</a>)</p>


## Benchmark


My intention was to use as much of the screen as possible while keeping the original 35 FPS of the
game. The latter I deemed more important since the screen is already too small to enjoy the game.

8MHz SPI of the nRF52 combined with the lowest color depth of 12 bit per pixel of the display
made me choose 192x96 pixel for the screen.

The fastest I could achive with BLE was around 700 Kbps
which made me choose the lowest resolution of Doom 96x48. By using the low detail option I could
reduce data needed for a single frame to 48*48 bytes plus 1 byte for the palette index (the final
implementation sends the palette index after each column). Low detail mode is only available in
Doom, it renders every column twice.

The maximum number of bytes nRF52 can write with a single DMA is 255. To reduce the number of DMAs
the screen is written by columns.

By my calculations this configuration could reach 35 FPS. I made some crude tests by instrumenting
the display code to toggle a GPIO pin and refreshing the screen without BT transmission at all.
Unfortunatelly I could only measure around 33 refresh per second, with Bluetooth this drops down to
an average of a little over 30.

The following diagram shows an actual measurement. `Sent` is the reported "MV" value from the
client converted to FPS. MV is the number of microseconds the client sleeps between frames and is
recalculated after every second based on the number of average packets queued in BlueZ in the last
second. `Displayed` is from the measured frametime on GPIO converted to FPS. Each mark is a frame
displayed on the watch. PXX values are percentiles for statistics.

[![picture][benchdia]](benchdia)

Although I have failed to reach my goal of 35 FPS, I think the game is still playable. I tested
with various bluetooth adapters and the controller adopts quite well with a setting time of 20-30
seconds. Video as proof.

TODO embed video

The video was recorded in 50 FPS. By checking the frames, I estimate that the delay between the two
displays is around 3-6 camera frames (0.06-0.12s). This was also the point where I noticed that the
last column is missing from the background and kept it as-is.

<p align="right">(<a href="#top">back to top</a>)</p>


## License

Distributed under the MIT License. See `LICENSE.txt` for more information. Use it at your own risk.

<p align="right">(<a href="#top">back to top</a>)</p>

Readme template from [othneildrew].


<!-- MARKDOWN LINKS & IMAGES -->
[product-screenshot]: images/picture.png
[toolchain]: https://developer.arm.com/downloads/-/gnu-rm
[newt-install]: https://mynewt.apache.org/latest/newt/install/newt_linux.html#setting-up-your-computer-to-use-apt-get-to-install-the-package
[pine64_wiring]: https://wiki.pine64.org/wiki/PineTime_Devkit_Wiring
[pine64_prog]: https://wiki.pine64.org/wiki/Reprogramming_the_PineTime
[cd_guide]: https://www.chocolate-doom.org/wiki/index.php/User_guide
[othneildrew]: https://github.com/othneildrew/Best-README-Template
[benchdia]: images/bench.png
