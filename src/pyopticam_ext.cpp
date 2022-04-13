#include <nanobind/nanobind.h>
#include <nanobind/tensor.h>
#include <cameralibrary.h>

using namespace CameraLibrary;

namespace nb = nanobind;

using namespace nb::literals;

NB_MODULE(pyopticam_ext, m) {
    //m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("add", [](int a, int b) { return a + b; }, "a"_a, "b"_a);

    m.def("inspect", [](nb::tensor<> tensor) {
        printf("Tensor data pointer : %p\n", tensor.data());
        printf("Tensor dimension : %zu\n", tensor.ndim());
        for (size_t i = 0; i < tensor.ndim(); ++i) {
            printf("Tensor dimension [%zu] : %zu\n", i, tensor.shape(i));
            printf("Tensor stride    [%zu] : %zd\n", i, tensor.stride(i));
        }
        printf("Device ID = %u (cpu=%i, cuda=%i)\n", tensor.device_id(),
            int(tensor.device_type() == nb::device::cpu::value),
            int(tensor.device_type() == nb::device::cuda::value)
        );
        printf("Tensor dtype: int16=%i, uint32=%i, float32=%i\n",
            tensor.dtype() == nb::dtype<int16_t>(),
            tensor.dtype() == nb::dtype<uint32_t>(),
            tensor.dtype() == nb::dtype<float>()
        );
    });

    m.def("process", [](nb::tensor<
                            uint8_t, nb::shape<nb::any, nb::any, 3>, 
                            nb::c_contig, nb::device::cpu> tensor) {
        // Double brightness of the MxNx3 RGB image
        for (size_t y = 0; y < tensor.shape(0); ++y){
            for (size_t x = 0; y < tensor.shape(1); ++x){
                for (size_t ch = 0; ch < 3; ++ch){
                    tensor(y, x, ch) = (uint8_t) std::min(255, tensor(y, x, ch) * 2);
                }
            }
        }
    });

    m.def("ret_numpy", []() {
        float *data = new float[8] { 1, 2, 3, 4, 
                                     5, 6, 7, 8 };
        size_t shape[2] = { 2, 4 };

        /// Delete 'data' when the 'deleter' capsule expires
        nb::capsule deleter(data, [](void *p) {
            delete[] (float *) p;
        });

        return nb::tensor<nb::numpy, float>(data, 2, shape, /* owner = */ deleter);
    });
    
    // Had to define this custom since I'm not sure
    // how reference return functions work in nanobind
    m.def("GetCameraList", [](CameraManager& manager) {
        CameraList cameras;
        //CameraManager::X()
        //manager.GetCameraList(cameras);
        return cameras;
    });//, nb::rv_policy::reference);

    //m.def("GetCameraFrame", [](int serial) {
    //    Camera* camera = CameraManager::X().GetCameraBySerial(serial);
    //    Frame* frame = camera->GetFrame();
    //    return frame;
    //});

    //m.def("GetCameraFrame", [](int serial) {
    //    Camera* camera = CameraManager::X().GetCameraBySerial(serial);
    //    Frame* frame = camera->GetFrame();

    //    printf("Optitrack Image Buffer Size : %p\n", frame->GetGrayscaleDataSize());
    //    printf("Optitrack Image Is Grayscale : %p\n", frame->IsGrayscale());
    //    uint8_t *data = frame->GetGrayscaleData();

    //    size_t shape[2] = { frame->Height(), frame->Width() };

    //    /// Delete 'data' when the 'deleter' capsule expires
    //    nb::capsule deleter(data, [](void *p) {
    //        delete[] (uint8_t *) p;
    //    });

    //    return nb::tensor<nb::numpy, uint8_t>(data, 2, shape, /* owner = */ deleter);
    //});

    m.def("GetCameraFrame", [](Frame* frame) {
        printf("[INFO] Optitrack Image Buffer Size : %i\n", frame->GetGrayscaleDataSize());
        printf("[INFO] Optitrack Image Is Grayscale : %i\n", frame->IsGrayscale());
        uint8_t *data = frame->GetGrayscaleData();
        printf("[INFO] Retrieved Data...");
        size_t shape[2] = { frame->Height(), frame->Width() };

        /// Delete 'data' when the 'deleter' capsule expires
        //nb::capsule deleter(data, [](void *p) {
        //    delete[] (uint8_t *) p;
        //});
        printf("[INFO] Defined Deleter");
        auto tensor = nb::tensor<nb::numpy, uint8_t>(data, 2, shape);//, /* owner = */ deleter);
        printf("[INFO] Defined Tensor, returning!");
        return tensor;
    });

    nb::enum_<Core::eVideoMode>(m, "eVideoMode")
        .value("SegmentMode"  , Core::eVideoMode::SegmentMode)
        .value("GrayscaleMode", Core::eVideoMode::GrayscaleMode)
        .value("ObjectMode", Core::eVideoMode::ObjectMode)
        .value("InterleavedGrayscaleMode", Core::eVideoMode::InterleavedGrayscaleMode)
        .value("PrecisionMode", Core::eVideoMode::PrecisionMode)
        .value("BitPackedPrecisionMode", Core::eVideoMode::BitPackedPrecisionMode)
        .value("MJPEGMode", Core::eVideoMode::MJPEGMode)
        .value("VideoMode", Core::eVideoMode::VideoMode)
        .value("SynchronizationTelemetry", Core::eVideoMode::SynchronizationTelemetry)
        .value("VideoModeCount", Core::eVideoMode::VideoModeCount)
        .value("UnknownMode", Core::eVideoMode::UnknownMode);
        //.export_values();

    nb::enum_<CameraLibrary::eCameraState>(m, "eCameraState")
        .value("Uninitialized"  , CameraLibrary::eCameraState::Uninitialized)
        .value("InitializingDevice", CameraLibrary::eCameraState::InitializingDevice)
        .value("InitializingCamera", CameraLibrary::eCameraState::InitializingCamera)
        .value("Initializing", CameraLibrary::eCameraState::Initializing)
        .value("WaitingForChildDevices", CameraLibrary::eCameraState::WaitingForChildDevices)
        .value("WaitingForDeviceInitialization", CameraLibrary::eCameraState::WaitingForDeviceInitialization)
        .value("Initialized", CameraLibrary::eCameraState::Initialized)
        .value("Disconnected", CameraLibrary::eCameraState::Disconnected)
        .value("Shutdown", CameraLibrary::eCameraState::Shutdown);

    nb::class_<sStatusLightColor>(m, "sStatusLightColor")
        .def(nb::init())
        .def_readwrite("Red", &sStatusLightColor::Red)
        .def_readwrite("Green", &sStatusLightColor::Green)
        .def_readwrite("Blue", &sStatusLightColor::Blue);

    nb::class_<cCameraLibraryStartupSettings>(m, "cCameraLibraryStartupSettings")
        .def(nb::init())
        .def("X", cCameraLibraryStartupSettings::X)
        .def("EnableDevelopment", &cCameraLibraryStartupSettings::EnableDevelopment)
        .def("IsDevelopmentEnabled", &cCameraLibraryStartupSettings::IsDevelopmentEnabled);

    nb::class_<Frame>(m, "Frame")
        .def(nb::init())
        .def("ObjectCount", &Frame::ObjectCount)
        .def("FrameID", &Frame::FrameID)
        .def("FrameType", &Frame::FrameType)
        .def("MJPEGQuality", &Frame::MJPEGQuality)
        .def("Object", &Frame::Object)
        .def("GetLink", &Frame::GetLink)
        .def("GetCamera", &Frame::GetCamera)
        .def("IsInvalid", &Frame::IsInvalid)
        .def("IsEmpty", &Frame::IsEmpty)
        .def("IsGrayscale", &Frame::IsGrayscale)
        .def("Width", &Frame::Width)
        .def("Height", &Frame::Height)
        .def("Left", &Frame::Left)
        .def("Top", &Frame::Top)
        .def("Right", &Frame::Right)
        .def("Bottom", &Frame::Bottom)
        .def("Scale", &Frame::Scale)
        .def("TimeStamp", &Frame::TimeStamp)
        .def("IsSynchInfoValid", &Frame::IsSynchInfoValid)
        .def("IsTimeCodeValid", &Frame::IsTimeCodeValid)
        .def("IsExternalLocked", &Frame::IsExternalLocked)
        .def("IsRecording", &Frame::IsRecording)
        .def("TimeCode", &Frame::TimeCode)
        .def("IMUTelemetryCount", &Frame::IMUTelemetryCount)
        .def("IMUTelemetry", &Frame::IMUTelemetry)
        .def("HardwareTimeStampValue", &Frame::HardwareTimeStampValue)
        .def("HardwareTimeStamp", &Frame::HardwareTimeStamp)
        .def("IsHardwareTimeStamp", &Frame::IsHardwareTimeStamp)
        .def("HardwareTimeFreq", &Frame::HardwareTimeFreq)
        .def("MasterTimingDevice", &Frame::MasterTimingDevice)
        .def("Release", &Frame::Release)
        .def("RefCount", &Frame::RefCount)
        .def("AddRef", &Frame::AddRef)
        //.def("Rasterize", &Frame::Rasterize) // Overloads!
        //.def("Rasterize", &Frame::Rasterize)
        .def("CompressedImageSize", &Frame::CompressedImageSize)
        .def("GetGrayscaleData", &Frame::GetGrayscaleData)
        .def("GetGrayscaleDataSize", &Frame::GetGrayscaleDataSize)
        .def("SetObjectCount", &Frame::SetObjectCount)
        .def("RemoveObject", &Frame::RemoveObject)
        //.def("HardwareRecording", &Frame::HardwareRecording)
        ;

    nb::class_<Camera>(m, "Camera")
        .def(nb::init())
        //.def("Width", &Camera::Width)
        //.def("Height", &Camera::Height)
        .def("GetFrame", &Camera::GetFrame)
        .def("Name", &Camera::Name)
        .def("Start", &Camera::Start)
        .def("Stop", &Camera::Stop)
        .def("IsCameraRunning", &Camera::IsCameraRunning)
        .def("Release", &Camera::Release) // Virtual
        .def("SetNumeric", &Camera::SetNumeric)
        .def("SetExposure", &Camera::SetExposure)
        .def("SetThreshold", &Camera::SetThreshold)
        .def("SetIntensity", &Camera::SetIntensity) // Virtual
        .def("SetPrecisionCap", &Camera::SetPrecisionCap)
        .def("SetShutterDelay", &Camera::SetShutterDelay) // Virtual
        .def("SetStrobeOffset", &Camera::SetStrobeOffset) // Virtual
        .def("SetFrameRate", &Camera::SetFrameRate) // Virtual
        .def("FrameRate", &Camera::FrameRate) // Virtual
        .def("SetFrameDecimation", &Camera::SetFrameDecimation) // Virtual
        .def("FrameDecimation", &Camera::FrameDecimation) // Virtual
        .def("GrayscaleDecimation", &Camera::GrayscaleDecimation)
        .def("PrecisionCap", &Camera::PrecisionCap)
        .def("ShutterDelay", &Camera::ShutterDelay) // Virtual
        .def("StrobeOffset", &Camera::StrobeOffset) // Virtual
        .def("Exposure", &Camera::Exposure)
        .def("Threshold", &Camera::Threshold)
        .def("Intensity", &Camera::Intensity) // Virtual
        .def("SetVideoType", &Camera::SetVideoType) // Virtual // Uses Enum
        .def("IsVideoTypeSupported", &Camera::IsVideoTypeSupported) // Virtual // Uses Enum
        .def("IsVideoTypeSynchronous", &Camera::IsVideoTypeSynchronous) // Virtual // Uses Enum
        .def("DataRate", &Camera::DataRate)
        .def("PacketSize", &Camera::PacketSize)
        .def("SetGrayscaleDecimation", &Camera::SetGrayscaleDecimation)
        .def("SendEmptyFrames", &Camera::SendEmptyFrames)
        .def("SendInvalidFrames", &Camera::SendInvalidFrames)
        .def("SetLateDecompression", &Camera::SetLateDecompression)
        .def("LateDecompression", &Camera::LateDecompression)
        .def("Serial", &Camera::Serial)
        .def("SerialString", &Camera::SerialString)
        .def("Model", &Camera::Model)
        .def("SubModel", &Camera::SubModel)
        .def("Revision", &Camera::Revision)
        .def("HardwareInterface", &Camera::HardwareInterface)
        .def("CameraID", &Camera::CameraID) // Virtual
        .def("CameraIDValid", &Camera::CameraIDValid) // Virtual
        .def("SetIRFilter", &Camera::SetIRFilter)
        .def("IRFilter", &Camera::IRFilter) // Virtual
        .def("IsFilterSwitchAvailable", &Camera::IsFilterSwitchAvailable) // Virtual
        .def("SetAGC", &Camera::SetAGC)
        .def("AGC", &Camera::AGC)
        .def("IsAGCAvailable", &Camera::IsAGCAvailable) // Virtual
        .def("SetAEC", &Camera::SetAEC)
        .def("AEC", &Camera::AEC)
        .def("IsAECAvailable", &Camera::IsAECAvailable) // Virtual
        .def("SetImagerGain", &Camera::SetImagerGain) // Uses Enum
        .def("ImagerGain", &Camera::ImagerGain) // Uses Enum
        .def("IsImagerGainAvailable", &Camera::IsImagerGainAvailable) // Virtual Uses Enum
        .def("ImagerGainLevels", &Camera::ImagerGainLevels) // Virtual Uses Enum
        .def("SetHighPowerMode", &Camera::SetHighPowerMode) // Virtual
        .def("HighPowerMode", &Camera::HighPowerMode) // Virtual
        .def("IsHighPowerModeAvailable", &Camera::IsHighPowerModeAvailable) // Virtual
        .def("IsHighPowerModeSupported", &Camera::IsHighPowerModeSupported) // Virtual
        .def("LowPowerSetting", &Camera::LowPowerSetting)
        .def("ActualFrameRate", &Camera::ActualFrameRate) // Virtual
        .def("SetMJPEGQuality", &Camera::SetMJPEGQuality)
        .def("MJPEGQuality", &Camera::MJPEGQuality) // Virtual
        .def("IsMJPEGAvailable", &Camera::IsMJPEGAvailable) // Virtual
        .def("IsContinuousIRAvailable", &Camera::IsContinuousIRAvailable) // Virtual
        .def("SetContinuousIR", &Camera::SetContinuousIR) // Virtual
        .def("ContinuousIR", &Camera::ContinuousIR) // Virtual
        .def("IsQuietModeAvailable", &Camera::IsQuietModeAvailable) // Virtual
        .def("SetQuietMode", &Camera::SetQuietMode) // Virtual
        .def("QuietMode", &Camera::QuietMode)
        .def("SetRinglightEnabledWhileStopped", &Camera::SetRinglightEnabledWhileStopped) // Virtual
        .def("RinglightEnabledWhileStopped", &Camera::RinglightEnabledWhileStopped) // Virtual
        .def("IsHardwareFiltered", &Camera::IsHardwareFiltered) // Virtual, Uses Enum
        .def("SwitchState", &Camera::SwitchState)
        .def("Health", &Camera::Health)
        .def("GetDistortionModel", &Camera::GetDistortionModel) // Virtual
        .def("ResetWindow", &Camera::ResetWindow)
        .def("SetWindow", &Camera::SetWindow) // Virtual
        .def("IsWindowingSupported", &Camera::IsWindowingSupported) // Virtual
        .def("CalcWindow", &Camera::CalcWindow) // Virtual
        .def("SetLED", &Camera::SetLED) // Uses Enum
        .def("SetAllLED", &Camera::SetAllLED) // Uses Enum
        .def("SetStatusIntensity", &Camera::SetStatusIntensity)
        .def("StatusRingLightCount", &Camera::StatusRingLightCount) // Virtual
        .def("SetStatusRingLights", &Camera::SetStatusRingLights) // Virtual
        .def("SetStatusRingRGB", &Camera::SetStatusRingRGB) // Virtual
        .def("IsIRIlluminationAvailable", &Camera::IsIRIlluminationAvailable) // Virtual
        .def("SetEnableBlockingMask", &Camera::SetEnableBlockingMask)
        .def("IsBlockingMaskEnabled", &Camera::IsBlockingMaskEnabled)
        .def("AddBlockingRectangle", &Camera::AddBlockingRectangle)
        .def("RemoveBlockingRectangle", &Camera::RemoveBlockingRectangle)
        .def("SetBitMaskPixel", &Camera::SetBitMaskPixel)
        .def("ClearBlockingMask", &Camera::ClearBlockingMask)
        //.def("GetBlockingMask", &Camera::GetBlockingMask)
        //.def("SetBlockingMask", &Camera::SetBlockingMask)
        .def("UpdateBlockingMask", &Camera::UpdateBlockingMask) // Virtual
        .def("BlockingMaskWidth", &Camera::BlockingMaskWidth)
        .def("BlockingMaskHeight", &Camera::BlockingMaskHeight)
        .def("BlockingGrid", &Camera::BlockingGrid)
        .def("ImagerWidth", &Camera::ImagerWidth) // Virtual
        .def("ImagerHeight", &Camera::ImagerHeight) // Virtual
        .def("FocalLength", &Camera::FocalLength) // Virtual
        .def("HardwareFrameRate", &Camera::HardwareFrameRate)
        .def("PhysicalPixelWidth", &Camera::PhysicalPixelWidth)
        .def("PhysicalPixelHeight", &Camera::PhysicalPixelHeight)
        .def("SetTextOverlay", &Camera::SetTextOverlay)

        .def("SetMarkerOverlay", &Camera::SetMarkerOverlay)
        .def("TextOverlay", &Camera::TextOverlay)
        .def("MarkerOverlay", &Camera::MarkerOverlay)
        //.def("SetName", &Camera::SetName)
        .def("IsInitilized", &Camera::IsInitilized)
        .def("IsDisconnected", &Camera::IsDisconnected)
        .def("State", &Camera::State)
        .def("UID", &Camera::UID)
        .def("ConnectionType", &Camera::ConnectionType)
        .def("IsVirtual", &Camera::IsVirtual) // Virtual
        //.def("AttachInput", &Camera::AttachInput) // Virtual
        //.def("DetachInput", &Camera::DetachInput)
        //.def("TransferInput", &Camera::TransferInput) // Virtual
        .def("IsCommandQueueEmpty", &Camera::IsCommandQueueEmpty)
        .def("ReleaseFrame", &Camera::ReleaseFrame)
        //.def("DevicePath", &Camera::DevicePath)
        .def("SendCommand", &Camera::SendCommand) // Virtual
        .def("AttachModule", &Camera::AttachModule)
        .def("RemoveModule", &Camera::RemoveModule)
        .def("ModuleCount", &Camera::ModuleCount)
        .def("AttachListener", &Camera::AttachListener)
        .def("RemoveListener", &Camera::RemoveListener)
        .def("Shutdown", &Camera::Shutdown)
        .def("IsCamera", &Camera::IsCamera) // Virtual
        .def("IsHardwareKey", &Camera::IsHardwareKey) // Virtual
        .def("IsHub", &Camera::IsHub) // Virtual
        .def("IsUSB", &Camera::IsUSB) // Virtual
        .def("IsEthernet", &Camera::IsEthernet) // Virtual
        .def("IsTBar", &Camera::IsTBar) // Virtual
        .def("IsSyncAuthority", &Camera::IsSyncAuthority) // Virtual
        .def("IsBaseStation", &Camera::IsBaseStation) // Virtual
        .def("SyncFeatures", &Camera::SyncFeatures) // Virtual
        .def("SetObjectColor", &Camera::SetObjectColor)
        .def("ObjectColor", &Camera::ObjectColor)
        .def("SetGrayscaleFloor", &Camera::SetGrayscaleFloor) // Virtual
        //.def("FrameSize", &Camera::FrameSize) // Virtual

        .def("SetEnablePayload", &Camera::SetEnablePayload) // Virtual
        .def("IsEnablePayload", &Camera::IsEnablePayload) // Virtual
        .def("IsCameraTempValid", &Camera::IsCameraTempValid) // Virtual
        .def("CameraTemp", &Camera::CameraTemp) // Virtual
        .def("IsRinglightTempValid", &Camera::IsRinglightTempValid) // Virtual
        .def("RinglightTemp", &Camera::RinglightTemp) // Virtual
        .def("IsCameraFanSpeedValid", &Camera::IsCameraFanSpeedValid) // Virtual
        .def("CameraFanSpeed", &Camera::CameraFanSpeed) // Virtual
        .def("IsPoEPlusActive", &Camera::IsPoEPlusActive) // Virtual
        .def("SetLLDPDetection", &Camera::SetLLDPDetection) // Uses Enum
        .def("IsLLDPDetectionAvaiable", &Camera::IsLLDPDetectionAvaiable) // Virtual  // Uses Enum
        .def("LLDPDetection", &Camera::LLDPDetection) // Uses Enum
        .def("MinimumExposureValue", &Camera::MinimumExposureValue) // Virtual
        .def("MaximumExposureValue", &Camera::MaximumExposureValue) // Virtual
        .def("MinimumFrameRateValue", &Camera::MinimumFrameRateValue) // Virtual
        .def("MaximumFrameRateValue", &Camera::MaximumFrameRateValue) // Virtual
        .def("MaximumFullImageFrameRateValue", &Camera::MaximumFullImageFrameRateValue) // Virtual
        .def("MinimumThreshold", &Camera::MinimumThreshold) // Virtual
        .def("MaximumThreshold", &Camera::MaximumThreshold) // Virtual
        .def("MinimumIntensity", &Camera::MinimumIntensity) // Virtual
        .def("MaximumIntensity", &Camera::MaximumIntensity) // Virtual
        .def("MaximumMJPEGRateValue", &Camera::MaximumMJPEGRateValue) // Virtual
        //.def("SetParameter", &Camera::SetParameter)// Overloads!
        //.def("SetParameter", &Camera::SetParameter) // Virtual
        .def("StorageMaxSize", &Camera::StorageMaxSize) // Virtual
        //.def("LoadFile", &Camera::LoadFile) // Virtual, Overloads
        //.def("SaveFile", &Camera::SaveFile) // Virtual
        .def("OptiHubConnectivity", &Camera::OptiHubConnectivity) // Virtual
        .def("IsColor", &Camera::IsColor) // Virtual
        .def("SetColorMatrix", &Camera::SetColorMatrix) // Virtual
        .def("SetColorGamma", &Camera::SetColorGamma) // Virtual
        .def("SetColorPrescalar", &Camera::SetColorPrescalar) // Virtual
        .def("SetColorCompression", &Camera::SetColorCompression) // Virtual
        .def("ColorMatrix", &Camera::ColorMatrix) // Virtual
        .def("ColorGamma", &Camera::ColorGamma) // Virtual
        .def("ColorPrescalar", &Camera::ColorPrescalar) // Virtual
        .def("ColorMode", &Camera::ColorMode) // Virtual
        .def("ColorCompression", &Camera::ColorCompression) // Virtual
        .def("ColorBitRate", &Camera::ColorBitRate) // Virtual
        .def("CameraResolutionCount", &Camera::CameraResolutionCount) // Virtual
        .def("CameraResolutionID", &Camera::CameraResolutionID) // Virtual
        .def("CameraResolution", &Camera::CameraResolution) // Virtual
        .def("SetCameraResolution", &Camera::SetCameraResolution) // Virtual
        .def("QueryHardwareTimeStampValue", &Camera::QueryHardwareTimeStampValue) // Virtual
        .def("IsHardwareTimeStampValueSupported", &Camera::IsHardwareTimeStampValueSupported) // Virtual
        .def("SetColorEnhancement", &Camera::SetColorEnhancement) // Virtual
        .def("ColorEnhancement", &Camera::ColorEnhancement) // Virtual
        .def("SetPixelIntensityMapping", &Camera::SetPixelIntensityMapping) // Virtual
        ;

    nb::class_<CameraManager>(m, "CameraManager")
        //.def(nb::init())
        .def_static("X", CameraManager::X, nb::rv_policy::reference)
        .def("WaitForInitialization", &CameraManager::WaitForInitialization)
        .def("AreCamerasInitialized", &CameraManager::AreCamerasInitialized)
        .def("AreCamerasShutdown", &CameraManager::AreCamerasShutdown)
        .def("Shutdown", &CameraManager::Shutdown)
        .def("GetCameraBySerial", &CameraManager::GetCameraBySerial)
        .def("GetCamera", nb::overload_cast<const Core::cUID&>(&CameraManager::GetCamera)) // Overloads!
        .def("GetCamera", nb::overload_cast<>(&CameraManager::GetCamera)) // Overloads! , nb::rv_policy::reference
        //.def("GetCamera", &CameraManager::GetCamera)
        .def("GetCameraList", &CameraManager::GetCameraList)
        .def("GetHardwareKey", &CameraManager::GetHardwareKey)
        .def("GetDevice", &CameraManager::GetDevice)
        .def("PrepareForSuspend", &CameraManager::PrepareForSuspend)
        .def("ResumeFromSuspend", &CameraManager::ResumeFromSuspend)
        .def("TimeStamp", &CameraManager::TimeStamp)
        .def("TimeStampFrequency", &CameraManager::TimeStampFrequency)
        .def("ResetTimeStamp", &CameraManager::ResetTimeStamp)
        .def("RegisterListener", &CameraManager::RegisterListener)
        .def("UnregisterListener", &CameraManager::UnregisterListener)
        .def_static("CameraFactory", CameraManager::CameraFactory)
        .def("AddCamera", &CameraManager::AddCamera)
        .def("RemoveCamera", &CameraManager::RemoveCamera)
        .def("RemoveVirtualCameras", &CameraManager::RemoveVirtualCameras)
        .def("ScanForCameras", &CameraManager::ScanForCameras)
        .def("ApplySyncSettings", &CameraManager::ApplySyncSettings)
        .def("GetSyncSettings", &CameraManager::GetSyncSettings)
        .def("SyncSettings", &CameraManager::SyncSettings)
        .def("SoftwareTrigger", &CameraManager::SoftwareTrigger)
        .def("SyncMode", &CameraManager::SyncMode)
        .def("UpdateRecordingBit", &CameraManager::UpdateRecordingBit)
        .def("GetSyncFeatures", &CameraManager::GetSyncFeatures)
        //.def("SyncDeviceName", &CameraManager::SyncDeviceName)
        .def("ShouldLockCameraExposures", &CameraManager::ShouldLockCameraExposures)
        .def("ShouldForceCameraRateControls", &CameraManager::ShouldForceCameraRateControls)
        .def("ShouldApplySyncOnExposureChange", &CameraManager::ShouldApplySyncOnExposureChange)
        .def("SuggestCameraIDOrder", &CameraManager::SuggestCameraIDOrder)
        .def_static("DestroyInstance", CameraManager::DestroyInstance)
        .def_static("IsActive", CameraManager::IsActive)
        .def_static("Ptr", CameraManager::Ptr)
        ;

    nb::class_<CameraEntry>(m, "CameraEntry")
        //.def(nb::init())
        .def("UID", &CameraEntry::UID)
        .def("Serial", &CameraEntry::Serial)
        .def("Revision", &CameraEntry::Revision)
        .def("Name", &CameraEntry::Name)
        .def("State", &CameraEntry::State)
        .def("IsVirtual", &CameraEntry::IsVirtual)
        .def("SerialString", &CameraEntry::SerialString)
        ;

    nb::class_<CameraList>(m, "CameraList")
        .def(nb::init())
        .def("get", &CameraList::operator[])
        .def("Count", &CameraList::Count)
        .def("Refresh", &CameraList::Refresh)
        ;

    nb::class_<HardwareKeyList>(m, "HardwareKeyList")
        .def(nb::init())
        .def("get", &HardwareKeyList::operator[])
        .def("Count", &HardwareKeyList::Count)
        ;

    nb::class_<HubList>(m, "HubList")
        .def(nb::init())
        .def("get", &HubList::operator[])
        .def("Count", &HubList::Count)
        ;

    nb::class_<HardwareDeviceList>(m, "HardwareDeviceList")
        .def(nb::init())
        .def("get", &HardwareDeviceList::operator[])
        .def("Count", &HardwareDeviceList::Count)
        ;

    nb::class_<CameraManagerListener>(m, "CameraManagerListener")
        //.def(nb::init())
        .def("CameraConnected", &CameraManagerListener::CameraConnected) // Virtual
        .def("CameraRemoved", &CameraManagerListener::CameraRemoved) // Virtual
        .def("SyncSettingsChanged", &CameraManagerListener::SyncSettingsChanged) // Virtual
        .def("CameraInitialized", &CameraManagerListener::CameraInitialized) // Virtual
        .def("SyncAuthorityInitialized", &CameraManagerListener::SyncAuthorityInitialized) // Virtual
        .def("SyncAuthorityRemoved", &CameraManagerListener::SyncAuthorityRemoved) // Virtual
        .def("CameraMessage", &CameraManagerListener::CameraMessage) // Virtual
        .def("RequestUnknownDeviceImplementation", &CameraManagerListener::RequestUnknownDeviceImplementation) // Virtual
        //.def("ShouldConnectCamera", &CameraManagerListener::ShouldConnectCamera) // Virtual
        ;

    nb::class_<Core::cUID>(m, "cUID")
        .def(nb::init())
        //.def("from_string", &Core::cUID::from_string)
        .def("SetValue", &Core::cUID::SetValue)
        .def("LowBits", &Core::cUID::LowBits)
        .def("HighBits", &Core::cUID::HighBits)
        .def("Valid", &Core::cUID::Valid)
        .def("Generate", Core::cUID::Generate)
        .def("isLessThan", &Core::cUID::operator<)
        .def("isLessThanOrEqual", &Core::cUID::operator<=)
        .def("isGreaterThan", &Core::cUID::operator>)
        .def("isGreaterThanOrEqual", &Core::cUID::operator>=)
        .def("isEqualTo", &Core::cUID::operator==)
        .def("isNotEqualTo", &Core::cUID::operator!=)
        //.def_readonly_static("Invalid", &Core::cUID::kInvalid)
        ;
}
