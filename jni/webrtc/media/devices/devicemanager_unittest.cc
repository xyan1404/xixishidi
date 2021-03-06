/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/devices/devicemanager.h"

#ifdef WIN32
#include <objbase.h>
#include "webrtc/base/win32.h"
#endif

#include <memory>
#include <string>

#include "webrtc/base/arraysize.h"
#include "webrtc/base/fileutils.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"
#include "webrtc/base/windowpickerfactory.h"
#include "webrtc/media/base/fakevideocapturer.h"
#include "webrtc/media/base/screencastid.h"
#include "webrtc/media/base/testutils.h"
#include "webrtc/media/base/videocapturerfactory.h"
#include "webrtc/media/devices/v4llookup.h"

#ifdef WEBRTC_LINUX
// TODO(juberti): Figure out why this doesn't compile on Windows.
#include "webrtc/base/fileutils_mock.h"
#endif  // WEBRTC_LINUX

using rtc::Pathname;
using rtc::FileTimeType;
using cricket::Device;
using cricket::DeviceManager;
using cricket::DeviceManagerFactory;
using cricket::DeviceManagerInterface;

const cricket::VideoFormat kVgaFormat(640, 480,
                                      cricket::VideoFormat::FpsToInterval(30),
                                      cricket::FOURCC_I420);
const cricket::VideoFormat kHdFormat(1280, 720,
                                     cricket::VideoFormat::FpsToInterval(30),
                                     cricket::FOURCC_I420);

class FakeVideoDeviceCapturerFactory :
    public cricket::VideoDeviceCapturerFactory {
 public:
  FakeVideoDeviceCapturerFactory() {}
  virtual ~FakeVideoDeviceCapturerFactory() {}

  virtual cricket::VideoCapturer* Create(const cricket::Device& device) {
    return new cricket::FakeVideoCapturer;
  }
};

class FakeScreenCapturerFactory : public cricket::ScreenCapturerFactory {
 public:
  FakeScreenCapturerFactory() {}
  virtual ~FakeScreenCapturerFactory() {}

  virtual cricket::VideoCapturer* Create(
      const cricket::ScreencastId& screenid) {
    return new cricket::FakeVideoCapturer;
  }
};

class DeviceManagerTestFake : public testing::Test {
 public:
  virtual void SetUp() {
    dm_.reset(DeviceManagerFactory::Create());
    EXPECT_TRUE(dm_->Init());
    DeviceManager* device_manager = static_cast<DeviceManager*>(dm_.get());
    device_manager->SetVideoDeviceCapturerFactory(
        new FakeVideoDeviceCapturerFactory());
    device_manager->SetScreenCapturerFactory(
        new FakeScreenCapturerFactory());
  }

  virtual void TearDown() {
    dm_->Terminate();
  }

 protected:
  std::unique_ptr<DeviceManagerInterface> dm_;
};


// Test that we startup/shutdown properly.
TEST(DeviceManagerTest, StartupShutdown) {
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  EXPECT_TRUE(dm->Init());
  dm->Terminate();
}

// Test CoInitEx behavior
#ifdef WIN32
TEST(DeviceManagerTest, CoInitialize) {
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> devices;
  // Ensure that calls to video device work if COM is not yet initialized.
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&devices));
  dm->Terminate();
  // Ensure that the ref count is correct.
  EXPECT_EQ(S_OK, CoInitializeEx(NULL, COINIT_MULTITHREADED));
  CoUninitialize();
  // Ensure that Init works in COINIT_APARTMENTTHREADED setting.
  EXPECT_EQ(S_OK, CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
  EXPECT_TRUE(dm->Init());
  dm->Terminate();
  CoUninitialize();
  // Ensure that the ref count is correct.
  EXPECT_EQ(S_OK, CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
  CoUninitialize();
  // Ensure that Init works in COINIT_MULTITHREADED setting.
  EXPECT_EQ(S_OK, CoInitializeEx(NULL, COINIT_MULTITHREADED));
  EXPECT_TRUE(dm->Init());
  dm->Terminate();
  CoUninitialize();
  // Ensure that the ref count is correct.
  EXPECT_EQ(S_OK, CoInitializeEx(NULL, COINIT_MULTITHREADED));
  CoUninitialize();
}
#endif

// Test enumerating devices (although we may not find any).
TEST(DeviceManagerTest, GetDevices) {
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> audio_ins, audio_outs, video_ins;
  std::vector<cricket::Device> video_in_devs;
  cricket::Device def_video;
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetAudioInputDevices(&audio_ins));
  EXPECT_TRUE(dm->GetAudioOutputDevices(&audio_outs));
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_in_devs));
  EXPECT_EQ(video_ins.size(), video_in_devs.size());
  // If we have any video devices, we should be able to pick a default.
  EXPECT_TRUE(dm->GetVideoCaptureDevice(
      cricket::DeviceManagerInterface::kDefaultDeviceName, &def_video)
      != video_ins.empty());
}

