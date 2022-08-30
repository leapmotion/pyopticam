pyopticam
================

This repository contains yet another Python wrapper for the [Optitrack Camera SDK](https://optitrack.com/software/camera-sdk/) using [nanobind](https://github.com/wjakob/nanobind).  Requires python >= 3.8.

Installation
------------

0. Ensure that you have the [Optitrack Camera SDK](https://optitrack.com/software/camera-sdk/) installed to: `C:/Program Files (x86)/OptiTrack/Camera SDK`
1. Clone this repository
2. Navigate to the top-level `pyopticam` directory and run `pip install -e .`
3. Find `C:/Program Files (x86)/OptiTrack/Camera SDK/lib/CameraLibrary2015x64S.dll` and copy it to `pyopticam/src/pyopticam`

Afterwards, make sure your Optitrack cameras are accessible on the network, and try the `video_viewer.py` or the `marker_viewer.py` example.

License
-------

_nanobind_ and this repository are both provided under a BSD-style
license that can be found in the [LICENSE](./LICENSE) file. By using,
distributing, or contributing to this project, you agree to the terms and
conditions of this license.
