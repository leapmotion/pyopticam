import numpy as np
import cv2
import mp4_thread
import optitrack_thread

optitrack = optitrack_thread.OptitrackThread()
optitrack.start()
ffmpeg_recording = False

fake = np.zeros((16, 16), dtype=np.uint8)
num_cams = 8

print("Starting to retrieve frame groups...")
keypress = cv2.waitKey(1)
while(not (keypress & 0xFF == ord('q'))):
    if (keypress & 0xFF == ord('w')):
        if not ffmpeg_recording:
            ffmpeg_process = mp4_thread.ffmpegThread("OptitrackOutput.mp4", width=640/2, height=512 * num_cams /2)
            ffmpeg_process.start()
            ffmpeg_recording = True
        else: 
            print("Killing FFMPEG process...")
            ffmpeg_process.end_encoding()
            ffmpeg_recording = False

    image_frame = optitrack.read()
    num_cams = image_frame.shape[0]
    if image_frame.shape[1] > 1:
        image_frame = np.reshape(image_frame, (-1, image_frame.shape[2]))
        image_frame = cv2.resize(image_frame, (int(640/2), int(512 * num_cams /2)))
        if ffmpeg_recording:
            ffmpeg_process.add_image(image_frame)
        cv2.imshow("CameraFrame", image_frame)

    keypress = cv2.waitKey(1)

optitrack.stop()

cv2.destroyAllWindows()
print("Fin!")
