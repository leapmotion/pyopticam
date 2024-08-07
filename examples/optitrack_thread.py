import numpy as np
import cv2
import threading
import time
import pyopticam as m

class OptitrackThread(threading.Thread):
    '''A thread for receiving images from the Optitrack SDK'''
    def __init__(self, mode=m.eVideoMode.MJPEGMode, exposure=50, delay_strobe=False, framerate=-1):
        '''Initialize Optitrack Receptiom'''
        threading.Thread.__init__(self)
        self.should_run = True
        self.deadmansSwitch = time.time()
        self.t = 0
        self.current_frame = None #np.zeros((1, 1, 1))

        print("Waiting for Cameras to Initialize...")
        m.CameraManager.X().WaitForInitialization()

        print("Are Cameras Initialized?:", m.CameraManager.X().AreCamerasInitialized())

        print("Is CameraManager Active:", m.CameraManager.X().IsActive())
        
        self.cameras = m.GetCameraList(m.CameraManager.X())
        print("Got CameraList; Number of Cameras:", self.cameras.Count())

        self.frame_array          = [None] * self.cameras.Count()
        self.camera_array         = []#[None] * self.cameras.Count()
        self.camera_entries_array = [None] * self.cameras.Count()

        for i in range(self.cameras.Count()):
            self.camera_entries_array[i] = self.cameras.get(i)

        # Sort the Camera Entries by Serial Number for deterministic enumeration
        self.camera_entries_array = sorted(self.camera_entries_array, key=lambda x: x.Serial())
        self.camera_serials = []

        self.mode = mode
        self.exposure = exposure
        self.delay_strobe = delay_strobe
        self.framerate = framerate

        # The limit to 4 grayscale cameras here is based on a 1 gigabit switch; 10 gigabit switches may support more
        num_cameras_to_enumerate = min(self.cameras.Count(), 4 if self.mode == m.eVideoMode.GrayscaleMode else 256)
        for i in range(num_cameras_to_enumerate):
            print("CameraEntry", i, "Parameters:", #UID", camera.UID(), 
                ", Serial:", self.camera_entries_array[i].Serial(), ", SerialString:", self.camera_entries_array[i].SerialString(),
                ", Name:", self.camera_entries_array[i].Name(), ", State:", self.camera_entries_array[i].State(), 
                ", IsVirtual:", self.camera_entries_array[i].IsVirtual(), ", Revision:", self.camera_entries_array[i].Revision())

            if self.camera_entries_array[i].State() == m.eCameraState.Initialized:
                # Get the actual camera object associated with this UID
                print("About to get Camera: ", i)
                self.camera_array.append(m.CameraManager.X().GetCamera(self.camera_entries_array[i].UID()))#BySerial(cameraEntry.Serial())
                self.camera_serials.append(self.camera_entries_array[i].Serial())
                print("Got Camera: ", i)
                time.sleep(0.05)

        if self.mode != m.eVideoMode.GrayscaleMode:
            print("Creating Sync Object...")
            self.sync = m.cModuleSync.Create()
            for i in range(len(self.camera_array)):
                self.sync.AddCamera(self.camera_array[i])
        else:
            self.sync = None

        for i in range(len(self.camera_array)):
            #print("Camera", i, "Parameters: Name", self.camera_array[i].Name() , ", Framerate:", self.camera_array[i].FrameRate(), 
            #      ", Exposure:" , self.camera_array[i].Exposure(), ", Threshold:", self.camera_array[i].Threshold(), 
            #      ", Intensity:", self.camera_array[i].Intensity(),", CameraID:" , self.camera_array[i].CameraID())
            self.camera_array[i].SetStatusRingRGB(0, 255, 0)
            self.camera_array[i].SetNumeric(True, i+1)

            print("Setting", self.mode)
            self.camera_array[i].SetVideoType(self.mode) # and GrayscaleMode work

            print("Starting Camera...")
            self.camera_array[i].Start()
            if self.mode != m.eVideoMode.GrayscaleMode:
                self.camera_array[i].SetExposure(self.exposure)
                self.camera_array[i].SetIntensity(5)
                self.camera_array[i].SetImagerGain(m.eImagerGain.Gain_Level7)
                #self.camera_array[i].SetFrameDecimation(1)
                print("Minimum Frame Rate:", self.camera_array[i].MinimumFrameRateValue())
                if self.framerate > 0:
                    self.camera_array[i].SetFrameRate(max(self.camera_array[i].MinimumFrameRateValue(), self.framerate))
                #print("Setting Framerate", self.camera_array[i].MinimumFrameRateValue(), "Current Decimation", self.camera_array[i].FrameDecimation(), "Current Grayscale Decimation", self.camera_array[i].GrayscaleDecimation())
                #self.camera_array[i].SetFrameRate(self.camera_array[i].MinimumFrameRateValue()//2)
                if self.delay_strobe:
                    self.camera_array[i].SetShutterDelay(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?
                    self.camera_array[i].SetStrobeOffset(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?
            time.sleep(0.05)

        self.camera_serials = np.array(self.camera_serials, dtype=np.int32)
        print("Camera Serials:", self.camera_serials)

        self.newFrame = False

    def fetch_frame(self):
        if self.mode == m.eVideoMode.GrayscaleMode:
            time.sleep(0.15) # This sleep appears to be necessary to allow the cameras to retrieve frames
            new_frame = m.GetSlowFrameArray(self.camera_serials)
        elif self.mode == m.eVideoMode.ObjectMode:
            new_frame = m.GetFrameGroupObjectArray(self.sync)
            new_frame = np.nan_to_num(new_frame, nan=0.0)
        else:
            current_framegroup = m.GetFrameGroup(self.sync)
            new_frame = np.zeros((current_framegroup.Count(), self.camera_array[0].Height(), self.camera_array[0].Width()), dtype=np.uint8)
            m.FillTensorFromFrameGroup(self.camera_array[0], current_framegroup, new_frame)
        self.current_frame = new_frame
        self.newFrame = True

    def run(self):
        print("Beginning Optitrack Receive Thread!")

        while(self.should_run):# and time.time() - self.deadmansSwitch < 6.0):
            self.fetch_frame()

        print("Should Run:", self.should_run, "Deadman Switch:", time.time() - self.deadmansSwitch)

        if self.sync:
            self.sync.RemoveAllCameras()
            m.cModuleSync.Destroy(self.sync)

        for i in range(len(self.camera_array)):
            if self.camera_array[i].State() == m.eCameraState.Initialized:
                print("Stopping Camera", i, "...")
                #camera_array[i].Stop(True)
                print("Stopped Camera", i)#" Releasing Camera...")
                #self.camera_array[i].Release()
                #print("Released Camera!")

        m.CameraManager.X().Shutdown()

        print("Exiting Optitrack Camera Thread!")
    def read(self):
        '''Retrieves the most recent frame from the system'''
        self.newFrame = False
        self.deadmansSwitch = time.time()
        return self.current_frame
    def stop(self):
        '''Ends the video receiving thread'''
        self.should_run = False
