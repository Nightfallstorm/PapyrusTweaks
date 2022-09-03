#pragma once
#pragma warning(2: 4235)
#include "RE/S/ScriptFunction.h"
#include <string.h>
/* TODO:
* 1. "Error: Unable to bind script MCMFlaskUtilsScript to FlaskUtilsMCM (7E007E63) because their base types do not match" - 
* show full inheritance of MCMFlaskUtilsScript and show full types of FlaskUtilMCM
* 2. "Error: File "Telengard.esp" does not exist or is not currently loaded." - Allow optional setting to turn this error off
*/
namespace LoggerHooks
{
	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

	// Putting this here avoids a compile error when used in PCH
	template <class T>
	static void write_thunk_call(std::uintptr_t a_src)
	{
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);

		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}
	
	struct ValidationSignaturesHook
	{
		// reimplementation of ValidateSignatureAndFixupNones, but with better logging
		static bool thunk(RE::BSScript::IFunction** a_function, RE::BSScrapArray<RE::BSScript::Variable>* a_varArray, char* a_outString, std::int32_t a_bufferSize)
		{
			if (a_outString != nullptr) {
				*a_outString = 0;
			} else {
				char temp[1024];
				a_outString = temp;
			}
			auto function = *a_function;
			int paramCount = function->GetParamCount();
			if (paramCount == a_varArray->size()) {
				bool mismatchedArgs = false;
				for (int index = 0; index < paramCount; index++) {
					RE::BSFixedString name;
					RE::BSScript::TypeInfo type;
					function->GetParam(index, name, type);
					auto argument = a_varArray->data()[index];
					if (!argument.FitsType(type)) {
						mismatchedArgs = true;
						break;	
					}
				}
				if (mismatchedArgs) {
					char mismatchedArgString[1072];
					std::string mismatchedFormat = "Function %s received incompatible arguments! Received types %s instead!";
					std::string types = "(";
					for (int i = 0; i < a_varArray->size(); i++) {
						types = types + a_varArray->data()[i].GetType().TypeAsString();
						if ((i + 1) < function->GetParamCount()) {
							types = types + ",";
						}
					}
					types = types + ")";
					snprintf(mismatchedArgString, 1024, mismatchedFormat.c_str(),
						ConvertFunctionToString(function).c_str(), types.c_str());
					strcat_s(a_outString, a_bufferSize, mismatchedArgString);
				}
			} else {
				std::string incorrectArgSizeFormat = "Incorrect number of arguments passed to function %s. Expected %u, got %u instead!. ";
				std::string functionName = ConvertFunctionToString(function);
				char incorrectArgString[1072];
				snprintf(incorrectArgString, 1024, incorrectArgSizeFormat.c_str(), functionName.c_str(), paramCount, a_varArray->size());
				strcat_s(a_outString, a_bufferSize, incorrectArgString);
			}
			char discard[1024];
			// Call original validation function to keep behavior the same, but throw away the errors generated from it in favor of ours
			return func(a_function, a_varArray, discard, a_bufferSize);
		}

		static std::string ConvertFunctionToString(RE::BSScript::IFunction* function)
		{
			std::string params = "";
			for (int i = 0; i < function->GetParamCount(); i++) {
				RE::BSFixedString name;
				RE::BSScript::TypeInfo type;
				function->GetParam(i, name, type);
				params = params + type.TypeAsString() + " " + name.c_str();
				if ((i + 1) < function->GetParamCount()) {
					params = params + ",";
				}
			}
			std::string result = std::format(
				"{}.{}({})",
				function->GetObjectTypeName().c_str(),
				function->GetName().c_str(), params
			);

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL_ID(98130, 00000), OFFSET_3(0x63D, 0x0, 0x0) };  // TODO: AE and VR
			write_thunk_call<ValidationSignaturesHook>(target.address());

			logger::info("ValidationSignaturesHook hooked at address " + fmt::format("{:x}", target.address()));
			logger::info("ValidationSignaturesHook at offset " + fmt::format("{:x}", target.offset()));
		}
	};

	static inline void InstallHooks()
	{
		ValidationSignaturesHook::Install();
	}
	
}