// Test that we return correct ids for default and bogus devices.
TEST(DeviceManagerTest, GetAudioDeviceIds) {
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  Device device;
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetAudioInputDevice(
      cricket::DeviceManagerInterface::kDefaultDeviceName, &device));
  EXPECT_EQ("-1", device.id);
  EXPECT_TRUE(dm->GetAudioOutputDevice(
      cricket::DeviceManagerInterface::kDefaultDeviceName, &device));
  EXPECT_EQ("-1", device.id);
  EXPECT_FALSE(dm->GetAudioInputDevice("_NOT A REAL DEVICE_", &device));
  EXPECT_FALSE(dm->GetAudioOutputDevice("_NOT A REAL DEVICE_", &device));
}

// Test that we get the video capture device by name properly.
TEST(DeviceManagerTest, GetVideoDeviceIds) {
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  Device device;
  EXPECT_TRUE(dm->Init());
  EXPECT_FALSE(dm->GetVideoCaptureDevice("_NOT A REAL DEVICE_", &device));
  std::vector<Device> video_ins;
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  if (!video_ins.empty()) {
    // Get the default device with the parameter kDefaultDeviceName.
    EXPECT_TRUE(dm->GetVideoCaptureDevice(
        cricket::DeviceManagerInterface::kDefaultDeviceName, &device));

    // Get the first device with the parameter video_ins[0].name.
    EXPECT_TRUE(dm->GetVideoCaptureDevice(video_ins[0].name, &device));
    EXPECT_EQ(device.name, video_ins[0].name);
    EXPECT_EQ(device.id, video_ins[0].id);
  }
}

TEST(DeviceManagerTest, VerifyDevicesListsAreCleared) {
  const std::string imaginary("_NOT A REAL DEVICE_");
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> audio_ins, audio_outs, video_ins;
  audio_ins.push_back(Device(imaginary, imaginary));
  audio_outs.push_back(Device(imaginary, imaginary));
  video_ins.push_back(Device(imaginary, imaginary));
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetAudioInputDevices(&audio_ins));
  EXPECT_TRUE(dm->GetAudioOutputDevices(&audio_outs));
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  for (size_t i = 0; i < audio_ins.size(); ++i) {
    EXPECT_NE(imaginary, audio_ins[i].name);
  }
  for (size_t i = 0; i < audio_outs.size(); ++i) {
    EXPECT_NE(imaginary, audio_outs[i].name);
  }
  for (size_t i = 0; i < video_ins.size(); ++i) {
    EXPECT_NE(imaginary, video_ins[i].name);
  }
}

static bool CompareDeviceList(std::vector<Device>& devices,
    const char* const device_list[], int list_size) {
  if (list_size != static_cast<int>(devices.size())) {
    return false;
  }
  for (int i = 0; i < list_size; ++i) {
    if (devices[i].name.compare(device_list[i]) != 0) {
      return false;
    }
  }
  return true;
}

TEST(DeviceManagerTest, VerifyFilterDevices) {
  static const char* const kTotalDevicesName[] = {
      "Google Camera Adapters are tons of fun.",
      "device1",
      "device2",
      "device3",
      "device4",
      "device5",
      "Google Camera Adapter 0",
      "Google Camera Adapter 1",
  };
  static const char* const kFilteredDevicesName[] = {
      "device2",
      "device4",
      "Google Camera Adapter",
      NULL,
  };
  static const char* const kDevicesName[] = {
      "device1",
      "device3",
      "device5",
  };
  std::vector<Device> devices;
  for (int i = 0; i < arraysize(kTotalDevicesName); ++i) {
    devices.push_back(Device(kTotalDevicesName[i], i));
  }
  EXPECT_TRUE(CompareDeviceList(devices, kTotalDevicesName,
                                arraysize(kTotalDevicesName)));
  // Return false if given NULL as the exclusion list.
  EXPECT_TRUE(DeviceManager::FilterDevices(&devices, NULL));
  // The devices should not change.
  EXPECT_TRUE(CompareDeviceList(devices, kTotalDevicesName,
                                arraysize(kTotalDevicesName)));
  EXPECT_TRUE(DeviceManager::FilterDevices(&devices, kFilteredDevicesName));
  EXPECT_TRUE(CompareDeviceList(devices, kDevicesName,
                                arraysize(kDevicesName)));
}

