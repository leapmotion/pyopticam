import pyopticam as m
import numpy as np
import time
import cv2
import ctypes

def test_add():
    assert m.add(1, 2) == 3

#print("Add 1 + 2 =", m.add(1, 2))

#print("Get Input Numpy Tensor Data")
#m.inspect(np.array([1, 2, 3]))

#print("Return a numpy tensor", m.ret_numpy())

#m.cCameraLibraryStartupSettings.X().EnableDevelopment()

cameraManager = m.CameraManager.X()
#if not cameraManager.AreCamerasInitialized():
print("Waiting for Cameras to Initialize...")
m.CameraManager.X().WaitForInitialization()

print("Is CameraManager Active:", m.CameraManager.IsActive())
#camera = cameraManager.GetCameraBySerial(27465)#GetCamera()
#print(camera)

#print("Pointer to CameraManager", m.CameraManager.Ptr)
#print("First Timestamp", cameraManager.TimeStamp())
#print("Second Timestamp", cameraManager.TimeStamp())
#print("CameraList Functions: ", dir(m.CameraList))
cameras = m.GetCameraList(m.CameraManager.X())
print("Got CameraList; Number of Cameras:", cameras.Count())

frame_array  = [None] * cameras.Count()
camera_array = [None] * cameras.Count()
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
        time.sleep(0.3)
        camera_array[i] = m.CameraManager.X().GetCamera(camera_entries_array[i].UID())#BySerial(cameraEntry.Serial())
        print("Got Camera: ", i)
        time.sleep(0.3)

        if camera_array[i] is not None:
            camera_array[i].SetExposure(8000)
            print("Camera", i, "Parameters: Name", camera_array[i].Name() , ", Framerate:", camera_array[i].FrameRate(), 
                ", Exposure:", camera_array[i].Exposure(),", Threshold:", camera_array[i].Threshold(), 
                ", Intensity:", camera_array[i].Intensity(),", CameraID:", camera_array[i].CameraID())

            try:
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
                time.sleep(0.5)

                #print("Enabled Text Overlay...")
                # Get Frame off of Camera!
                frame_array[i] = camera_array[i].GetFrame()
                image_frame = m.GetCameraFrame(frame_array[i])
                #print("Frame Object", image_frame.shape)
                #print("Width", frame_array[i].Width(), "Height", frame_array[i].Height(), 
                #    "Is GrayScale", frame_array[i].IsGrayscale(), "Grayscale Size:", frame_array[i].GetGrayscaleDataSize())

    #            ##np.ctypeslib.as_array(frame.GetGrayscaleData(), (frame.Height(), frame.Width()))

    #            #frame = camera.GetFrame()
    #            #print("Frame is GrayScale: ", frame.IsGrayscale())
    #            #g = (ctypes.c_char*frame.GetGrayscaleDataSize()).from_address(frame.GetGrayscaleData())
    #            #print("Retrieved frame Object:", frame, g)
    #            #print("Numpy Conversion happening now...")
    #            #image_frame = np.ctypeslib.as_array(g, (frame.Height(), frame.Width()))
    #            #print("Retrieved Image Frame:", image_frame.shape, image_frame)


                #print("Retrieved Frame!", frame.shape)
                cv2.imwrite("CameraFrame"+str(i)+".png", image_frame)
            finally:
                print("Stopping Camera...")
                #camera_array[i].Stop(True)
                #time.sleep(0.1)
                #print("Stopped Camera, Releasing Camera...")
                #camera_array[i].Release()
                #time.sleep(0.1)
                #print("Released Camera!")

#cameraManager.Shutdown()
m.CameraManager.DestroyInstance()
