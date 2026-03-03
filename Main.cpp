#include "MyVariant.h"
#include <iostream>

template <typename... Ts>
struct overloads : Ts... {
	using Ts::operator()...;
};

//void printStuff(int i) {
//	std::cout << "My stuff: " << i << std::endl;
//}

int main() {
	int value0 = type_index<int>::value;
	int value1 = type_index<int, int>::value;
	int value2 = type_index<int, float>::value;
	int value3 = type_index<int, float, int>::value;
	int value4 = type_index<int, float, double, int>::value;
	int value5 = type_index<int, float, double, int, char>::value;

	MyVariant<int, float> v = 42;
	MyVariant<int, float, char> w = 'a';

	int i = get<int>(v);
	char j = get<char>(w);

	int y = w.index();

	bool bool1 = my_holds_alternative<int>(v);
	bool bool2 = my_holds_alternative<int>(w);

	//void (*func)(int);
	//func = printStuff;

	const auto visitor = overloads{
		[](int a) -> bool { return a > 0 ? true : false; },
		[](float f) -> bool { return f > 0.0f; },
		[](char c) -> bool { return c > 'b' ? true : false; }
	};
	bool bool3 = my_visit(visitor, v);
	bool bool4 = my_visit(visitor, w);

	my_visit([](auto& val){
		std::cout << "blah blah " << val << std::endl;
	}, v);

	int x = 1;
}