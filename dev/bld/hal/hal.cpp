
#include <stdlib.h>
#include <stdio.h>

#include <process.h>

#include <string>
#include <list>
#include <functional>

#include "nxcore.h"
#include "xmlparser.h"


using namespace NXCore;
using namespace gnilk;
using namespace gnilk::xml;



typedef enum {
	kDTArray = 0,
	kDTDict = 1,
	kDTString = 2,
	kDTNumber = 3,
	kDTReal = 4,
	kDTInteger = 5,
	kDTBool = 6
} DataType;

class BaseElement {
public:
	BaseElement(DataType type) {
		this->elementType = type;

	}
	bool IsType(DataType t) {
		return (t == elementType);
	}
	DataType GetType() { return elementType; }
private:
	DataType elementType;
};

template <class T>
class PropertyElement : public BaseElement {
public:
	PropertyElement(DataType type) : BaseElement(type) {
	}
	T GetElement() {
		//T val = boost::any_cast<T>(element);
		return element;
	}
	virtual void SetElement(T _element) {
		element = _element;
	}
private:
	T element;
};

//
// TODO: Template specialisation
//

class PropertyXMLParser {
public:
	virtual BaseElement *ParseXML(std::list<ITag *> &tags);
	BaseElement *ParseElement(ITag *tag);
};

class PropertyXMLArrayParser : public PropertyXMLParser {
public:
	BaseElement *ParseXML(std::list<ITag *> &tags);
private:
	std::list<BaseElement *> elements;
};
class PropertyXMLDictParser : public PropertyXMLParser {
public:
	BaseElement *ParseXML(std::list<ITag *> &tags);
private:
	std::map<std::string, BaseElement *> elements;
};

class PropertyList : public PropertyXMLParser {
public:
	PropertyList *LoadFromXML(std::string data);
	void Traverse();
private:
	void TraverseList(std::list <BaseElement *> &list);
	void OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes);
	void OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes);

private:
	std::list <BaseElement *> elements;
	std::string keyName;
	std::string keyValue;
};

//////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//

BaseElement *PropertyXMLParser::ParseElement(ITag *tag) {
    BaseElement *element = NULL;

    if (tag->getName() == "array") {
    	PropertyXMLArrayParser arrayParser;
    	element = arrayParser.ParseXML(tag->getChildren());
    } else if (tag->getName() == "dict") {
    	PropertyXMLDictParser dictParser;
    	element = dictParser.ParseXML(tag->getChildren());
    }
	return element;	
}

static int depth = 0;
BaseElement *PropertyXMLParser::ParseXML(std::list<ITag *> &tags) {
	BaseElement *element = NULL;
	depth++;
	for(auto it : tags) {
		ITag *tag = it;
		if (depth == 1) {
			printf("TAG: %s\n",tag->getName().c_str());			
		}
		if (tag->getName() == "plist") {
			depth--;
			element = ParseXML(tag->getChildren());			
			return element;
		} else {
		    element = ParseElement(tag);
		}
	}
	return element;
}

// static 
BaseElement *PropertyXMLArrayParser::ParseXML(std::list<ITag *> &tags) {
	printf("Parsing array\n");
	for(auto it : tags) {
		ITag *tag = it;
		BaseElement *element = NULL;
		if (tag->getName() == "string") {
			std::string data = tag->getContent();
			PropertyElement<std::string> *str = new PropertyElement<std::string>(kDTString);
			str->SetElement(data);
			element = str;
			printf("string - %s\n", data.c_str());
		} else {
		    element = ParseElement(tag);
		}

	  	if (element != NULL) {
			elements.push_back(element);
	  	}
	}

	auto mylist = new PropertyElement<std::list<BaseElement *> >(kDTArray);
	mylist->SetElement(elements);
	return mylist;
}

