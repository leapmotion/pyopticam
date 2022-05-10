import cv2
import numpy as np

def smoothStep(x, edge0, edge1):
  t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
  return t * t * (3.0 - 2.0 * t)

thresh1 = 30
thresh2 = 40
background_two_frames = None

cap = cv2.VideoCapture('./OptitrackOutput.mp4')

while(cap.isOpened() and not (cv2.waitKey(1) & 0xFF == ord('q'))):
    ret, frame = cap.read()
    if ret:
        frame = cv2.resize(frame, (int(frame.shape[1]/2), int(frame.shape[0]/2)))
        cv2.imshow("Raw Frame", frame)

        # Background Model and Subtraction
        if(thresh2 > thresh1):
            # Initialize the background model
            if(background_two_frames is None):
                background_two_frames = np.copy(frame.astype(np.float))

            # Suppress pixels in the image that are close to the background model
            deviation = cv2.absdiff(frame.astype(np.float), background_two_frames)
            mask = np.where(deviation < thresh1, 0, 1).astype(np.uint8)
            mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, np.ones((3,3),np.uint8))
            #mask = smoothStep(deviation, thresh1, thresh2)
            frame[:,:,:] = cv2.multiply(frame[:,:,:], mask, scale=1.0, dtype=0).astype(np.uint8)

        cv2.imshow("Subtracted Frame", frame)
    else:
        break

cap.release()
cv2.destroyAllWindows()
