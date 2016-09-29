#include <stdio.h>
#include <map>
#include <vector>
#include <functional>
#include <utility> // for std::forward

class Dummy {
private:
	int a;
public:
	Dummy(int _a);
	void Print();
};
Dummy::Dummy(int _a) :
	a(_a) {
		printf("A: %d\n",a);
}

void Dummy::Print() {
	printf("%d",a);
}

static std::multimap<int, Dummy *> subscribers;

void AddSub(int id, Dummy *dummy) {
	subscribers.insert(std::make_pair(id, dummy));
}

void Invoke() {
	for(auto it = subscribers.begin(); it != subscribers.end(); it++) {
		printf("%d: ",(int)it->first);
		it->second->Print();
		printf("\n");
	}
}

void InvokeFind(int id) {
	auto range = subscribers.equal_range(id);
	for(auto it = range.first; it != range.second; it++) {
		printf("%d: ",(int)it->first);
		it->second->Print();		
		printf("\n");		
	}

	// auto it = subscribers.find(id);
	// while (it != subscribers.end()) {
	// 	printf("%d: ",(int)it->first);
	// 	it->second->Print();		
	// 	printf("\n");
	// 	it++;
	// }
}

void callback(int i) {
	printf("callback %d\n",i);
}
void testcxxfunc() {
	std::function<void(int)> func = callback;
	func(10);
}


int main(int argc, char **argv) {

	printf("test xxx\n");
	testcxxfunc();
	return 0;

	Dummy d1(1);
	Dummy d2(2);

	AddSub(10, &d1);
	AddSub(20, &d2);

	Invoke();
	printf("find\n");
	InvokeFind(20);

	return 0;
}