// static
BaseElement *PropertyXMLDictParser::ParseXML(std::list<ITag *> &tags) {
	std::string key;
	printf("parsing dict\n");
	for(auto it : tags) {
		ITag *tag = it;
		BaseElement *element = NULL;
		if (tag->getName() == "key") {
			key = tag->getContent();
		} else {
			// TODO: check if key is empty
		    element = ParseElement(tag);		 
		}

		if (element != NULL) {
			// TODO: check for empty key
	  		elements.insert(std::make_pair(key, element));
			key = "";	// ensure empty
	  	}
	}

	//PropertyElement<std::map<std::string, BaseElement *> > *mymap = new PropertyElement<std::map>(kDTDict);
	auto mymap = new PropertyElement<std::map<std::string, BaseElement *> >(kDTDict);
	mymap->SetElement(elements);
	return mymap;
}


PropertyList *PropertyList::LoadFromXML(std::string data) {
	PropertyList *plist = new PropertyList();
	printf("Loading XML\n");
	Document *doc = Parser::loadXML(data);

	if (doc == NULL) {
		printf("XML PARSER FAILED!");
		return NULL;
	}

	printf("Search doc for '<string>en0</string>'\n");
	DocPath docSearch;
	ITag *en0 = docSearch.findFirst(doc, "string", "Wi-Fi");
	if (en0 != NULL) {
		printf("element found, traversing from node\n");
		doc->traverseFromNode(en0->getParent(), 
		    std::bind(&PropertyList::OnDefinitionTagDataStart,this, std::placeholders::_1, std::placeholders::_2),
    		std::bind(&PropertyList::OnDefinitionTagDataEnd,this, std::placeholders::_1, std::placeholders::_2));
	}

	//Logger::GetLogger("ServiceManager")->Debug("");
	// printf("Parsing XML\n");
	// BaseElement *element = ParseXML(doc->getRoot()->getChildren());
	// if (element != NULL) {
	// 	printf("element: %d\n", element->GetType());
	// }
	// this->elements.push_back(element);
	return plist;
}

void PropertyList::OnDefinitionTagDataStart(ITag *tag, std::list<IAttribute *>&attributes) {
	if (tag->getName() == "key") {
		keyName = tag->getContent();
	} else if (tag->getName() == "string") {
		keyValue = tag->getContent();

		printf("%s = %s\n", keyName.c_str(), keyValue.c_str());
	}
//	printf("%s:%s\n",tag->getName().c_str(), tag->getContent().c_str());
}
void PropertyList::OnDefinitionTagDataEnd(ITag *tag, std::list<IAttribute *>&attributes) {
}


void PropertyList::Traverse() {
	TraverseList(elements);
}
void PropertyList::TraverseList(std::list <BaseElement *> &list) {
	for(auto it : list) {
		BaseElement *element = it;

		printf("%d\n",element->GetType());
		if (element->GetType() == kDTArray) {
			auto dummy = (PropertyElement<std::list<BaseElement *> >*)element;
			std::list<BaseElement *> lista = dummy->GetElement();
			TraverseList(lista);
		}
	}
}





#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CoreFoundation.h>
static void printTypes() {
	printf("string: %d\n", CFStringGetTypeID());
	printf("bool..: %d\n", CFBooleanGetTypeID());
	printf("number: %d\n", CFNumberGetTypeID());
}

//////////////////////////////////
//
// Using the system configuration API, in the documentation start a search for:
//	SCDynamicStoreCreate
// And read the schema introduction article
//
// To mimic the below, use the 'scutil'
// 
// This does,
//	1) retrieve a list of devices from 'State:/Network/Interface'
//  2) For each device grab a set of sub-keys
//  	Link  (State:/Network/Interface/<name>/Link)
//      IPv4
//      IPv6	
//
// Todo: Enhance with information from the Setup key's
//				Setup:/Network/Global/IPv4
// Since it contains some other details
//
//
//

typedef std::function<void(const void *key, const void *value)> OnDictEnumDelegate;

class DetectionContext
{
public:
	DetectionContext();
	virtual ~DetectionContext();

	CFPropertyListRef CopyValues(CFStringRef key);
	void TraverseDict(std::string key, OnDictEnumDelegate handler);

private:
	SCDynamicStoreRef sysconfig;
};

static void DetectionContextDictCBGateway(const void* key, const void* value, void* context);


