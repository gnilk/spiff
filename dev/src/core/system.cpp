#include "nxcore.h"

// ----------------------------
//
// System
//
using namespace NXCore;

static System glb_system;
Utils::ITimer *System::GetTimer() {
	return dynamic_cast<ITimer*>(new Timer());
}

ILogger *System::GetLogger(const char *name) {
	return Logger::GetLogger(name);
}

void System::RaiseError() {

}

ISystem *System::GetInstance() {
	return dynamic_cast<ISystem *>(&glb_system);
}

IConfig *System::GetConfig() {
	return dynamic_cast<IConfig *>(&configuration);
}
ObjectContainer *System::GetObjectContainer() {
	return &objectContainer;

}


// ----------------------
//
// BaseManager
//

ISystem *BaseManager::GetSystem() {
	return System::GetInstance();
}

