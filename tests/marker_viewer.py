import optitrack_thread
import numpy as np
import time
import cv2
import pyopticam as m

num_images = 0
num_cams = 0
last_time = time.time()
optitrack = optitrack_thread.OptitrackThread(mode=m.eVideoMode.ObjectMode)
optitrack.start()

colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0), (0, 255, 255), (255, 0, 255), (255, 127, 0), (127, 0, 255), (127, 0, 127)]
t = 0
print("Starting to retrieve frame groups...")
keyPressed = cv2.waitKey(1)
while(not (keyPressed & 0xFF == ord('q'))):
    image_frame = optitrack.read()

    if image_frame is None:
        print("Received None Frame!")
        #time.sleep(0.1)
        continue

    time_ms = (time.perf_counter()-t)*(10 ** 3)
    t = time.perf_counter()
    print("Time: %.2fms" % time_ms)

    #print("Image Shape: ", image_frame.shape, image_frame)
    full_image = np.zeros((1024, 1280, 3), dtype=np.uint8)
    for camera_index in range(image_frame.shape[0]):
        for marker_index in range(image_frame.shape[1]):
            if np.min(image_frame[camera_index,marker_index]) > 5 and np.max(image_frame[camera_index,marker_index]) < 1280:
                center = (int(image_frame[camera_index,marker_index,0]),
                          int(image_frame[camera_index,marker_index,1]))
                radius =  int(image_frame[camera_index,marker_index,2])
                #print("Drawing Marker", center, radius)
                cv2.circle(full_image, center, radius, colors[camera_index], -1)
    cv2.imshow("Combined Camera Frames", full_image)

    keyPressed = cv2.waitKey(1)

optitrack.stop()
cv2.destroyAllWindows()
print("Fin!")
