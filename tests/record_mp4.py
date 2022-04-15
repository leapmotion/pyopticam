import pyopticam as m
import numpy as np
import time
import cv2
import sys
import subprocess
import ctypes
import mp4_thread

#m.cCameraLibraryStartupSettings.X().EnableDevelopment()

cameraManager = m.CameraManager.X()
print("Waiting for Cameras to Initialize...")
m.CameraManager.X().WaitForInitialization()

print("Is CameraManager Active:", m.CameraManager.IsActive())
cameras = m.GetCameraList(m.CameraManager.X())
print("Got CameraList; Number of Cameras:", cameras.Count())

frame_array          = [None] * cameras.Count()
camera_array         = [None] * cameras.Count()
camera_entries_array = [None] * cameras.Count()

for i in range(cameras.Count()):
    camera_entries_array[i] = cameras.get(i)

    print("CameraEntry", i, "Parameters:", #UID", camera.UID(), 
        ", Serial:", camera_entries_array[i].Serial(), ", SerialString:", camera_entries_array[i].SerialString(),
        ", Name:", camera_entries_array[i].Name(), ", State:", camera_entries_array[i].State(), 
        ", IsVirtual:", camera_entries_array[i].IsVirtual(), ", Revision:", camera_entries_array[i].Revision())

    if camera_entries_array[i].State() == m.eCameraState.Initialized:
        # Get the actual camera object associated with this UID
        print("About to get Camera: ", i)
        time.sleep(0.1)
        camera_array[i] = m.CameraManager.X().GetCamera(camera_entries_array[i].UID())#BySerial(cameraEntry.Serial())
        print("Got Camera: ", i)
        #time.sleep(0.1)

sync = m.cModuleSync.Create()
for i in range(len(camera_array)):
    sync.AddCamera(camera_array[i], 0)

for i in range(len(camera_array)):
     #camera_array[i].SetExposure(8000)
     print("Camera", i, "Parameters: Name", camera_array[i].Name() , ", Framerate:", camera_array[i].FrameRate(), 
         ", Exposure:", camera_array[i].Exposure(),", Threshold:", camera_array[i].Threshold(), 
         ", Intensity:", camera_array[i].Intensity(),", CameraID:", camera_array[i].CameraID())
     camera_array[i].SetStatusRingRGB(0, 255, 0)

     print("Setting MJPEG Mode")
     camera_array[i].SetVideoType(m.eVideoMode.MJPEGMode) # and GrayscaleMode work
     camera_array[i].SetExposure(250)
     #camera_array[i].SetThreshold(150)
     #camera_array[i].SetIntensity(5)
     print("Starting Camera...")
     camera_array[i].Start()
     #
     #time.sleep(0.5)

     #print("Enabled Text Overlay...")
     ## Get Frame off of Camera!
     #frame_array[i] = camera_array[i].GetFrame()
     #image_frame = m.GetCameraFrame(frame_array[i])

     #print("Retrieved Frame!", image_frame.shape)
     #cv2.imwrite("CameraFrame"+str(i)+".png", image_frame)

ffmpeg_process = mp4_thread.ffmpegThread("OptitrackOutput.mp4", width=640 / 4, height=512 * len(camera_array) / 4)
ffmpeg_process.start()

print("Starting to retrieve frame groups...")
while(not (cv2.waitKey(4) & 0xFF == ord('q'))):
    #print("Retrieving FrameGroup...")
    image_frame = m.GetFrameGroupArray(sync)
    image_frame = np.reshape(image_frame, (-1, image_frame.shape[2]))
    image_frame = cv2.resize(image_frame, (int(ffmpeg_process.width), int(ffmpeg_process.height)))
    ffmpeg_process.add_image(np.copy(image_frame))
    cv2.imshow("CameraFrame - " + str(i), np.copy(image_frame))

    #if ffmpeg_process.num_frames_input % 100 == 0:
    #    #print("Processed", ffmpeg_process.num_frames_input, "frames")
    #    print("Images backed up in the FMMPEG Thread: ", len(ffmpeg_process.image_queue))

print("Killing FFMPEG process...")
ffmpeg_process.end_encoding()

sync.RemoveAllCameras()
m.cModuleSync.Destroy(sync)

for i in range(len(camera_array)):
    if camera_array[i].State() == m.eCameraState.Initialized:
        print("Stopping Camera", i, "...")
        #camera_array[i].Stop(True)
        print("Stopped Camera", i," Releasing Camera...")
        camera_array[i].Release()
        print("Released Camera!")

cameraManager.Shutdown()
cv2.destroyAllWindows()
#m.CameraManager.DestroyInstance()
print("Fin!")