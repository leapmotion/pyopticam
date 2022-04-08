import pyopticam as m
import numpy as np

def test_add():
    assert m.add(1, 2) == 3

print(m.add(1, 2))

print(m.inspect(np.array([1, 2, 3])))

print(m.ret_numpy())

cameraManager = m.CameraManager.X()
print(m.CameraManager.IsActive())
print(m.CameraManager.Ptr)
print(cameraManager.AreCamerasInitialized())

print(m.test_optitrack())

m.CameraManager.DestroyInstance()
