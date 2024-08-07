pyopticam
================

This repository contains a comprehensive Python wrapper for the [Optitrack Camera SDK](https://optitrack.com/software/camera-sdk/) using [nanobind](https://github.com/wjakob/nanobind).  It exposes a wide variety of camera manipulation functions, as well as GIL-friendly video and marker retrieval functions for use with `numpy`.

Requires python >= 3.8.   This has only ever been tested with [Prime 13W](https://optitrack.com/cameras/prime-13w/)'s; different cameras may need some of the hard-coded resolution assumptions updated.

Installation
------------
0. Ensure you are using Python 3.8.10 and you're on Windows
1. Make a virtual environment with `python.exe -m venv .venv`, activate venv with `.\.venv\Scripts\activate`
2. Install the [Optitrack Camera SDK](https://optitrack.com/software/camera-sdk/) to: `C:/Program Files (x86)/OptiTrack/Camera SDK`
3. Clone this repository
4. Navigate to the top-level `pyopticam` directory and run `pip install -e . ` (Do not upgrade pip, use pip version `21.1.1`)
5. Find `C:/Program Files (x86)/OptiTrack/CameraSDK/lib/CameraLibrary2019x64S.dll` and copy it to `pyopticam/src/pyopticam`

Afterwards, make sure your Optitrack cameras are accessible on the network, and try the `video_viewer.py` or the `marker_viewer.py` example.

License
-------

_pyopticam_ is provided under the Apache V2 license that can be found in the [LICENSE](./LICENSE) file. 

By using, distributing, or contributing to this project, you agree to the terms and conditions of this license, and the licenses of _nanobind_ (BSD 3-Clause) and Optitrack's Camera SDK.
