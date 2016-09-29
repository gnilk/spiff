#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <functional>

#include "nxcore.h"

using namespace MessageRouter;
using namespace Domain;

// ------------------------
// Message Router
//

Router::Router() {

}

Router::~Router() {

}

void Router::InitalizeRouter() {
	//
	// 1. Register this instance with the system
	//	
}


void Router::UnsubscribeFromChannel(int channelid, IMessageSubscriber *subscriber) {
	//
	// 1. check if channel is 'MSG_CHANNEL_ANY'
	//   1.1 remove subsctiber from all channels
	// 2. remove the subscriber from the channel subscriber list
	//	
}


void Router::SubscribeToChannel(int channelid, IMessageSubscriber *subscriber) {
	channelSubscribers.insert(std::make_pair(channelid, subscriber));
}

WriteChannel *Router::GetWriteChannel(int channelid) {

	if (channelid == MSG_CHANNEL_ANY) {
		// TODO: Throw exception here!  - THIS IS NOT ALLOWED
		return NULL;
	}

	WriteChannel *writer;
	auto it = channelWriters.find(channelid);
	if (it == channelWriters.end()) {
		writer = new WriteChannel(channelid, *this);
		channelWriters.insert(std::make_pair(channelid, writer));
	} else {
		writer = it->second;
	}
	return writer;
}

void Router::Write(int senderchannel, int sessionid, const Domain::Message &message) {

	// not allowed
	if (senderchannel == MSG_CHANNEL_ANY) return;

	printf("Router::Write(%d,%d)\n",senderchannel,sessionid);

	// 1. call all subscribers of that channel id
	auto range = channelSubscribers.equal_range(senderchannel);
	for (auto it = range.first; it != range.second; it++) {
		printf("Router::Write, it->first = %d\n",it->first);
		it->second->OnMessage(senderchannel, sessionid, message);		
	}
	
	WriteToAny(senderchannel, sessionid, message);
}

void Router::WriteToAny(int senderchannel, int sessionid, const Domain::Message &message) {

	// not allowed
	if (senderchannel == MSG_CHANNEL_ANY) return;

	// 1. call all subscribers 
	auto range = channelSubscribers.equal_range(MSG_CHANNEL_ANY);
	for (auto it = range.first; it != range.second; it++) {
		printf("Router::WriteAny, it->first = %d\n",it->first);
		it->second->OnMessage(senderchannel, sessionid, message);		
	}
}
