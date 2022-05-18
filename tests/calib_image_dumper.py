import optitrack_thread
import numpy as np
import time
import cv2
import os

num_images = 0
num_cams = 0
last_time = time.time()
optitrack = optitrack_thread.OptitrackThread()
optitrack.start()

print("Starting to retrieve frame groups...")
keyPressed = cv2.waitKey(1)
while(not (keyPressed & 0xFF == ord('q'))):
    image_frame = optitrack.read()

    #if (keyPressed & 0xFF == ord('w')):
    num_cams = image_frame.shape[0]
    #if time.time() > last_time + 1.0:
    if (keyPressed & 0xFF == ord('w')):
        if (num_cams == 8):
            for camera_index in range(image_frame.shape[0]):
                if not os.path.isdir("./calib_images/camera_" + str(camera_index)):
                    os.makedirs("./calib_images/camera_" + str(camera_index))

                # Delete all the images in those directories to make way for the next calibration attempt
                if num_images == 0:
                    for f in os.listdir("./calib_images/camera_" + str(camera_index)):
                        os.remove(os.path.join("./calib_images/camera_" + str(camera_index), f))

                cv2.imwrite("./calib_images/camera_" + str(camera_index) + "/camera_" + str(camera_index) + "_" + str(num_images) + ".png", image_frame[camera_index])
            num_images += 1
            print("Dumped", num_images, "image")
            last_time += 1.0

    #image_frame = np.reshape(image_frame, (-1, image_frame.shape[2]))
    if num_cams == 8:
        image_frame        = np.vstack((np.hstack((image_frame[0], image_frame[1], image_frame[2], image_frame[3])),
                                        np.hstack((image_frame[4], image_frame[5], image_frame[6], image_frame[7]))))
        image_frame = cv2.resize(image_frame, (int(image_frame.shape[1]/2), int(image_frame.shape[0]/2)))
        cv2.imshow("CameraFrame", image_frame)
    else:
        print("Received fewer than 8 cameras", image_frame.shape)
        image_frame = np.reshape(image_frame, (-1, image_frame.shape[2]))
        cv2.imshow("CameraFrame", image_frame)


    keyPressed = cv2.waitKey(1)

optitrack.stop()
cv2.destroyAllWindows()
print("Fin!")
