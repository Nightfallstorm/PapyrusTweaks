#include "BaseTypeMismatch.h"

namespace
{
	// expands script name to include inheritance
	std::string printScriptInfoInheritance(RE::BSScript::ObjectTypeInfo* scriptInfo)
	{
		// example: CustomQuest (CustomQuest->Quest->Form)
		std::string newScriptName = std::string(scriptInfo->name.c_str()) + " (" + std::string(scriptInfo->name.c_str()) + "->";
		while ((scriptInfo = scriptInfo->GetParent()) != nullptr) {
			newScriptName = newScriptName + scriptInfo->name.c_str() + "->";
		};
		std::string result = newScriptName.substr(0, newScriptName.size() - 2) + ")";
		return result;
	}
}

namespace hooks::tweak::basetypemismatch
{
	// Improve BaseTypeMismatch to distinguish when script not loaded vs script type incorrect
	bool BaseTypeMismatch::thunk(const char* a_buffer, const std::size_t bufferCount, const char* a_format, const char* a_scriptName, const char* a_objectName)
	{
		if (a_scriptName == nullptr || a_objectName == nullptr || a_format == nullptr || a_buffer == nullptr) {
			return func(a_buffer, bufferCount, a_format, a_scriptName, a_objectName);
		}

		if (const auto VM = VM::GetSingleton(); VM->TypeIsValid(a_scriptName)) {
			RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> info;
			VM->GetScriptObjectType1(a_scriptName, info);
			const auto newScriptName = printScriptInfoInheritance(info.get());
			return func(a_buffer, bufferCount, a_format, newScriptName.c_str(), a_objectName);
		}

		auto a_newFormat = "Script %s cannot be bound to %s because the script does not exist or is not currently loaded";
		return func(a_buffer, bufferCount, a_newFormat, a_scriptName, a_objectName);
	}
}