import pyopticam as m
import numpy as np
import time
import cv2
import optitrack_thread
import json

num_images = 0
last_time = time.time()
optitrack = optitrack_thread.OptitrackThread()
optitrack.start()

# Define the dimensions of checkerboard
CHECKERBOARD = (9, 7)
# 3D points real world coordinates
objectp3d = np.zeros((1, CHECKERBOARD[0]
                       * CHECKERBOARD[1],
                      3), np.float32)
objectp3d[0, :, :2] = np.mgrid[0:CHECKERBOARD[0],
                               0:CHECKERBOARD[1]].T.reshape(-1, 2)

with(open('./tests/working_optitrack_calibration_opencv.json', 'r')) as f:
    calib = json.load(f)
print("Camera Calibrations:", calib)

cam0_params = calib['Calibration']['cameras'][0]['model']['ptr_wrapper']['data']['parameters']
cam1_params = calib['Calibration']['cameras'][1]['model']['ptr_wrapper']['data']['parameters']
print(cam0_params)
cam0_mat = np.array([[   cam0_params['f']['val'], 0, cam0_params['cx']['val']],
                     [0, cam0_params['f']['val'],    cam0_params['cy']['val']],
                     [0, 0, 1]], dtype=np.float32)
cam0_dist = np.array([[0,0,0,0]], dtype=np.float32)
cam1_mat = np.array([[   cam1_params['f']['val'], 0, cam1_params['cx']['val']],
                     [0, cam1_params['f']['val'],    cam1_params['cy']['val']],
                     [0, 0, 1]], dtype=np.float32)
cam1_dist = np.array([[0,0,0,0]], dtype=np.float32)

cam0_rot = calib['Calibration']['cameras'][0]['transform']['rotation']
cam1_rot = calib['Calibration']['cameras'][1]['transform']['rotation']
cam0_rot = np.array([[cam0_rot['rx'], cam0_rot['ry'], cam0_rot['rz']]], dtype=np.float32)
cam1_rot = np.array([[cam1_rot['rx'], cam1_rot['ry'], cam1_rot['rz']]], dtype=np.float32)

cam0_trans = calib['Calibration']['cameras'][0]['transform']['translation']
cam1_trans = calib['Calibration']['cameras'][1]['transform']['translation']
cam0_trans = np.array([[cam0_trans['x'], cam0_trans['y'], cam0_trans['z']]], dtype=np.float32)
cam1_trans = np.array([[cam1_trans['x'], cam1_trans['y'], cam1_trans['z']]], dtype=np.float32)

print("Starting to retrieve frame groups...")
keyPressed = cv2.waitKey(1)
while(not (keyPressed & 0xFF == ord('q'))):
    image_frame = optitrack.read()

    ret, corners = cv2.findChessboardCorners(
                    image_frame[0], CHECKERBOARD,
                    cv2.CALIB_CB_ADAPTIVE_THRESH
                    + cv2.CALIB_CB_FAST_CHECK +
                    cv2.CALIB_CB_NORMALIZE_IMAGE)
 
    if ret == True:
        cv2.drawChessboardCorners(image_frame[0], CHECKERBOARD, corners, ret)
        print("Corners", corners, corners.shape, corners.dtype)

        # Find the rotation and translation vectors.
        ret, rvecs, tvecs = cv2.solvePnP(objectp3d, corners, cam0_mat, cam0_dist)

        print("3D Points Shape:", objectp3d.shape)

        # Transform the 3D points by their solved translation and rotation
        objectp3d_rotated = cv2.Rodrigues(rvecs)[0].dot(objectp3d[0].T).T
        objectp3d_rotated_translated = (objectp3d_rotated + tvecs[0])[None, :, :]

        print("3D Points Shape After:", objectp3d_rotated_translated.shape)

        # Project to the next camera and draw
        print("Moved Points:", objectp3d_rotated_translated, objectp3d_rotated_translated.shape)
        imgpts, jac = cv2.projectPoints(objectp3d_rotated_translated, cam1_rot, cam1_trans, cam1_mat, cam1_dist)
        imgpts = imgpts.astype(np.float32)
        print("Imgpts", imgpts, imgpts.shape, imgpts.dtype)
        cv2.drawChessboardCorners(image_frame[1], CHECKERBOARD, imgpts, True)

    cv2.imshow("Camera Frames", np.hstack((image_frame[0], image_frame[1])))

    keyPressed = cv2.waitKey(1)

optitrack.stop()
cv2.destroyAllWindows()
print("Fin!")