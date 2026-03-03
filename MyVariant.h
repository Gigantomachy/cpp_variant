#pragma once
#include <type_traits>
#include <new>

template <typename First, typename... Ts>
using FirstOf = First;

template <typename T, typename... Ts>
struct type_index {
	static constexpr int value = -1;
};

template <typename T, typename H, typename... R>
struct type_index<T, H, R...> {
	static constexpr int value = std::is_same_v<T, H> ? 0 : (type_index<T, R...>::value == -1 ? -1 : 1 + type_index<T, R...>::value);
};

template <typename T>
constexpr size_t max_size() { return sizeof(T); };

template <typename T, typename S, typename... Rest>
constexpr size_t max_size() {
	return sizeof(T) > max_size<S, Rest...>() ? sizeof(T) : max_size<S, Rest...>();
};

template <typename... Ts>
class MyVariant {
	int tag;
	alignas(Ts...) char buf[max_size<Ts...>()];

	// Us, type can be resolved by compiler using function arguments, not just class type arguments
	template <typename U, typename... Us>
	friend U& get(MyVariant<Us...>& v);

	template <typename U, typename... Us>
	friend bool my_holds_alternative(MyVariant<Us...>& v) noexcept;

	// don't want to shadow Ts... - name it Us
	template <typename Callable, typename... Us>
	friend auto my_visit(Callable&& c, MyVariant<Us...>& v);

public:

	template <typename U>
	MyVariant(U val) {
		tag = type_index<U, Ts...>::value;
		static_assert(type_index<U, Ts...>::value != -1, "");
		new (buf) U(val);
	}

	~MyVariant() {
		using Func = void(*)(char*);

		Func dtors[] = {
			[](char* buf){
				reinterpret_cast<Ts*>(buf)->~Ts();
			}...
		};

		dtors[tag](buf);
	}

	MyVariant(const MyVariant& other) {
		tag = other.tag;

		using Func = void(*)(char*, const char*);

		Func ctors[]= {
			[](char* dst, const char* src) {
				new (dst) Ts(*reinterpret_cast<const Ts*>(src));
			}...
		};

		ctors[tag](buf, other.buf);
	}

	MyVariant(MyVariant&& other) : tag(other.tag) {
		using Func = void(*)(char*, char*);
		Func ctors[] = {
			[](char* dst, char* src) {
				new (dst) Ts(std::move(*reinterpret_cast<Ts*>(src)));
			}...
		};
		ctors[tag](buf, other.buf);
	}

	int index() {
		return tag;
	}
};

template <typename U, typename... Us>
U& get(MyVariant<Us...>& v) {
	int idx = type_index<U, Us...>::value;
	if (v.tag != idx) throw std::bad_variant_access{};
	return *reinterpret_cast<U*>(v.buf);
}

template <typename U, typename... Types>
bool my_holds_alternative(MyVariant<Types...>& v) noexcept {
	bool holdsAlt = type_index<U, Types...>::value == v.tag;
	return holdsAlt;
}

// reference collapsing / forward reference - only happens when used with a deduced template parameter
template <typename Callable, typename... Ts>
auto my_visit(Callable&& c, MyVariant<Ts...>& v) {
//void my_visit(Callable& c, MyVariant<Ts...>& v) {
	//void (*func_array[])(Callable&, char*) = {
	//	[](Callable& c, char* buf){
	//		c(*reinterpret_cast<Ts*>(buf));
	//	}...
	//};

	// Note, this is evaluated once - not expanded.
	// Therefore, all of the lambdas we feed into my_visit must have the same return type
	// This limitation exists for std::visit as well
	using ReturnType = std::invoke_result_t<Callable&, FirstOf<Ts...>&>;

	//using Func = void(*)(Callable&, char*);
	using Func = ReturnType(*)(const void*, char*);

	using CallableBase = std::remove_reference_t<Callable>;

	Func func_array[] = {
		[](const void* callable, char* buf) -> ReturnType {
			return (*reinterpret_cast<const CallableBase*>(callable))(*reinterpret_cast<Ts*>(buf));
		}...
	};

	return func_array[v.tag](static_cast<const void*>(&c), v.buf);
}