#ifdef WEBRTC_LINUX
class FakeV4LLookup : public cricket::V4LLookup {
 public:
  explicit FakeV4LLookup(std::vector<std::string> device_paths)
      : device_paths_(device_paths) {}

 protected:
  bool CheckIsV4L2Device(const std::string& device) {
    return std::find(device_paths_.begin(), device_paths_.end(), device)
        != device_paths_.end();
  }

 private:
  std::vector<std::string> device_paths_;
};

TEST(DeviceManagerTest, GetVideoCaptureDevices_K2_6) {
  std::vector<std::string> devices;
  devices.push_back("/dev/video0");
  devices.push_back("/dev/video5");
  cricket::V4LLookup::SetV4LLookup(new FakeV4LLookup(devices));

  std::vector<rtc::FakeFileSystem::File> files;
  files.push_back(rtc::FakeFileSystem::File("/dev/video0", ""));
  files.push_back(rtc::FakeFileSystem::File("/dev/video5", ""));
  files.push_back(rtc::FakeFileSystem::File(
      "/sys/class/video4linux/video0/name", "Video Device 1"));
  files.push_back(rtc::FakeFileSystem::File(
      "/sys/class/video4linux/video1/model", "Bad Device"));
  files.push_back(
      rtc::FakeFileSystem::File("/sys/class/video4linux/video5/model",
                                      "Video Device 2"));
  rtc::FilesystemScope fs(new rtc::FakeFileSystem(files));

  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> video_ins;
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  EXPECT_EQ(2u, video_ins.size());
  EXPECT_EQ("Video Device 1", video_ins.at(0).name);
  EXPECT_EQ("Video Device 2", video_ins.at(1).name);
}

TEST(DeviceManagerTest, GetVideoCaptureDevices_K2_4) {
  std::vector<std::string> devices;
  devices.push_back("/dev/video0");
  devices.push_back("/dev/video5");
  cricket::V4LLookup::SetV4LLookup(new FakeV4LLookup(devices));

  std::vector<rtc::FakeFileSystem::File> files;
  files.push_back(rtc::FakeFileSystem::File("/dev/video0", ""));
  files.push_back(rtc::FakeFileSystem::File("/dev/video5", ""));
  files.push_back(rtc::FakeFileSystem::File(
          "/proc/video/dev/video0",
          "param1: value1\nname: Video Device 1\n param2: value2\n"));
  files.push_back(rtc::FakeFileSystem::File(
          "/proc/video/dev/video1",
          "param1: value1\nname: Bad Device\n param2: value2\n"));
  files.push_back(rtc::FakeFileSystem::File(
          "/proc/video/dev/video5",
          "param1: value1\nname:   Video Device 2\n param2: value2\n"));
  rtc::FilesystemScope fs(new rtc::FakeFileSystem(files));

  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> video_ins;
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  EXPECT_EQ(2u, video_ins.size());
  EXPECT_EQ("Video Device 1", video_ins.at(0).name);
  EXPECT_EQ("Video Device 2", video_ins.at(1).name);
}

TEST(DeviceManagerTest, GetVideoCaptureDevices_KUnknown) {
  std::vector<std::string> devices;
  devices.push_back("/dev/video0");
  devices.push_back("/dev/video5");
  cricket::V4LLookup::SetV4LLookup(new FakeV4LLookup(devices));

  std::vector<rtc::FakeFileSystem::File> files;
  files.push_back(rtc::FakeFileSystem::File("/dev/video0", ""));
  files.push_back(rtc::FakeFileSystem::File("/dev/video1", ""));
  files.push_back(rtc::FakeFileSystem::File("/dev/video5", ""));
  rtc::FilesystemScope fs(new rtc::FakeFileSystem(files));

  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  std::vector<Device> video_ins;
  EXPECT_TRUE(dm->Init());
  EXPECT_TRUE(dm->GetVideoCaptureDevices(&video_ins));
  EXPECT_EQ(2u, video_ins.size());
  EXPECT_EQ("/dev/video0", video_ins.at(0).name);
  EXPECT_EQ("/dev/video5", video_ins.at(1).name);
}
#endif  // WEBRTC_LINUX

