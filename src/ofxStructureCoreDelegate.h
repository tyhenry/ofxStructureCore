#pragma once
#include <chrono>
#include <map>
#include <thread>
#include <vector>

#include "ST/CaptureSession.h"
#include "ST/DeviceManager.h"

class ofxStructureCore;

namespace ofx {
namespace structure {

	struct DeviceManager : public ST::CaptureSessionDelegate
	{
		DeviceManager();
		~DeviceManager();

		// delegate functions overrides
		void captureSessionEventDidOccur( ST::CaptureSession* session, ST::CaptureSessionEventId evt ) override;
		void captureSessionDidOutputSample( ST::CaptureSession* session, const ST::CaptureSessionSample& sample ) override;

		std::vector<std::string> listDetectedSerials();

	protected:
		ST::DeviceManager _deviceManager;
		std::map<ST::CaptureSession*, std::string> _sessionSerials;  // session : serial
		std::map<std::string, ofxStructureCore*> _devices;           // serial : device

		friend class ofxStructureCore;
		void attach( const std::string& serial, ofxStructureCore* device );
		void release( ofxStructureCore* device );
	};

	DeviceManager& deviceManager();  // singleton

}  // namespace structure
}  // namespace ofx
