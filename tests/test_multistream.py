import pyopticam as m
import numpy as np
import time
import cv2
import ctypes

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

        if camera_array[i] is not None:
            camera_array[i].SetExposure(8000)
            print("Camera", i, "Parameters: Name", camera_array[i].Name() , ", Framerate:", camera_array[i].FrameRate(), 
                ", Exposure:", camera_array[i].Exposure(),", Threshold:", camera_array[i].Threshold(), 
                ", Intensity:", camera_array[i].Intensity(),", CameraID:", camera_array[i].CameraID())

            
            #color = m.sStatusLightColor()
            #color.Red = 255
            #color.Green = 255
            #color.Blue = 255
            #camera_array[i].SetStatusRingLights(10, color)
            #camera_array[i].SetStatusRingRGB(255, 255, 255)

            print("Setting Grayscale Mode")
            camera_array[i].SetVideoType(m.eVideoMode.MJPEGMode) # and GrayscaleMode work
            camera_array[i].SetExposure(8000)
            camera_array[i].SetThreshold(1)
            camera_array[i].SetIntensity(15)
            #time.sleep(0.5)
            #print("Enabling Text Overlay...")
            #camera_array[i].SetTextOverlay(True)
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

while(not (cv2.waitKey(100) & 0xFF == ord('q'))):
    for i in range(len(camera_array)):
        #if camera_array[i].State() == m.eCameraState.Initialized:
            # Get Frame off of Camera!
            frame_array[i] = camera_array[i].GetFrame()
            image_frame = np.copy(m.GetCameraFrame(frame_array[i]))
            #print("Retrieved Frame!", image_frame.shape)
            cv2.imshow("CameraFrame - " + str(i), image_frame)
            #frame_array[i].Release()

for i in range(len(camera_array)):
    if camera_array[i].State() == m.eCameraState.Initialized:
        print("Stopping Camera", i, "...")
        camera_array[i].Stop(True)
        time.sleep(0.1)
        print("Stopped Camera", i," Releasing Camera...")
        camera_array[i].Release()
        time.sleep(0.1)
        print("Released Camera!")

cv2.destroyAllWindows()
#cameraManager.Shutdown()
m.CameraManager.DestroyInstance()