DetectionContext::DetectionContext() {
	SCDynamicStoreRef sysconfig = SCDynamicStoreCreate(NULL, CFSTR("store"), NULL, NULL);	
}
DetectionContext::~DetectionContext() {

}

CFPropertyListRef DetectionContext::CopyValues(CFStringRef key) {
	return SCDynamicStoreCopyValue (sysconfig, key);
}

void DetectionContext::TraverseDict(std::string key, OnDictEnumDelegate handler) {

 	CFStringRef cfkey = CFStringCreateWithCString (NULL, key.c_str(), kCFStringEncodingASCII);
	CFPropertyListRef data = CopyValues (cfkey);
	if (data != NULL) {
		CFDictionaryRef dict = (CFDictionaryRef)data;
		//dictHandler = handler;
		CFDictionaryApplyFunction(dict, DetectionContextDictCBGateway, &handler);
	}

	CFRelease(cfkey);
}

static void DetectionContextDictCBGateway(const void* key, const void* value, void* context) {
	OnDictEnumDelegate handler = *(OnDictEnumDelegate *)context;
	handler(key, value);

}

class IPAddress {
public:
	void SetAddress(std::string _address) {
		this->address = _address;
	}
	void SetBroadcast(std::string _broadcast) {
		this->broadcast = _broadcast;
	}
	void SetNetmask(std::string _netmask) {
		this->netmask = _netmask;
	}

	std::string Address() { return address; }
	std::string Broadcast() { return broadcast; }
	std::string Netmask() { return netmask; }
private:
	std::string address;
	std::string broadcast;
	std::string netmask;	
};

class Device {
public:
	Device(std::string name);

	std::string Name();
	void DetectConfiguration(DetectionContext *context);
	void Dump();
public:
	void DictCallback(const void* key, const void* value);
	bool HaveIPv4() { return haveIPv4; }
	bool HaveIPv6() { return haveIPv6; }
	int GetNumIPv4Addresses();
	IPAddress *GetIPAddress(IPAddress *outAddress, int idx = 0);

private:
	void LinkFromConfig(const void *key, const void* value);
	void IPv4FromConfig(const void *key, const void* value);
	void IPv6FromConfig(const void *key, const void* value);

private:
	bool haveIPv4;
	bool haveIPv6;
	//std::list<std::string> IPv4Addresses;
	std::list<std::string> IPv4Addresses;
	std::list<std::string> IPv4Broadcasts;
	std::list<std::string> IPv4Netmasks;
	std::list<std::string> IPv6Addresses;	// this is not the best method
	std::string name;
	bool active;
};

class DeviceDetection {
public:
	void DetectDevices();
	void Dump();
public:
	void InterfaceCallback(const void *value);
private:
	void InterfaceDictCallback(const void* key, const void* value);
private:
	std::list<Device *> devices;
	DetectionContext context;
};

// should be different types of devices
// - ethernet
// - wifi
// - usb
// - etc..

Device::Device(std::string _name) {
	this->name = _name;
	active = false;
	haveIPv4 = false;
	haveIPv6 = false;
}
std::string Device::Name() {
	return name;
}

int Device::GetNumIPv4Addresses() {
	return IPv4Addresses.size();
}

IPAddress *Device::GetIPAddress(IPAddress *outAddress, int idx) {
	if (!haveIPv4) return NULL;
	outAddress->SetAddress(IPv4Addresses.front());
	outAddress->SetBroadcast(IPv4Broadcasts.front());
	outAddress->SetNetmask(IPv4Netmasks.front());
	return outAddress;
}


void Device::DetectConfiguration(DetectionContext *context) {
	std::string linkKey = std::string("State:/Network/Interface/")+Name()+std::string("/Link");
	context->TraverseDict(linkKey, std::bind(&Device::LinkFromConfig, this, std::placeholders::_1, std::placeholders::_2));

	// Note: IP confgiuration comes in 3 arrays
	// - IP Address
	// - IP Broadcast
	// - IP Netmask
	// Arrays are traversed one at the time, making this a bit tricky...
	std::string ipv4Key = std::string("State:/Network/Interface/")+Name()+std::string("/IPv4");
	context->TraverseDict(ipv4Key, std::bind(&Device::IPv4FromConfig, this, std::placeholders::_1, std::placeholders::_2));

	std::string ipv6Key = std::string("State:/Network/Interface/")+Name()+std::string("/IPv6");
	context->TraverseDict(ipv6Key, std::bind(&Device::IPv6FromConfig, this, std::placeholders::_1, std::placeholders::_2));
}

