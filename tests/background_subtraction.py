import cv2
import numpy as np

def smoothStep(x, edge0, edge1):
  t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)

thresh1 = 5
thresh2 = 20
background_two_frames = None

cap = cv2.VideoCapture('./tests/OptitrackOutput.mp4')

while(cap.isOpened() and not (cv2.waitKey(1) & 0xFF == ord('q'))):
    ret, frame = cap.read()
    if ret:

        # Background Model and Subtraction
        if(thresh2 > thresh1):
            # Initialize the background model
            if(background_two_frames is None):
                background_two_frames = np.full(frame.shape, 0, dtype=np.float)
                background_temp       = np.full(frame.shape, 0, dtype=np.float)

            background_temp[:,:,:] = np.where(background_two_frames < frame, 
                                              background_two_frames + (7.0 * 0.01111), # Increase 7 Brightness Values Per Second (at 90hz)
                                             (background_two_frames*0.15) + (frame.astype(np.float) * 0.85))

            # Set the original background model to be the new composite temp model
            background_two_frames[:,:,:] = background_temp[:,:,:]

            # Suppress pixels in the image that are close to the background model
            deviation = cv2.absdiff(frame.astype(np.float), background_two_frames)
            mask = np.where(deviation < thresh1, 0.0, 1.0)
            #mask = smoothStep(deviation, m("BG Threshold1"), m("BG Threshold2"))
            frame[:,:,:] = cv2.multiply(frame[:,:,:], mask, scale=1.0, dtype=0).astype(np.uint8)

        cv2.imshow("Frame", frame)
    else:
        break

cap.release()
cv2.destroyAllWindows()
