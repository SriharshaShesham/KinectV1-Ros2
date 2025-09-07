# 1. Building libfreenect from Source on Ubuntu 22.04

## 1. Remove APT Version (Clean Slate)

```sh
sudo apt remove libfreenect-bin libfreenect0.5 libfreenect-dev
sudo apt autoremove
```

## 2. Install Build Dependencies

```sh
sudo apt update
sudo apt install git cmake build-essential libusb-1.0-0-dev libgl-dev
sudo apt install freeglut3-dev libxmu-dev libxi-dev libasound2-dev
```

## 3. Build with Firmware Support (CRITICAL for Model 1517)

```sh
git clone https://github.com/OpenKinect/libfreenect.git
cd libfreenect
mkdir build && cd build

cmake .. \
    -DBUILD_REDIST_PACKAGE=OFF \
    -DBUILD_AUDIO=ON \
    -DBUILD_EXAMPLES=ON \
    -DOpenGL_GL_PREFERENCE=GLVND \
    -DCMAKE_INSTALL_PREFIX=/usr/local

make -j$(nproc)
ls -la audios.bin  # Verify firmware downloaded
sudo make install
sudo ldconfig
```

## 4. Install udev Rules and Permissions

```sh
sudo cp ../platform/linux/udev/51-kinect.rules /etc/udev/rules.d/
sudo adduser $USER video
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## 5. Remove Conflicting Kernel Modules

```sh
sudo modprobe -r gspca_kinect || true
echo 'blacklist gspca_kinect' | sudo tee -a /etc/modprobe.d/blacklist.conf
```

## 6. Log Out and Back In, Then Test

```sh
lsusb | grep Microsoft  # Should show 3 devices
/usr/local/bin/freenect-glview  # Should work without sudo
```

---


# 2. Kinect v1 Automatic Firmware Upload on Connection


## Background

The Kinect v1 ships without active firmware in RAM. When powered on, the host PC must upload the runtime firmware over USB before the camera, depth, and audio streams will work. And hence I had to run the `freenect-micview` first before I run the camtest or depth test. If you run `freenect-glview` without doing this first, it fails to start. Looks like this firmware is valid only until it is disconnected. This guide explains how to automatically upload firmware to a Kinect v1 sensor when it’s plugged in, without having to manually run `freenect-micview`  each time.


## Solution Overview

We’ll create:
1. A small shell script that runs `freenect-micview` briefly to trigger the firmware upload, then exits.
2. A `udev` rule that runs this script automatically when the Kinect is connected.
3. Sample files uploaded [here](./samplefiles)
---

## 1. Create the Firmware Kick Script

Save the following as `/usr/local/bin/kinect-fwkick.sh`:

```sh
#!/bin/bash
# Path to freenect-micview (adjust if different)
MICVIEW_BIN="$(command -v freenect-micview)"

if [ -z "$MICVIEW_BIN" ]; then
    echo "freenect-micview not found in PATH"
    exit 1
fi

# Launch micview in the background to trigger firmware upload
"$MICVIEW_BIN" &
PID=$!

# Wait a few seconds for firmware to upload
sleep 5

# Kill micview so the device is free for other apps
kill "$PID" 2>/dev/null
```

### Make it executable:
```sh
sudo chmod +x /usr/local/bin/kinect-fwkick.sh

```

## 2. Identify Kinect USB IDs
Plug in your Kinect and run:

```sh
lsusb | grep -i kinect
```
### For Kinect v1, you’ll typically see something like:

```sh
Bus 002 Device 005: ID 045e:02ae Microsoft Corp. Xbox NUI Audio
```
#### idVendor = 045e
#### idProduct = 02ae (audio device — triggers firmware load)

## 3. Create the udev Rule
Create /etc/udev/rules.d/51-kinect-fwkick.rules:

```sh
SUBSYSTEM=="usb", ATTR{idVendor}=="045e", ATTR{idProduct}=="02bf", ACTION=="add", RUN+="/usr/local/bin/kinect-fwkick.sh"
```


## 4. Reload udev Rules

```sh 
sudo udevadm control --reload-rules
sudo udevadm trigger

```

## 5. Test
Unplug the Kinect.Connect it back in.

Wait ~5 seconds for the firmware to upload automatically.

Run:

```sh
freenect-glview
```

It should now work without running freenect-micview manually.