#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMMNOSOUND

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <SimpleIni.h>
#include <robin_hood.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>

namespace logger = SKSE::log;
namespace string = SKSE::stl::string;

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;

	void asm_replace(std::uintptr_t a_from, std::size_t a_size, std::uintptr_t a_to);

	template <class T>
	void asm_replace(std::uintptr_t a_from)
	{
		asm_replace(a_from, T::size, reinterpret_cast<std::uintptr_t>(T::func));
	}

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, size_t offset, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[offset] };
		T::func = vtbl.write_vfunc(T::idx, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		write_vfunc<F, 0, T>();
	}

	inline std::string as_string(std::string_view a_view)
	{
		return { a_view.data(), a_view.size() };
	}
}

namespace REL
{

	#define Module tempModule

	class tempModule
	{
	public:
#ifdef SKYRIMVR
		static bool IsVR(){ return true; }
		static bool IsAE() { return false; }
		static bool IsSE() { return false; }
#elif SKYRIM_AE
		static bool IsVR(){ return false; }
		static bool IsAE() { return true; }
		static bool IsSE() { return false; }
#else
		static bool IsVR(){ return false; }
		static bool IsAE() { return false; }
		static bool IsSE() { return true; }
#endif
	};

	static int VariantOffset(int SE, int AE, int VR)
	{
#ifdef SKYRIM_VR
		return VR;

#elif SKYRIM_AE
		return AE;
#else
		return SE;
#endif
	}
}

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
