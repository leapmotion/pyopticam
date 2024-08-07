import cv2
import numpy as np
import mp4_thread
import optitrack_thread

# A simple example for viewing and recording mp4's from your optitrack cameras
# Press 'w' to toggle recording, press `q` to quit

optitrack = optitrack_thread.OptitrackThread(exposure=150, delay_strobe=False)
optitrack.start()
ffmpeg_recording = False

fake = np.zeros((16, 16), dtype=np.uint8)
num_cams = 1
output_width  = int(640 / 2)
#output_height = int(512 * num_cams / 2) # Dependent on the number of cameras; see below

try:
    print("Starting to retrieve frame groups...")
    keypress = cv2.waitKey(1)
    while(not (keypress & 0xFF == ord('q'))):
        if optitrack.newFrame:
            image_frame = optitrack.read()
            num_cams = image_frame.shape[0]
            output_height = int(512 * num_cams / 2)
            if image_frame.shape[1] > 1:
                image_frame = np.reshape(image_frame, (-1, image_frame.shape[2]))
                #image_frame        = np.vstack((np.hstack((image_frame[0], image_frame[1], image_frame[2], image_frame[3])),
                #                                np.hstack((image_frame[4], image_frame[5], image_frame[6], image_frame[7]))))
                #image_frame = cv2.resize(image_frame, (output_width, output_height))

                if (keypress & 0xFF == ord('w')):
                    if not ffmpeg_recording:
                        ffmpeg_process = mp4_thread.ffmpegThread("OptitrackOutput.mp4", width=output_width, height=output_height)#image_frame.shape[1], height=image_frame.shape[0])
                        ffmpeg_process.start()
                        ffmpeg_recording = True
                    else: 
                        print("Killing FFMPEG process...")
                        ffmpeg_process.end_encoding()
                        ffmpeg_recording = False

                if ffmpeg_recording:
                    ffmpeg_process.add_image(image_frame)
                cv2.imshow("CameraFrame", image_frame)

        keypress = cv2.waitKey(1)
finally:
    if ffmpeg_recording:
        print("Killing FFMPEG process...")
        ffmpeg_process.end_encoding()
        ffmpeg_recording = False

    optitrack.stop()

    cv2.destroyAllWindows()
    print("Fin!")
