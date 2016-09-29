#include <stdio.h>
#include <map>
#include <list>
#include <string>
#include <functional>

#include "nxcore.h"

//#include <SafeQueue.h>


using namespace MessageRouter;
using namespace Domain;


void BaseMessageSubscriber::OnMessage(int channelid, int sessionid, const Domain::Message &message) {
	printf("BaseMessageSubscriber::OnMessage(%d,%d)\n",channelid,sessionid);
}

// A write channel is a parameter abstraction for the message router
WriteChannel::WriteChannel(int _id, Router &_router) :
	id(_id), router(_router) {

}


void WriteChannel::Write(int sessionid, const Domain::Message &message) {
	// 1. forward to router, add the channel id
	//printf("WriteChannel::Write(%d)\n",sessionid);
	router.Write(id, sessionid, message);	
}








