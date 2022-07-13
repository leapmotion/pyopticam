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
        self.current_frame = np.zeros((1, 1, 1))
        
        self.cameraManager = m.CameraManager.X()
        print("Waiting for Cameras to Initialize...")
        m.CameraManager.X().WaitForInitialization()

        print("Are Cameras Initialized?:", m.CameraManager.X().AreCamerasInitialized())

        print("Is CameraManager Active:", m.CameraManager.X().IsActive())
        
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
                self.camera_array[i] = m.CameraManager.X().GetCamera(self.camera_entries_array[i].UID())#BySerial(cameraEntry.Serial())
                print("Got Camera: ", i)

        self.mode = mode
        self.exposure = exposure
        self.delay_strobe = delay_strobe
        for i in range(len(self.camera_array)):
            #print("Camera", i, "Parameters: Name", self.camera_array[i].Name() , ", Framerate:", self.camera_array[i].FrameRate(), 
            #      ", Exposure:" , self.camera_array[i].Exposure(), ", Threshold:", self.camera_array[i].Threshold(), 
            #      ", Intensity:", self.camera_array[i].Intensity(),", CameraID:" , self.camera_array[i].CameraID())
            #self.camera_array[i].SetStatusRingRGB(0, 255, 0)

            print("Setting", self.mode)
            self.camera_array[i].SetVideoType(self.mode) # and GrayscaleMode work

            print("Starting Camera...")
            self.camera_array[i].Start()
            #self.camera_array[i].SetExposure(self.exposure)
            #self.camera_array[i].SetIntensity(5)
            #self.camera_array[i].SetImagerGain(m.eImagerGain.Gain_Level7)
            #if self.delay_strobe:
            #    self.camera_array[i].SetShutterDelay(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?
            #    self.camera_array[i].SetStrobeOffset(int(self.exposure * 1.2 * i)) # Keep the cameras from firing into eachother?

        self.sync = m.cModuleSync.Create()
        for i in range(len(self.camera_array)):
            self.sync.AddCamera(self.camera_array[i], 0)

        self.newFrame = False
        self.thread = None
        self.frame_id = 0
        #self.new_frame_id = 0

    def fetch_frame(self, garb, sync):
        #t = cv2.getTickCount()/cv2.getTickFrequency()
        current_framegroup = m.GetFrameGroup(sync)
        new_frame_id = current_framegroup.FrameID()
        if new_frame_id > self.frame_id:
            self.frame_id = new_frame_id
            if self.mode == m.eVideoMode.ObjectMode:
                new_frame = m.GetObjectArrayFromFrameGroup(current_framegroup)
                new_frame = np.nan_to_num(new_frame, nan=0.0)
            else:
                new_frame = m.GetTensorFromFrameGroup(current_framegroup)
            self.current_frame = new_frame
        #print("Read Frame costs:", ((cv2.getTickCount()/cv2.getTickFrequency()) - t)*1000, "ms")

    def run(self):
        print("Beginning Optitrack Receive Thread!")

        while(self.should_run and time.time() - self.deadmansSwitch < 4.0):
            # Have Numpy handle Allocating the array
            frame_group = m.GetFrameGroup(self.sync)
            self.current_frame = np.zeros((frame_group.Count(), 512, 640), dtype=np.uint8)
            m.FillTensorFromFrameGroup(frame_group, self.current_frame)
            self.newFrame = True

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
        self.newFrame = False
        self.deadmansSwitch = time.time()
        return self.current_frame
    def stop(self):
        '''Ends the video receiving thread'''
        self.should_run = False
        self.thread.join()