// TODO(noahric): These are flaky on windows on headless machines.
#ifndef WIN32
TEST(DeviceManagerTest, GetWindows) {
  if (!rtc::WindowPickerFactory::IsSupported()) {
    LOG(LS_INFO) << "skipping test: window capturing is not supported with "
                 << "current configuration.";
    return;
  }
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  dm->SetScreenCapturerFactory(new FakeScreenCapturerFactory());
  std::vector<rtc::WindowDescription> descriptions;
  EXPECT_TRUE(dm->Init());
  if (!dm->GetWindows(&descriptions) || descriptions.empty()) {
    LOG(LS_INFO) << "skipping test: window capturing. Does not have any "
                 << "windows to capture.";
    return;
  }
  std::unique_ptr<cricket::VideoCapturer> capturer(dm->CreateScreenCapturer(
      cricket::ScreencastId(descriptions.front().id())));
  EXPECT_FALSE(capturer.get() == NULL);
  // TODO(hellner): creating a window capturer and immediately deleting it
  // results in "Continuous Build and Test Mainline - Mac opt" failure (crash).
  // Remove the following line as soon as this has been resolved.
  rtc::Thread::Current()->ProcessMessages(1);
}

TEST(DeviceManagerTest, GetDesktops) {
  if (!rtc::WindowPickerFactory::IsSupported()) {
    LOG(LS_INFO) << "skipping test: desktop capturing is not supported with "
                 << "current configuration.";
    return;
  }
  std::unique_ptr<DeviceManagerInterface> dm(DeviceManagerFactory::Create());
  dm->SetScreenCapturerFactory(new FakeScreenCapturerFactory());
  std::vector<rtc::DesktopDescription> descriptions;
  EXPECT_TRUE(dm->Init());
  if (!dm->GetDesktops(&descriptions) || descriptions.empty()) {
    LOG(LS_INFO) << "skipping test: desktop capturing. Does not have any "
                 << "desktops to capture.";
    return;
  }
  std::unique_ptr<cricket::VideoCapturer> capturer(dm->CreateScreenCapturer(
      cricket::ScreencastId(descriptions.front().id())));
  EXPECT_FALSE(capturer.get() == NULL);
}
#endif  // !WIN32

TEST_F(DeviceManagerTestFake, CaptureConstraintsWhitelisted) {
  const Device device("white", "white_id");
  dm_->SetVideoCaptureDeviceMaxFormat(device.name, kHdFormat);
  std::unique_ptr<cricket::VideoCapturer> capturer(
      dm_->CreateVideoCapturer(device));
  cricket::VideoFormat best_format;
  capturer->set_enable_camera_list(true);
  EXPECT_TRUE(capturer->GetBestCaptureFormat(kHdFormat, &best_format));
  EXPECT_EQ(kHdFormat, best_format);
}

TEST_F(DeviceManagerTestFake, CaptureConstraintsNotWhitelisted) {
  const Device device("regular", "regular_id");
  std::unique_ptr<cricket::VideoCapturer> capturer(
      dm_->CreateVideoCapturer(device));
  cricket::VideoFormat best_format;
  capturer->set_enable_camera_list(true);
  EXPECT_TRUE(capturer->GetBestCaptureFormat(kHdFormat, &best_format));
  EXPECT_EQ(kHdFormat, best_format);
}

TEST_F(DeviceManagerTestFake, CaptureConstraintsUnWhitelisted) {
  const Device device("un_white", "un_white_id");
  dm_->SetVideoCaptureDeviceMaxFormat(device.name, kHdFormat);
  dm_->ClearVideoCaptureDeviceMaxFormat(device.name);
  std::unique_ptr<cricket::VideoCapturer> capturer(
      dm_->CreateVideoCapturer(device));
  cricket::VideoFormat best_format;
  capturer->set_enable_camera_list(true);
  EXPECT_TRUE(capturer->GetBestCaptureFormat(kHdFormat, &best_format));
  EXPECT_EQ(kHdFormat, best_format);
}

TEST_F(DeviceManagerTestFake, CaptureConstraintsWildcard) {
  const Device device("any_device", "any_device");
  dm_->SetVideoCaptureDeviceMaxFormat("*", kHdFormat);
  std::unique_ptr<cricket::VideoCapturer> capturer(
      dm_->CreateVideoCapturer(device));
  cricket::VideoFormat best_format;
  capturer->set_enable_camera_list(true);
  EXPECT_TRUE(capturer->GetBestCaptureFormat(kHdFormat, &best_format));
  EXPECT_EQ(kHdFormat, best_format);
}