void Device::LinkFromConfig(const void *key, const void *value) {
	active = CFBooleanGetValue((CFBooleanRef)value);
}
static void CFArrayValueToList(const void *value, void *context) {
	std::list<std::string> *IPv4Addresses = (std::list<std::string> *)context;

	const char *address = CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingASCII);	
	IPv4Addresses->push_back(std::string(address));

}
void Device::IPv4FromConfig(const void *key, const void *value) {
	CFStringRef strKey = (CFStringRef)key;
	CFArrayRef values = (CFArrayRef)value;

	// called once per array in the dictionary
	// three arrays; IP Address, Broadcast, Netmask
	haveIPv4 = true;
	int num = CFArrayGetCount(values);
	if (CFStringCompare(strKey, CFSTR("Addresses"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {	
		CFArrayApplyFunction(values, CFRangeMake(0,num), CFArrayValueToList, &IPv4Addresses);
	}
	if (CFStringCompare(strKey, CFSTR("BroadcastAddresses"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {	
		CFArrayApplyFunction(values, CFRangeMake(0,num), CFArrayValueToList, &IPv4Broadcasts);
	}
	if (CFStringCompare(strKey, CFSTR("SubnetMasks"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {	
		CFArrayApplyFunction(values, CFRangeMake(0,num), CFArrayValueToList, &IPv4Netmasks);
	}

	// CFShow(strKey);
	// CFShow(values);

}

void Device::IPv6FromConfig(const void *key, const void *value) {
	CFStringRef strKey = (CFStringRef)key;
	CFArrayRef values = (CFArrayRef)value;


	if (CFStringCompare(strKey, CFSTR("Addresses"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
		// called once per array in the dictionary
		// three arrays; IP Address, Broadcast, Netmask
		haveIPv6 = true;
		int num = CFArrayGetCount(values);
		CFArrayApplyFunction(values, CFRangeMake(0,num), CFArrayValueToList, &IPv6Addresses);

	} else {
		// Have numbers and shit here
		CFShow(strKey);
		CFShow(values);
	}


}


void Device::Dump() {
	printf("%s\n", name.c_str());
	printf("  active: %s\n", active?"yes":"no");
	if (haveIPv4) {
		printf("  IPv4\n");
		printf("    Addresses: ");
		for(auto it=IPv4Addresses.begin();it!=IPv4Addresses.end();it++) {
			std::string str = *it;
			printf("%s ", str.c_str());
		}
		printf("\n");
		printf("    BroadcastAddresses: ");
		for(auto it=IPv4Broadcasts.begin();it!=IPv4Broadcasts.end();it++) {
			std::string str = *it;
			printf("%s ", str.c_str());
		}
		printf("\n");
		printf("    SubnetMasks: ");
		for(auto it=IPv4Netmasks.begin();it!=IPv4Netmasks.end();it++) {
			std::string str = *it;
			printf("%s ", str.c_str());
		}
		printf("\n");
	}

	if (haveIPv6) {
		printf("  IPv6 Addresses\n");
		for(auto it=IPv6Addresses.begin();it!=IPv6Addresses.end();it++) {
			std::string str = *it;
			printf("    %s\n", str.c_str());
		}		
	}
}



/// -- Device detection
static void InterfaceArrayCGGateway(const void *value, void *context);

void DeviceDetection::DetectDevices() {

	std::string linkKey = std::string("State:/Network/Interface");
	context.TraverseDict(linkKey, std::bind(&DeviceDetection::InterfaceDictCallback, this, std::placeholders::_1, std::placeholders::_2));	

	for (auto it = devices.begin(); it != devices.end(); it++) {
		Device *device = *it;
		device->DetectConfiguration(&context);
	}

 }


void DeviceDetection::InterfaceDictCallback(const void* key, const void* value) {
	// Get's a list of interface
	CFTypeID keytype = CFGetTypeID(value);
	if (keytype == CFArrayGetTypeID()) {
		CFArrayRef array = (CFArrayRef)value;
		int num = CFArrayGetCount(array);
		CFArrayApplyFunction(array, CFRangeMake(0, num), InterfaceArrayCGGateway, this);
	}
}

static void InterfaceArrayCGGateway(const void *value, void *context) {
	DeviceDetection *dd = (DeviceDetection *)context;
	dd->InterfaceCallback(value);
}

void DeviceDetection::InterfaceCallback(const void *value) {
	const char *mstring = CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingASCII);
	Device *device = new Device(std::string(mstring));
	devices.push_back(device);
}


void DeviceDetection::Dump() {
	for(auto it = devices.begin(); it != devices.end(); it++) {
		Device *device = *it;
		device->Dump();
	}
}


static void printArray(const void *value, void *context) {
	CFTypeID type = CFGetTypeID(value);
	const char *mstring = CFStringGetCStringPtr((CFStringRef)value, kCFStringEncodingASCII);
	//CFShow(value);
	printf("%s\n",mstring);
}

static void printDict (const void* key, const void* value, void* context) {
  CFShow(key);
  CFTypeID keytype = CFGetTypeID(value);
  if (keytype == CFArrayGetTypeID()) {
  	CFArrayRef array = (CFArrayRef)value;
  	int num = CFArrayGetCount(array);
  	CFArrayApplyFunction(array, CFRangeMake(0, num), printArray, NULL);
  } else if (keytype == CFBooleanGetTypeID()) {
  	CFBooleanRef boolref = (CFBooleanRef)value;
  	bool boolval = CFBooleanGetValue(boolref);
  	printf("%s\n",boolval?"true":"false");
  } else {
  	CFShow(value);
  }
}
void enumDict(CFDictionaryRef dict) {
	CFDictionaryApplyFunction(dict, printDict, NULL);
}


void sysconfig() {
	SCDynamicStoreRef store = SCDynamicStoreCreate(NULL, CFSTR("bla"), NULL, NULL);
	CFPropertyListRef data = SCDynamicStoreCopyValue (store, CFSTR("State:/Network/Interface"));
//	CFPropertyListRef data = SCDynamicStoreCopyValue (store, CFSTR("State:/Network/Interface/en0/Link"));
	if (data == NULL) {
		printf("[!] No such key\n");
		return;
	}
	CFTypeID raw_type = CFGetTypeID(data);
	if (raw_type == CFDictionaryGetTypeID() ) {
		CFDictionaryRef dict = (CFDictionaryRef)data;
		printf("Dictionary\n");
		enumDict(dict);
		// int num = CFDictionaryGetCount(dict);
		// printf("Num: %d\n", num);
	} else if (raw_type == CFStringGetTypeID()) {
		printf("string\n");
	} else if (raw_type == CFNumberGetTypeID()) {
		printf("number\n");
	} else {
		printf("[!] Error, unknown CF Type\n");
	}
}

class SysprofilerCapture : public ProcessCallbackBase {
public:
	virtual void OnStdOutData(std::string data) { xml += data; }
	std::string GetData() { return xml; }
private:
	std::string xml;

};
void sysprofiler() {
	SysprofilerCapture capture;
	Process p("system_profiler");

	printf("Capturing 'SPNetworkDataType\n");
	p.AddArgument("-xml");
	p.AddArgument("SPNetworkDataType");
	p.SetCallback(&capture);
	p.ExecuteAndWait();

	//printf("DATA: %s\n", capture.GetData().c_str());

	PropertyList PList;
	PList.LoadFromXML(capture.GetData());
	// printf("TRAVERSE\n");
	// PList.Traverse();

}


int main(int argc, char **argv) {
	printf("hello world!\n");
	sysprofiler();
	DeviceDetection dd;
	dd.DetectDevices();
	printf("dump\n");
	dd.Dump();
	return 0;
}