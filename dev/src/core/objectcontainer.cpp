#include <boost/any.hpp>
#include <string>
#include <map>
#include <functional>

#include "nxcore.h"

using namespace NXCore;
using boost::any_cast;


ObjectContainer::ObjectContainer() {

}
ObjectContainer::~ObjectContainer() {
	// todo: clear objects and factories here
}

bool ObjectContainer::RegisterInstance(boost::any object, std::string identifier) {
	bool result = false;
	if (!HasInstance(identifier)) {
		objects.insert(std::make_pair(identifier, object));
		result = true;
	} else {
		Logger::GetLogger("ObjectContainer")->Error("Instance '%s' already existing", identifier.c_str());
	}
	return result;
}

bool ObjectContainer::HasInstance(std::string identifier) {
	auto it = objects.find(identifier);
	if (it == objects.end()) {
		return false;
	}
	return true;
}

boost::any ObjectContainer::GetInstance(std::string identifier) {
	auto it = objects.find(identifier);
	if (it == objects.end()) {
		return NULL;
	}
	return it->second;
}

template <class T>
bool ObjectContainer::RegisterFactory(std::function<T()> factory, std::string identifier) {
	factories.insert(std::make_pair(identifier, factory));
}

template <class T>
T ObjectContainer::CreateInstance(std::string identifier, bool autoregister) {
	auto it = factories.find(identifier);
	if (it != factories.end()) {
	 	try
	    {	    	
	    	if (autoregister && HasInstance(identifier)) {
	    		Logger::GetLogger("conapp")->Debug("Instance already exist, returning existing");
	    		return boost::any_cast<T>(GetInstance(identifier));
	    	}
	    	std::function<T()> factory = boost::any_cast<std::function<T()> >(it->second);	
	    	T res = factory();
	    	RegisterInstance(boost::any(res), identifier);
	    	return res;
	    }
	    catch(const boost::bad_any_cast &)
	    {
			std::string name = it->second.type().name();
			std::string tname = typeid(T).name();
			Logger::GetLogger("conapp")->Debug("Bad type in factory type, name=%s != tname=%s",name.c_str(),tname.c_str());
	       // return false;
	    }	
	}
}
