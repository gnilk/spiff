// ---------------------
//
// this is under refactoring (split to separate files)
//
#include "nxcore.h"
#include "xmlparser.h"
#include <functional>

using namespace Services;
using namespace NXCore;
using namespace gnilk::xml;


// -----------------
//
// Parameter Base
//
ParameterBase::ParameterBase(std::string _name) {
	name = _name;
	typeName = "no-type";
	value = "no-value";
}
ParameterBase::~ParameterBase() {

}
std::string &ParameterBase::GetName() {
	return name;
}
std::string &ParameterBase::GetTypeName() {
	return typeName;
}
std::string &ParameterBase::GetValueAsString() {
	return value;	// this is stupid..
}

// -----------------
//
// Parameter Definition
//
ParameterDefinition::ParameterDefinition(std::string _name) : 
	ParameterBase(_name) {

}
ParameterDefinition::~ParameterDefinition() {

}

void ParameterDefinition::SetTypeName(std::string _typeName) {
	typeName = _typeName;
}
void ParameterDefinition::SetDescription(std::string _description) {
	description = _description;
}
void ParameterDefinition::SetDisplayName(std::string _displayName) {
	displayName = _displayName;
}
void ParameterDefinition::SetDefaultValue(std::string _defaultValue) {
	defaultValue = _defaultValue;
}
std::string ParameterDefinition::ToXML() {
	std::string xml("<parameter ");

	xml += std::string("name=\"")+name+std::string("\" ");
	xml += std::string("display=\"")+displayName+std::string("\" ");
	xml += std::string("type=\"")+typeName+std::string("\" ");
	xml += std::string("default=\"")+defaultValue+std::string("\" ");
	xml += std::string("description=\"")+description+std::string("\"");

	xml += "/>";
	return xml;
}










