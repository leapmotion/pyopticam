import pyopticam as m
import numpy as np
import time
import cv2

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
     camera_array[i].SetExposure(100)
     camera_array[i].SetShutterDelay(100 * i) # Keep the cameras from firing into eachother?
     camera_array[i].SetStrobeOffset(100 * i) # Keep the cameras from firing into eachother?
     #camera_array[i].SetThreshold(150)
     #camera_array[i].SetIntensity(5)
     print("Starting Camera...")
     camera_array[i].Start()

num_images = 0

print("Starting to retrieve frame groups...")
keyPressed = cv2.waitKey(1)
while(not (keyPressed & 0xFF == ord('q'))):
    image_frame = m.GetFrameGroupArray(sync)

    if (keyPressed & 0xFF == ord('q')):

        
        for camera_index in range(image_frame.shape[0]):
            cv2.imwrite("./camera_" + str(camera_index) + "/camera_" + str(camera_index) + "_" + str(num_images) + ".jpg", image_frame[camera_index])
        num_images += 1

    keyPressed = cv2.waitKey(1)

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