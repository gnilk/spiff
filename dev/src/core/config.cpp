#include <string>
#include "nxcore.h"
#include "inifile.h"
//#include "PropertyContainer.h"

using namespace gnilk;
using namespace gnilk::inifile;
using namespace NXCore;

// todo: move the property container to the 21st century...

std::string Config::GetValue(std::string section, std::string name, std::string defaultValue) {
	return container.GetValue(section, name, defaultValue);
}

bool Config::SetValue(std::string section, std::string name, std::string value) {
	container.SetValue(section, name, value);
	return true;
}

bool Config::HasValue(std::string section, std::string name) {
	return container.HasValue(section, name);
}

void Config::ReadIniFileStructure(std::string data) {
	container.FromBuffer(data);
}

bool Config::DeleteValue(std::string section, std::string name) {

}

bool Config::DeleteSection(std::string section) {

}