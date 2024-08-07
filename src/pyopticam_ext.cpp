#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/list.h>
#include <cameralibrary.h>

#ifdef _WIN32
    #include <Windows.h>
#endif
#include <iostream>
#include <cstdlib>

using namespace CameraLibrary;

namespace nb = nanobind;

using namespace nb::literals;

NB_MODULE(pyopticam_ext, m) {
    //m.doc() = "pybind11 example plugin"; // optional module docstring
    
    m.def("GetCameraList", [](CameraManager& manager) {
        CameraList cameras;
        //CameraManager::X()
        //manager.GetCameraList(cameras);
        return cameras;
    });//, nb::rv_policy::reference);

    m.def("GetFrameGroupObjectArray", [](cModuleSync* sync) {
        nanobind::gil_scoped_release release;

        // Layout is X, Y, Radius
        float *tracked_object_data = new float[8 * 255 * 3];// { 1, 2, 3, 4, 5, 6, 7, 8 };
        size_t tracked_object_shape[3] = { 8, 255, 3 };
        nb::capsule tracked_object_deleter(tracked_object_data, [](void *p) noexcept { delete[] (float *) p; });
        nb::ndarray<nb::numpy, float> ndarray = nb::ndarray<nb::numpy, float>(tracked_object_data, 3, tracked_object_shape, /* owner = */ tracked_object_deleter);
 
        // Zero out the marker memory
        for(int i = 0; i < 8 * 255 * 3; i++){ tracked_object_data[i] = 0.0f; }

        //printf("[INFO] About to get FrameGroup!\n");
        std::shared_ptr<const FrameGroup> frameGroup = sync->GetFrameGroup();
        //printf("[INFO] Retrieved FrameGroup!\n");
        bool invalid_frame_group = frameGroup == nullptr || frameGroup->Count() == 0;
        while(invalid_frame_group){
            //printf("[INFO] Bad Framegroup; Sleeping...\n");
            Sleep(1);
            frameGroup = sync -> GetFrameGroup();
            invalid_frame_group = frameGroup == nullptr || frameGroup->Count() == 0;
 
            //if(invalid_frame_group){
            //    if(frameGroup == nullptr){
            //        printf("[INFO] FrameGroup is Null!\n");
            //    } else {
            //        printf("[INFO] FrameGroup has %i frames\n", frameGroup->Count());
            //    }
            //}
        }
 
        if(frameGroup != nullptr && frameGroup->Count() > 0 ){
            int count = frameGroup->Count();
 
            for(int i = 0; i < count; i++){
                //printf("[INFO] About to read SubFrame %i\n", i);
                std::shared_ptr<const Frame> frame = frameGroup->GetFrame(i);
                if(!(frame->IsInvalid())){
                    //printf("[INFO] Getting Subframe Size\n");
                    int objCount = frame->ObjectCount();
                    //printf("[INFO] Num objects are: %i\n", count);
 
                    for(int j = 0; j < objCount; j++){
                        tracked_object_data[(i * 255 * 3) + (j * 3) + 0] = frame->Object(j)->X();
                        tracked_object_data[(i * 255 * 3) + (j * 3) + 1] = frame->Object(j)->Y();
                        tracked_object_data[(i * 255 * 3) + (j * 3) + 2] = frame->Object(j)->Radius();
                    }
                } else {
                    //printf("[WARNING] Subframe was Empty or Invalid! From camera: %i\n", frame->GetCamera()->Serial());
                }
            }
        }else{
            printf("[WARNING] Framegroup is a nullptr or has an invalid number of cameras!\n");
        }
        nanobind::gil_scoped_acquire acquire;
        return ndarray;
    });

    m.def("GetSlowFrameArray", [](nb::ndarray<int32_t, nb::ndim<2>, nb::c_contig, nb::device::cpu> serials){//nb::list<int> serials){
        nanobind::gil_scoped_release release;
        //printf("[INFO] Creating Dummy Data!\n");
        uint8_t *stand_in_data = new uint8_t[8] { 1, 2, 3, 4, 5, 6, 7, 8 };
        size_t stand_in_shape[3] = { 8, 1, 1 };
        nb::capsule stand_in_deleter(stand_in_data, [](void *p) noexcept { delete[] (uint8_t *) p; });
        nb::ndarray<nb::numpy, uint8_t> ndarray = nb::ndarray<nb::numpy, uint8_t>(stand_in_data, 3, stand_in_shape, /* owner = */ stand_in_deleter);

        // FrameGroups are null in GrayscaleMode!, Read Frames Individually the Stupid Way!
        size_t offset = 0;
        auto view = serials.view();
        int count = view.shape(0);
        //printf("[INFO] Count is %i\n", count);
        uint8_t* full_buffer = nullptr;
        int height = 0, width = 0;
        size_t last_address = 0;

        //printf("[INFO] About to get Camera Manager\n");
        CameraLibrary::CameraManager* cameraManager = &CameraManager::X();

        for(int i = 0; i < count; i++){
            //printf("[INFO] About to get Camera %i with serial %i\n", i, serials(i));
            std::shared_ptr <Camera> camera = cameraManager->GetCameraBySerial(view(i, 0));
            //printf("[INFO] About to Check if camera with serial %i is running...\n", serials(i));
            if (!camera->IsCameraRunning()) { printf("[WARNING] CAMERA IS NOT RUNNING!\n");  camera->Start(); }
            std::shared_ptr<const Frame> frame; bool success = false;
            while(!success){
                try {
                    //printf("[INFO] About to read SubFrame %i with serial %i\n", i, serials(i));
                    frame = camera->LatestFrame();//camera->GetFrame();//
                    success = true;
                } catch (...) { 
                    // This never gets triggered; it just silently crashes...
                    printf("[WARNING] Image Read Failed, trying again...\n");
                    Sleep(1);
                }
            }
            if(!(frame->IsInvalid())){
                //printf("[INFO] Getting Subframe Size\n");
                int size = frame->GrayscaleDataSize();
                //printf("[INFO] SubFrame Size is: %i\n", size);

                if(offset == 0) {
                    stand_in_deleter.release();
                    height = frame->Height(); width = frame->Width();

                    while(full_buffer == nullptr){
                        //printf("[INFO] About to alloc full_buffer: BufferSize = %i, Count = %i, Offset = %i, Size = %i, Width = %i, Height = %i\n", size * (count+1), count, offset, size, width, height);
                        full_buffer = (uint8_t*) malloc(size * (count+1));
                        if(full_buffer == nullptr){
                            printf("[ERROR] Failed to allocate memory for full buffer!  Trying again...\n");
                            Sleep(2);
                        }
                    }
                    size_t shape[3] = { count, height, width };
                    nb::capsule deleter(full_buffer, [](void *p) noexcept { delete[] (uint8_t *) p; }); /// Delete 'full_buffer' when the 'deleter' capsule expires
                    ndarray = nb::ndarray<nb::numpy, uint8_t>(full_buffer, 3, shape, deleter);
                }
                if(size > width * height){
                    printf("[WARNING] Couldn't MemCpy; Count = %i, Offset = %i, Size = %i, Width = %i, Height = %i\n", count, offset, size, width, height);
                    break;
                }else{
                    const uint8_t* data = frame->GrayscaleData(*camera);
                    // Copy the frame from the Optitrack SDK to our contiguous Numpy-Managed Buffer
                    //printf("[INFO] Starting MemCpy at address: %zu, offset forward by %zu\n", (size_t)data, (size_t)data - last_address);
                    last_address = (size_t)data;
                    memcpy(full_buffer + offset, data, size);
                    offset += size;
                }
            } else {
                printf("[WARNING] Subframe was Empty or Invalid! From camera: %i\n", camera->Serial());
            }
        }
        //frameGroup->Release();
        if(offset == 0) { printf("[WARNING] No full or valid frames were found in the FrameGroup!  Returning Default ndarray...\n"); }

        nanobind::gil_scoped_acquire acquire;

        return ndarray;
    });

    
    m.def("GetFrameGroup", [](cModuleSync* sync) {
        nanobind::gil_scoped_release release;
        //FrameGroup* frameGroup = sync -> GetFrameGroup(); //GetFrameGroupSharedPointer();//
        std::shared_ptr<const FrameGroup> frameGroup = sync->GetFrameGroup();
        bool invalid_frame_group = !frameGroup || frameGroup == nullptr || frameGroup->Count() == 0;
        while(invalid_frame_group){
            //printf("[INFO] Bad Framegroup; Sleeping...\n");
            //Sleep(8);
            //frameGroup = sync -> GetFrameGroup();
            frameGroup = sync->GetFrameGroup();
            invalid_frame_group = !frameGroup || frameGroup == nullptr || frameGroup->Count() == 0;

            //if(invalid_frame_group){
            //    if(frameGroup == nullptr){
            //        printf("[INFO] FrameGroup is Null!\n");
            //    } else {
            //        printf("[INFO] FrameGroup has %i frames\n", frameGroup->Count());
            //    }
            //}
        }
        nanobind::gil_scoped_acquire acquire;
        return frameGroup;
    });

    
    m.def("FillTensorFromFrameGroup", [](std::shared_ptr<Camera> camera, std::shared_ptr<FrameGroup> frameGroup, nb::ndarray<nb::numpy> ndarray) { //,uint8_t, nb::shape<8, 1024, 1280>, nb::c_contig, nb::device::cpu
        nanobind::gil_scoped_release release;
        //uint8_t *stand_in_data = new uint8_t[8] { 1, 2, 3, 4, 5, 6, 7, 8 };
        //size_t stand_in_shape[3] = { 8, 1, 1 };
        //nb::capsule stand_in_deleter(stand_in_data, [](void *p) noexcept { delete[] (uint8_t *) p; });
        //nb::ndarray<nb::numpy, uint8_t> ndarray = nb::ndarray<nb::numpy, uint8_t>(stand_in_data, 3, stand_in_shape, stand_in_deleter);

        size_t offset = 0;
        if(frameGroup && frameGroup != nullptr && frameGroup->Count() > 0 ){
            int count = frameGroup->Count();
            uint8_t* full_buffer = nullptr;
            int height = 0, width = 0;
            //unsigned int last_address = 0;
            //printf("[WARNING] FrameGroup Count = %i\n", count);

            for(int i = 0; i < count; i++){
                std::shared_ptr<const Frame> frame = frameGroup->GetFrame(i);
                if(!(frame->IsInvalid())){
                    int size = frame->GrayscaleDataSize();

                    if(offset == 0) {
                        //stand_in_deleter.release();
                        height = frame->Height(); width = frame->Width();

                        while(full_buffer == nullptr){
                            full_buffer = (uint8_t*)ndarray.data();//(uint8_t*) malloc(size * (count+1));
                            if(full_buffer == nullptr){
                                printf("[ERROR] Failed to allocate memory for full buffer!  Trying again...\n");
                                Sleep(2);
                            }
                        }
                    }
                    if(size > width * height || ndarray.shape(1) < height || ndarray.shape(2) < width){
                        printf("[WARNING] Couldn't MemCpy, wrong shape; Count = %i, Offset = %zi, Size = %i, Width = %i, Height = %i\n", count, offset, size, width, height);
                        break;
                    }else{
                        const uint8_t* data = frame->GrayscaleData(*camera);
                        // Copy the frame from the Optitrack SDK to our contiguous Numpy-Managed Buffer
                        //printf("[WARNING] Starting MemCpy at address: %zu, offset forward by %zu\n", (size_t)data, (size_t)data - last_address);
                        //last_address = (size_t)data;
                        memcpy(full_buffer + offset, data, size);
                        offset += size;
                    }
                } else {
                    printf("[WARNING] Subframe was Empty or Invalid!");// From camera : % i\n", frame->GetCamera()->Serial());
                }
            }
        }else{
            printf("[WARNING] Framegroup is a nullptr or has an invalid number of cameras!\n");
        }
        //frameGroup->Release();
        if(offset == 0) { printf("[WARNING] No full or valid frames were found in the FrameGroup!  Returning Default ndarray...\n"); }
        //return ndarray;
        nanobind::gil_scoped_acquire acquire;
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
        //.value("VideoModeCount", Core::eVideoMode::Video)
        .value("UnknownMode", Core::eVideoMode::UnknownMode);
        //.export_values();

    nb::enum_<CameraLibrary::eImagerGain>(m, "eImagerGain")
        .value("Gain_Level0", CameraLibrary::eImagerGain::Gain_Level0)
        .value("Gain_Level1", CameraLibrary::eImagerGain::Gain_Level1)
        .value("Gain_Level2", CameraLibrary::eImagerGain::Gain_Level2)
        .value("Gain_Level3", CameraLibrary::eImagerGain::Gain_Level3)
        .value("Gain_Level4", CameraLibrary::eImagerGain::Gain_Level4)
        .value("Gain_Level5", CameraLibrary::eImagerGain::Gain_Level5)
        .value("Gain_Level6", CameraLibrary::eImagerGain::Gain_Level6)
        .value("Gain_Level7", CameraLibrary::eImagerGain::Gain_Level7);

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

    nb::enum_<CameraLibrary::cModuleSync::eOptimization>(m, "eOptimization")
        .value("ForceTimelyDelivery"  , CameraLibrary::cModuleSync::eOptimization::ForceTimelyDelivery)
        .value("FavorTimelyDelivery", CameraLibrary::cModuleSync::eOptimization::FavorTimelyDelivery)
        .value("ForceCompleteDelivery", CameraLibrary::cModuleSync::eOptimization::ForceCompleteDelivery)
        .value("eOptimizationCount", CameraLibrary::cModuleSync::eOptimization::eOptimizationCount);

    nb::class_<sStatusLightColor>(m, "sStatusLightColor")
        .def(nb::init())
        .def_rw("Red", &sStatusLightColor::Red)
        .def_rw("Green", &sStatusLightColor::Green)
        .def_rw("Blue", &sStatusLightColor::Blue);

    nb::class_<CameraLibrary::cModuleSync>(m, "cModuleSync")
        .def_static("Create", CameraLibrary::cModuleSync::Create, nb::rv_policy::reference)
        .def_static("Destroy", CameraLibrary::cModuleSync::Destroy)
        .def("PostFrame", &CameraLibrary::cModuleSync::PostFrame)
        .def("FrameDeliveryRate", &CameraLibrary::cModuleSync::FrameDeliveryRate) // Virtual
        .def("AddCamera", &CameraLibrary::cModuleSync::AddCamera)
        .def("CameraCount", &CameraLibrary::cModuleSync::CameraCount)
        .def("GetFrameGroup", &CameraLibrary::cModuleSync::GetFrameGroup)
        //.def("GetFrameGroupSharedPtr", &CameraLibrary::cModuleSync::GetFrameGroupSharedPtr)
        .def("LastFrameGroupMode", &CameraLibrary::cModuleSync::LastFrameGroupMode)
        .def("AllowIncompleteGroups", &CameraLibrary::cModuleSync::AllowIncompleteGroups)
        .def("SetAllowIncompleteGroups", &CameraLibrary::cModuleSync::SetAllowIncompleteGroups)
        .def("SetOptimization", &CameraLibrary::cModuleSync::SetOptimization)
        .def("Optimization", &CameraLibrary::cModuleSync::Optimization)
        .def("RemoveAllCameras", &CameraLibrary::cModuleSync::RemoveAllCameras)
        .def("SetSuppressOutOfOrder", &CameraLibrary::cModuleSync::SetSuppressOutOfOrder)
        //.def("IsSuppressOutOfOrder", &CameraLibrary::cModuleSync::Out);
        //.def("FlushFrames", &CameraLibrary::cModuleSync::FlushFrames)
        ;

    //nb::class_<CameraLibrary::cModuleSync, CameraLibrary::cModuleSync>(m, "cModuleSync")
    //
    //    ;

    nb::enum_<CameraLibrary::FrameGroup::Modes>(m, "Modes")
        .value("None"  , CameraLibrary::FrameGroup::Modes::None)
        .value("Software", CameraLibrary::FrameGroup::Modes::Software)
        .value("Hardware", CameraLibrary::FrameGroup::Modes::Hardware)
        //.value("ModeCount", CameraLibrary::FrameGroup::Modes::ModeCount)
        ;

    //nb::class_<DroppedFrameInfo>(m, "DroppedFrameInfo")
    //    //.def(nb::init())
    //    .def("Serial", &DroppedFrameInfo::Serial)
    //    .def("UserData", &DroppedFrameInfo::UserData);

    nb::class_<FrameGroup>(m, "FrameGroup")
        .def(nb::init())
        .def("Count", &FrameGroup::Count)
        .def("GetFrame", &FrameGroup::GetFrame, nb::rv_policy::reference)
        //.def("GetFrameUserData", &FrameGroup::GetFrameUserData)
        //.def("AddRef", &FrameGroup::AddRef)
        //.def("Release", &FrameGroup::Release)
        .def("AddFrame", &FrameGroup::AddFrame)
        //.def("Clear", &FrameGroup::Clear)
        .def("SetMode", &FrameGroup::SetMode)
        .def("Mode", &FrameGroup::Mode)
        .def("SetTimeStamp", &FrameGroup::SetTimeStamp)
        .def("SetTimeSpread", &FrameGroup::SetTimeSpread)
        .def("SetEarliestTimeStamp", &FrameGroup::SetEarliestTimeStamp)
        .def("SetLatestTimeStamp", &FrameGroup::SetLatestTimeStamp)
        .def("TimeSpread", &FrameGroup::TimeSpread)
        .def("TimeStamp", &FrameGroup::TimeStamp)
        .def("EarliestTimeStamp", &FrameGroup::EarliestTimeStamp)
        .def("LatestTimeStamp", &FrameGroup::LatestTimeStamp)
        .def("FrameID", &FrameGroup::FrameID)
        .def("TimeSpreadDeviation", &FrameGroup::TimeSpreadDeviation)
        //.def("DroppedFrameCount", &FrameGroup::DroppedFrameCount)
        .def("DroppedFrames", &FrameGroup::DroppedFrames)
        ;

    nb::class_<cCameraLibraryStartupSettings>(m, "cCameraLibraryStartupSettings")
        .def(nb::init())
        .def_static("X", cCameraLibraryStartupSettings::X)
        .def("EnableDevelopment", &cCameraLibraryStartupSettings::EnableDevelopment)
        .def("IsDevelopmentEnabled", &cCameraLibraryStartupSettings::IsDevelopmentEnabled);

    nb::class_<Frame>(m, "Frame")
        //.def(nb::init())
        .def("ObjectCount", &Frame::ObjectCount)
        .def("FrameID", &Frame::FrameID)
        .def("FrameType", &Frame::FrameType)
        .def("MJPEGQuality", &Frame::MJPEGQuality)
        .def("Object", &Frame::Object)
        //.def("GetLink", &Frame::GetLink)
        //.def("GetCamera", &Frame::GetCamera)
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
        //.def("IsTimeCodeValid", &Frame::IsTimeCodeValid)
        .def("IsExternalLocked", &Frame::IsExternalLocked)
        .def("IsRecording", &Frame::IsRecording)
        //.def("TimeCode", &Frame::TimeCode)
        //.def("IMUTelemetryCount", &Frame::IMUTelemetryCount)
        //.def("IMUTelemetry", &Frame::IMUTelemetry)
        .def("HardwareTimeStampValue", &Frame::HardwareTimeStampValue)
        .def("HardwareTimeStamp", &Frame::HardwareTimeStamp)
        .def("IsHardwareTimeStamp", &Frame::IsHardwareTimeStamp)
        .def("HardwareTimeFreq", &Frame::HardwareTimeFreq)
        .def("MasterTimingDevice", &Frame::MasterTimingDevice)
        //.def("Release", &Frame::Release)
        //.def("RefCount", &Frame::RefCount)
        //.def("AddRef", &Frame::AddRef)
        //.def("Rasterize", &Frame::Rasterize) // Overloads!
        //.def("Rasterize", &Frame::Rasterize)
        .def("ImageDataSize", &Frame::ImageDataSize)
        //.def("GrayscaleData", Frame::GrayscaleData) // <-- TODO: Need to pass in a pointer...
        .def("GrayscaleDataSize", &Frame::GrayscaleDataSize)
        .def("SetObjectCount", &Frame::SetObjectCount)
        .def("RemoveObject", &Frame::RemoveObject)
        //.def("HardwareRecording", &Frame::HardwareRecording)
        ;

    nb::class_<Camera>(m, "Camera")
        //.def(nb::init())
        .def("Width", &Camera::Width)
        .def("Height", &Camera::Height)
        .def("LatestFrame", &Camera::LatestFrame)
        .def("NextFrame", &Camera::NextFrame)
        .def("Name", &Camera::Name)
        .def("Start", &Camera::Start)
        .def("Stop", &Camera::Stop)
        .def("IsCameraRunning", &Camera::IsCameraRunning)
        //.def("Release", &Camera::Release) // Virtual
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
        //.def("IsQuietModeAvailable", &Camera::IsQuietModeAvailable) // Virtual
        //.def("SetQuietMode", &Camera::SetQuietMode) // Virtual
        //.def("QuietMode", &Camera::QuietMode)
        .def("SetRinglightEnabledWhileStopped", &Camera::SetRinglightEnabledWhileStopped) // Virtual
        .def("RinglightEnabledWhileStopped", &Camera::RinglightEnabledWhileStopped) // Virtual
        .def("IsHardwareFiltered", &Camera::IsHardwareFiltered) // Virtual, Uses Enum
        .def("SwitchState", &Camera::SwitchState)
        //.def("Health", &Camera::Health)
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
        .def("IsInitialized", &Camera::IsInitialized)
        .def("IsDisconnected", &Camera::IsDisconnected)
        .def("State", &Camera::State)
        .def("UID", &Camera::UID)
        .def("ConnectionType", &Camera::ConnectionType)
        .def("IsVirtual", &Camera::IsVirtual) // Virtual
        //.def("AttachInput", &Camera::AttachInput) // Virtual
        //.def("DetachInput", &Camera::DetachInput)
        //.def("TransferInput", &Camera::TransferInput) // Virtual
        .def("IsCommandQueueEmpty", &Camera::IsCommandQueueEmpty)
        //.def("ReleaseFrame", &Camera::ReleaseFrame)
        //.def("DevicePath", &Camera::DevicePath)
        //.def("SendCommand", &Camera::SendCommand) // Virtual
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
        .def("SetGrayscaleDecimation", &Camera::SetGrayscaleDecimation) // Virtual
        //.def("FrameSize", &Camera::FrameSize) // Virtual

        .def("SetEnablePayload", &Camera::SetEnablePayload) // Virtual
        .def("IsEnablePayload", &Camera::IsEnablePayload) // Virtual
        .def("IsCameraTempValid", &Camera::IsCameraTempValid) // Virtual
        .def("CameraTemp", &Camera::CameraTemp) // Virtual
        .def("IsRinglightTempValid", &Camera::IsRinglightTempValid) // Virtual
        .def("RinglightTemp", &Camera::RinglightTemp) // Virtual
        //.def("IsCameraFanSpeedValid", &Camera::IsCameraFanSpeedValid) // Virtual
        //.def("CameraFanSpeed", &Camera::CameraFanSpeed) // Virtual
        .def("IsPoEPlusActive", &Camera::IsPoEPlusActive) // Virtual
        .def("SetLLDPDetection", &Camera::SetLLDPDetection) // Uses Enum
        .def("IsLLDPDetectionAvailable", &Camera::IsLLDPDetectionAvailable) // Virtual  // Uses Enum
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
        //.def("SetPixelIntensityMapping", &Camera::SetPixelIntensityMapping) // Virtual
        ;

    nb::class_<CameraManager>(m, "CameraManager")
        //.def(nb::init())
        .def_static("X", CameraManager::X, nb::rv_policy::reference)
        .def("WaitForInitialization", &CameraManager::WaitForInitialization)
        .def("AreCamerasInitialized", &CameraManager::AreCamerasInitialized)
        .def("AreCamerasShutdown", &CameraManager::AreCamerasShutdown)
        .def("Shutdown", &CameraManager::Shutdown)
        .def("GetCameraBySerial", &CameraManager::GetCameraBySerial, nb::rv_policy::reference)
        .def("GetCamera", nb::overload_cast<const Core::cUID&>(&CameraManager::GetCamera), nb::rv_policy::reference) // Overloads!
        .def("GetCamera", nb::overload_cast<>(&CameraManager::GetCamera), nb::rv_policy::reference) // Overloads! , nb::rv_policy::reference
        //.def("GetCamera", &CameraManager::GetCamera)
        .def("GetCameraList", &CameraManager::GetCameraList)
        .def("GetHardwareKey", &CameraManager::GetHardwareKey)
        .def("GetDevice", &CameraManager::GetDevice)
        .def("PrepareForSuspend", &CameraManager::PrepareForSuspend)
        .def("ResumeFromSuspend", &CameraManager::ResumeFromSuspend)
        .def("TimeStamp", &CameraManager::TimeStamp)
        //.def("TimeStampFrequency", &CameraManager::TimeStampFrequency)
        //.def("ResetTimeStamp", &CameraManager::ResetTimeStamp)
        .def("RegisterListener", &CameraManager::RegisterListener)
        .def("UnregisterListener", &CameraManager::UnregisterListener)
        .def_static("CameraFactory", CameraManager::CameraFactory)
        .def("AddCamera", &CameraManager::AddCamera)
        .def("RemoveCamera", &CameraManager::RemoveCamera)
        //.def("RemoveVirtualCameras", &CameraManager::RemoveCamera)
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

    nb::class_<cCameraManagerListener>(m, "CameraManagerListener")
        //.def(nb::init())
        .def("CameraConnected", &cCameraManagerListener::CameraConnected) // Virtual
        .def("CameraRemoved", &cCameraManagerListener::CameraRemoved) // Virtual
        .def("SyncSettingsChanged", &cCameraManagerListener::SyncSettingsChanged) // Virtual
        .def("CameraInitialized", &cCameraManagerListener::CameraInitialized) // Virtual
        .def("SyncAuthorityInitialized", &cCameraManagerListener::SyncAuthorityInitialized) // Virtual
        .def("SyncAuthorityRemoved", &cCameraManagerListener::SyncAuthorityRemoved) // Virtual
        .def("CameraMessage", &cCameraManagerListener::CameraMessage) // Virtual
        .def("RequestUnknownDeviceImplementation", &cCameraManagerListener::RequestUnknownDeviceImplementation) // Virtual
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

    //nb::class_<Core::cTimeCode>(m, "cTimeCode")
    //    .def(nb::init())
    //    //.def("from_string", &Core::cUID::from_string)
    //    .def("SetValue", &Core::cTimeCode::)
    //    ;
}
