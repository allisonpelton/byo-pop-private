// Binding C++ classes and their methods to Lua, as the original's LuaLib::MethodWrapper<Class, method> and
// RawMethodWrapper<Class> did (358 of them). A class is registered with a chain of calls:
//
//     LuaLib::Class<ArrowHint>("ArrowHint").Base<Widget>()
//         .Method<&ArrowHint::AddText>("AddText")
//         .Method<&ArrowHint::SetDir>("SetDir");
//
// Arguments are checked from the first on, as the wrappers did: ints accept ints, numbers (truncated) and int
// strings ("int expected"), booleans only booleans ("boolean expected"), strings strings and numbers, objects
// their class or a derived one, or nil (NULL). Results: ints as ints, floats and doubles as numbers, strings
// (NULL is nil), objects (NULL is nil), and LuaMultiRet for methods that push their own results.
#pragma once

#include <array>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "LuaSupport.h"

namespace LuaLib {

// The class info of a bound C++ class (set by Class<T>)
template <class T>
struct ClassOf {
	static ClassInfo* info;
};
template <class T>
ClassInfo* ClassOf<T>::info = nullptr;

// A method that pushes its own results returns how many
struct LuaMultiRet {
	int n;
};

// An argument passed as it is: its stack index (the original's `lua_TObject const*')
struct LuaValue {
	int idx;
};

inline int CheckInt(lua_State* L, int i)
{
	return (int)luaL_checkint(L, i);
}

inline bool CheckBool(lua_State* L, int i)
{
	if (lua_type(L, i) != LUA_TBOOLEAN)
		luaL_typerror(L, i, "boolean");
	return lua_toboolean(L, i) != 0;
}

// ---- arguments ----

template <class T, class Enable = void>
struct Arg;

template <class T>
struct Arg<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
	static T Get(lua_State* L, int i) { return (T)CheckInt(L, i); }
};
template <>
struct Arg<bool> {
	static bool Get(lua_State* L, int i) { return CheckBool(L, i); }
};
template <class T>
struct Arg<T, std::enable_if_t<std::is_floating_point_v<T>>> {
	static T Get(lua_State* L, int i) { return (T)luaL_checknumber(L, i); }
};
template <>
struct Arg<std::string> {
	static std::string Get(lua_State* L, int i) { return std::string(luaL_checkstring(L, i), lua_strlen(L, i)); }
};
template <>
struct Arg<const char*> {
	static const char* Get(lua_State* L, int i) { return luaL_checkstring(L, i); }
};
template <>
struct Arg<lua_State*> {
	static lua_State* Get(lua_State* L, int) { return L; }
};
template <>
struct Arg<LuaValue> {
	static LuaValue Get(lua_State*, int i) { return LuaValue{i}; }
};
template <class T>
struct Arg<T*, std::enable_if_t<std::is_class_v<T>>> {
	static T* Get(lua_State*, int i) { return (T*)gLuaSupport->CheckObject(i, ClassOf<std::remove_cv_t<T>>::info); }
};

// lua_State* takes no stack slot
template <class T>
constexpr int Slots()
{
	return std::is_same_v<std::decay_t<T>, lua_State*> ? 0 : 1;
}

// ---- results ----

template <class T, class Enable = void>
struct Ret;

template <class T>
struct Ret<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
	static int Push(lua_State* L, T v)
	{
		lua_pushint(L, (lua_Int)v);
		return 1;
	}
};
template <>
struct Ret<bool> {
	static int Push(lua_State* L, bool v)
	{
		lua_pushboolean(L, v);
		return 1;
	}
};
template <class T>
struct Ret<T, std::enable_if_t<std::is_floating_point_v<T>>> {
	static int Push(lua_State* L, T v)
	{
		lua_pushnumber(L, (lua_Number)v);
		return 1;
	}
};
template <>
struct Ret<std::string> {
	static int Push(lua_State* L, const std::string& v)
	{
		lua_pushlstring(L, v.data(), v.size());
		return 1;
	}
};
template <>
struct Ret<const char*> {
	static int Push(lua_State* L, const char* v)
	{
		if (v == nullptr)
			lua_pushnil(L);
		else
			lua_pushstring(L, v);
		return 1;
	}
};
template <>
struct Ret<LuaMultiRet> {
	static int Push(lua_State*, LuaMultiRet v) { return v.n; }
};
template <class T>
struct Ret<T*, std::enable_if_t<std::is_class_v<T>>> {
	static int Push(lua_State*, T* v)
	{
		gLuaSupport->PushObject(ClassOf<std::remove_cv_t<T>>::info, (void*)v);
		return 1;
	}
};

