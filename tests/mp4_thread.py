import numpy as np
import cv2
import threading
import time
import sys
import subprocess
import queue

class ffmpegThread(threading.Thread):
    '''A thread for encoding images to FFMPEG'''
    def __init__(self, out_file = 'output.mp4', width = 640, height=512*8, framerate=120, codec="libx264", crf=2, g=40, preset='fast', tune='fastdecode'):
        '''Initialize FFMPEG Encoding'''
        threading.Thread.__init__(self)
        self.out_file = out_file
        self.width  = int(width)
        self.height = int(height)
        self.framerate = framerate
        self.codec = codec
        self.crf = crf
        self.g = g
        self.preset = preset
        self.tune = tune

        self.image_queue = queue.Queue()
        self.num_frames_input = 0

        self.cmd_out = [
            'ffmpeg',
           '-loglevel', 'quiet', # tell ffmpeg to be more quieter
           '-y', # (optional) overwrite output file if it exists
           '-f', 'rawvideo',
           '-vcodec','rawvideo',
           '-s', '%dx%d'%(int(self.width), int(self.height)), # size of one frame
           '-pix_fmt', 'gray',
           '-r', str(self.framerate), # frames per second
           '-i', '-', # The input comes from a pipe
           '-an', # Tells FFMPEG not to expect any audio
           '-vcodec', self.codec,
           '-x264-params', 'keyint={}:scenecut=0'.format(self.g),
           '-crf', str(self.crf),
           '-g', str(self.g), # frames per keyframe
           '-pix_fmt', 'yuv420p',
           '-preset', preset,#"slow",
           '-tune',"fastdecode",
           #'-vf', str(args.vf)
           self.out_file
        ]

        self.should_run = True
        self.deadmansSwitch = time.time()
        self.t = 0

    def run(self):
        print("Beginning FFMPEG Encode Thread!")
        self.pipe = subprocess.Popen(self.cmd_out, stdin=subprocess.PIPE)
        self.fout = self.pipe.stdin

        while(self.should_run or not self.image_queue.empty()):# and time.time() - self.deadmansSwitch < 1.0): # and 
            time_ms = (time.perf_counter()-self.t)*(10 ** 3)
            self.t = time.perf_counter()
            #if self.pipe.stdout is not None:
            #    for line in self.pipe.stdout:
            #        print(line)

            if self.num_frames_input % 120 == 0:
                sys.stdout.flush()
                print("FFMPEG Interframe Time: "+ str(time_ms) + "ms", "Deadman's Switch", time.time() - self.deadmansSwitch)
                print("FFMPEG: Encoding Frame", self.num_frames_input, "with", self.image_queue.qsize(), "images in queue")

            if not self.image_queue.empty():
                image_frame = self.image_queue.get()
                if image_frame.shape[0] != self.height or image_frame.shape[1] != self.width:
                    image_frame = cv2.resize(image_frame, (int(self.width), int(self.height)))
                self.fout.write(image_frame.tostring())
                self.num_frames_input += 1

            time.sleep(0.000001)

        print("Closing FOUT...")
        self.fout.close()
        print("Waiting...")
        #if self.pipe.stdout is not None:
        #    for line in self.pipe.stdout:
        #        print(line)
        self.pipe.wait()
        #if self.pipe.stdout is not None:
        #    for line in self.pipe.stdout:
        #        print(line)
        print("Done Waiting!")
        if self.pipe.returncode !=0:
            raise subprocess.CalledProcessError(self.pipe.returncode, self.cmd_out)

        print("Exiting FFMPEG Video Encoding Thread!")
    def add_image(self, image):
        #image_frame = np.reshape(image, (self.height, self.width))
        self.image_queue.put(image)
        self.deadmansSwitch = time.time()
        #print("Switch is being reset!")
    def end_encoding(self):
        '''Ends the video encoding thread'''
        self.should_run = False
        sys.stdout.flush()
