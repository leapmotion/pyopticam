import cv2
import time
import numpy as np
import pyopticam
import optitrack_thread

# A simple example for viewing marker data from your optitrack cameras

min_size = 1 # 5
max_size = 100000

optitrack = optitrack_thread.OptitrackThread(mode=pyopticam.eVideoMode.ObjectMode, delay_strobe=True)
optitrack.start()

full_image = np.zeros((1024, 1280, 3), dtype=np.uint8)
colors = [(255, 0,   0), (  0, 255, 0), (  0, 0, 255), (255, 255,   0), (0, 255, 255), 
          (255, 0, 255), (255, 127, 0), (127, 0, 255), (127,   0, 127)]

print("Starting to retrieve frame groups...")
keyPressed = cv2.waitKey(1)
while(not (keyPressed & 0xFF == ord('q'))):
    marker_frame = optitrack.read()

    if marker_frame is None:
        print("Received None Frame!")
        time.sleep(0.1)
        continue

    #print("Image Shape: ", image_frame.shape, image_frame)
    full_image[:, :, :] = 0
    for camera_index in range(marker_frame.shape[0]):
        for marker_index in range(marker_frame.shape[1]):
            if np.min(marker_frame[camera_index, marker_index]) > min_size and np.max(marker_frame[camera_index, marker_index]) < max_size:
                center = (int(marker_frame[camera_index, marker_index, 0]),
                          int(marker_frame[camera_index, marker_index, 1]))
                radius =  int(marker_frame[camera_index, marker_index, 2])
                cv2.circle(full_image, center, radius, colors[camera_index % len(colors)], -1)
    cv2.imshow("Combined Camera Frames", full_image)

    keyPressed = cv2.waitKey(1)

optitrack.stop()
cv2.destroyAllWindows()
print("Fin!")