// ---- calling ----

// The stack index of each argument (lua_State* takes none)
template <class... A, size_t... I>
constexpr auto ArgIndices(std::index_sequence<I...>)
{
	constexpr int slots[] = {0, Slots<A>()...};
	std::array<int, sizeof...(A) + 1> idx{};
	int n = 1;
	for (size_t k = 0; k < sizeof...(A); k++) {
		idx[k] = n;
		n += slots[k + 1];
	}
	return idx;
}

template <class R, class F, class... A, size_t... I>
int Invoke(lua_State* L, F f, std::index_sequence<I...> seq)
{
	constexpr auto idx = ArgIndices<A...>(seq);
	// a braced list evaluates (and checks) the arguments from the first on
	std::tuple<std::decay_t<A>...> args{Arg<std::decay_t<A>>::Get(L, idx[I])...};
	if constexpr (std::is_void_v<R>) {
		std::apply(f, std::move(args));
		return 0;
	} else
		return Ret<std::decay_t<R>>::Push(L, std::apply(f, std::move(args)));
}

template <auto M>
struct MethodThunk;

template <class C, class R, class... A, R (C::*M)(A...)>
struct MethodThunk<M> {
	using Class = C;
	static int Call(lua_State* L, void* self)
	{
		C* obj = (C*)self;
		auto f = [obj](auto&&... a) -> R { return (obj->*M)(std::forward<decltype(a)>(a)...); };
		return Invoke<R, decltype(f), A...>(L, f, std::index_sequence_for<A...>{});
	}
};

template <class C, class R, class... A, R (C::*M)(A...) const>
struct MethodThunk<M> {
	using Class = C;
	static int Call(lua_State* L, void* self)
	{
		const C* obj = (const C*)self;
		auto f = [obj](auto&&... a) -> R { return (obj->*M)(std::forward<decltype(a)>(a)...); };
		return Invoke<R, decltype(f), A...>(L, f, std::index_sequence_for<A...>{});
	}
};

// A plain function taking the object first: R f(C*, A...)
template <auto F>
struct FunctionThunk;

template <class C, class R, class... A, R (*F)(C*, A...)>
struct FunctionThunk<F> {
	static int Call(lua_State* L, void* self)
	{
		C* obj = (C*)self;
		auto f = [obj](auto&&... a) -> R { return F(obj, std::forward<decltype(a)>(a)...); };
		return Invoke<R, decltype(f), A...>(L, f, std::index_sequence_for<A...>{});
	}
};

// ---- registering ----

template <class C>
class Class {
public:
	// `deletable': Lua may own objects of the class (and then deletes them when it collects them)
	explicit Class(const std::string& name, bool deletable = true)
	{
		Destroy destroy = nullptr;
		if constexpr (std::is_destructible_v<C>)
			if (deletable)
				destroy = [](void* p) { delete (C*)p; };
		mInfo = gLuaSupport->CreateClass(name, destroy);
		ClassOf<C>::info = mInfo;
	}

	template <class B>
	Class& Base()
	{
		static_assert(std::is_base_of_v<B, C>, "not a base class");
		gLuaSupport->SetBaseClass(mInfo, ClassOf<B>::info, [](void* p) -> void* { return static_cast<B*>((C*)p); });
		return *this;
	}

	// A member function of C or of a base of C
	template <auto M>
	Class& Method(const std::string& name)
	{
		static_assert(std::is_base_of_v<typename MethodThunk<M>::Class, C>, "not a method of the class");
		gLuaSupport->AddMethod(mInfo, name, &Self<M>);
		return *this;
	}

	// A function R f(C*, args...) as a method
	template <auto F>
	Class& Function(const std::string& name)
	{
		gLuaSupport->AddMethod(mInfo, name, &FunctionThunk<F>::Call);
		return *this;
	}

	// A function int f(C*, lua_State*) that reads its own arguments (from index 1; self is removed) and pushes
	// its own results, as the original's raw wrappers did
	template <int (*F)(C*, lua_State*)>
	Class& Raw(const std::string& name)
	{
		gLuaSupport->AddMethod(mInfo, name, [](lua_State* L, void* self) { return F((C*)self, L); });
		return *this;
	}

	ClassInfo* Info() const { return mInfo; }

private:
	// converts self to the class that declares the method
	template <auto M>
	static int Self(lua_State* L, void* self)
	{
		using D = typename MethodThunk<M>::Class;
		return MethodThunk<M>::Call(L, static_cast<D*>((C*)self));
	}

	ClassInfo* mInfo;
};

}  // namespace LuaLib
