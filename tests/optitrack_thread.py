import numpy as np
import cv2
import threading
import time
import pyopticam as m

class OptitrackThread(threading.Thread):
    '''A thread for receiving images from the Optitrack SDK'''
    def __init__(self, mode=m.eVideoMode.MJPEGMode, exposure=50, delay_strobe=False):
        '''Initialize Optitrack Receptiom'''
        threading.Thread.__init__(self)
        self.should_run = True
        self.deadmansSwitch = time.time()
        self.t = 0
        self.current_frame = None
        
        self.cameraManager = m.CameraManager.X()
        print("Waiting for Cameras to Initialize...")
        m.CameraManager.X().WaitForInitialization()

        print("Is CameraManager Active:", m.CameraManager.IsActive())
        self.cameras = m.GetCameraList(m.CameraManager.X())
        print("Got CameraList; Number of Cameras:", self.cameras.Count())

        self.frame_array          = [None] * self.cameras.Count()
        self.camera_array         = [None] * self.cameras.Count()
        self.camera_entries_array = [None] * self.cameras.Count()

        for i in range(self.cameras.Count()):
            self.camera_entries_array[i] = self.cameras.get(i)

        # Sort the Camera Entries by Serial Number for deterministic enumeration
        self.camera_entries_array = sorted(self.camera_entries_array, key=lambda x: x.Serial()) 

        for i in range(self.cameras.Count()):
            print("CameraEntry", i, "Parameters:", #UID", camera.UID(), 
                ", Serial:", self.camera_entries_array[i].Serial(), ", SerialString:", self.camera_entries_array[i].SerialString(),
                ", Name:", self.camera_entries_array[i].Name(), ", State:", self.camera_entries_array[i].State(), 
                ", IsVirtual:", self.camera_entries_array[i].IsVirtual(), ", Revision:", self.camera_entries_array[i].Revision())

            if self.camera_entries_array[i].State() == m.eCameraState.Initialized:
                # Get the actual camera object associated with this UID
                print("About to get Camera: ", i)
                time.sleep(0.1)
                self.camera_array[i] = m.CameraManager.X().GetCamera(self.camera_entries_array[i].UID())#BySerial(cameraEntry.Serial())
                print("Got Camera: ", i)

        self.sync = m.cModuleSync.Create()
        for i in range(len(self.camera_array)):
            self.sync.AddCamera(self.camera_array[i], 0)

        self.mode = mode
        self.exposure = exposure
        self.delay_strobe = delay_strobe
        for i in range(len(self.camera_array)):
            print("Camera", i, "Parameters: Name", self.camera_array[i].Name() , ", Framerate:", self.camera_array[i].FrameRate(), 
                  ", Exposure:" , self.camera_array[i].Exposure(), ", Threshold:", self.camera_array[i].Threshold(), 
                  ", Intensity:", self.camera_array[i].Intensity(),", CameraID:" , self.camera_array[i].CameraID())
            self.camera_array[i].SetStatusRingRGB(0, 255, 0)

            print("Setting MJPEG Mode")
            self.camera_array[i].SetVideoType(self.mode) # and GrayscaleMode work
            self.camera_array[i].SetExposure(self.exposure)
            self.camera_array[i].SetIntensity(10)
            self.camera_array[i].SetImagerGain(m.eImagerGain.Gain_Level7)
            if self.delay_strobe:
                self.camera_array[i].SetShutterDelay(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?
                self.camera_array[i].SetStrobeOffset(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?
            #camera_array[i].SetThreshold(150)
            #camera_array[i].SetIntensity(5)
            print("Starting Camera...")
            self.camera_array[i].Start()


    def run(self):
        print("Beginning Optitrack Receive Thread!")

        while(self.should_run and time.time() - self.deadmansSwitch < 4.0):
            #time_ms = (time.perf_counter()-self.t)*(10 ** 3)
            #self.t = time.perf_counter()
            self.current_frame = m.GetFrameGroupArray(self.sync)
            time.sleep(0.001)

        self.sync.RemoveAllCameras()
        m.cModuleSync.Destroy(self.sync)

        for i in range(len(self.camera_array)):
            if self.camera_array[i].State() == m.eCameraState.Initialized:
                print("Stopping Camera", i, "...")
                #camera_array[i].Stop(True)
                print("Stopped Camera", i," Releasing Camera...")
                self.camera_array[i].Release()
                print("Released Camera!")

        self.cameraManager.Shutdown()

        print("Exiting Optitrack Camera Thread!")
    def read(self):
        '''Retrieves the most recent frame from the system'''
        #image_frame = np.reshape(image, (self.height, self.width))
        #self.image_queue.put(image)
        self.deadmansSwitch = time.time()
        return self.current_frame
    def stop(self):
        '''Ends the video receiving thread'''
        self.should_run = False
