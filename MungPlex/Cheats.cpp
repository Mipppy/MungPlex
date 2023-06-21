#include"Cheats.h"
static float scale = 2.0f;

MungPlex::Cheats::Cheats()
{
	updateConnectionInfo();
	_lua.open_libraries(sol::lib::base,
		sol::lib::string,
		sol::lib::math,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::table,
		sol::lib::io,
		sol::lib::os);

	_lua.set("INT8", INT8);
	_lua.set("INT16", INT16);
	_lua.set("INT32", INT32);
	_lua.set("INT64", INT64);
	_lua.set("FLOAT", FLOAT);
	_lua.set("DOUBLE", DOUBLE);
	_lua.set("BOOL", BOOL);

	_lua.set_function("ReadFromRAM", &readFromRAM);
	_lua.set_function("ReadBool", &readBool);
	_lua.set_function("ReadInt8", &readInt8);
	_lua.set_function("ReadInt16", &readInt16);
	_lua.set_function("ReadInt32", &readInt32);
	_lua.set_function("ReadInt64", &readInt64);
	_lua.set_function("ReadUInt8", &readUInt8);
	_lua.set_function("ReadUInt16", &readUInt16);
	_lua.set_function("ReadUInt32", &readUInt32);
	_lua.set_function("ReadUInt64", &readUInt64);
	_lua.set_function("ReadFloat", &readFloat);
	_lua.set_function("ReadDouble", &readDouble);

	_lua.set_function("WriteToRAM", &writeToRAM);
	_lua.set_function("WriteBool", &writeBool);
	_lua.set_function("WriteInt8", &writeInt8);
	_lua.set_function("WriteInt16", &writeInt16);
	_lua.set_function("WriteInt32", &writeInt32);
	_lua.set_function("WriteInt64", &writeInt64);
	_lua.set_function("WriteFloat", &writeFloat);
	_lua.set_function("WriteDouble", &writeDouble);

	_lua.set_exception_handler(&luaExceptionHandler);

	//put this into the settings class later
	PWSTR path = new wchar_t[256];
	if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path)))
	{
		std::cout << "Cheats: Documents path couldn't be retrieved.\n" << std::endl;
	}
	else
	{
		_documentsPath = std::wstring(path);
		_documentsPath.append(L"\\MungPlex");
		std::wcout << _documentsPath;

		if (!std::filesystem::is_directory(_documentsPath))
		{
			std::filesystem::create_directory(_documentsPath);
			_documentsPath.append(L"\\Cheats");
			std::filesystem::create_directory(_documentsPath);
			_documentsPath.append(L"\\GameCube");
			std::filesystem::create_directory(_documentsPath);
		}
		else
			_documentsPath.append(L"\\Cheats\\GameCube");
	}

	CoTaskMemFree(path);

	_currentGameID = std::wstring(L"GFZE01");
	//SetGameID("GFZE01");
	initCheatFile();
}

void MungPlex::Cheats::DrawWindow()
{
	ImGui::Begin("Cheats");
	GetInstance().DrawCheatList();
	ImGui::SameLine();
	GetInstance().DrawCheatInformation(); 
	GetInstance().DrawControl();
	ImGui::End();
}

void MungPlex::Cheats::DrawCheatList()
{
	float groupWidth = ImGui::GetContentRegionAvail().x / scale;
	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(groupWidth);
		ImGui::SeparatorText("Cheat List");

		

	}
	ImGui::EndGroup();
}

void MungPlex::Cheats::DrawCheatInformation()
{
	float groupWidth = ImGui::GetContentRegionAvail().x / scale;

	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(groupWidth);
		ImGui::SeparatorText("Cheat Information");

		static char buf[256];
		strcpy(buf, "test\0");
		ImGui::InputText("Title", buf, IM_ARRAYSIZE(buf));
		strcpy(buf, "test\0");
		ImGui::InputText("Hacker(s)", buf, IM_ARRAYSIZE(buf));

		_textCheatLua;
		_textCheatDescription;
		static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
		ImGui::InputTextMultiline("Lua Cheat", _textCheatLua, IM_ARRAYSIZE(_textCheatLua), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);
		ImGui::InputTextMultiline("Description", _textCheatDescription, IM_ARRAYSIZE(_textCheatDescription), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

		if (ImGui::Button("Save to list"))
		{

		}
	}
	ImGui::EndGroup();
}

void MungPlex::Cheats::DrawControl()
{
	float groupWidth = ImGui::GetContentRegionAvail().x / scale;

	ImGui::BeginGroup();
	{
		ImGui::PushItemWidth(groupWidth);
		ImGui::SeparatorText("Cheat Control");

		static char buf[256];
		
		if (ImGui::Button(_executeCheats ? "Terminate Cheats" : "Apply Cheats"))
		{
			if (_cheatError)
			{
				_cheatThread.join();
				_cheatError = false;
			}

			if (_executeCheats)
			{
				_executeCheats = false;
				_cheatThread.join();
			}
			else
			{
				updateConnectionInfo();
				_processInfo = Xertz::SystemInfo::GetProcessInfo(_pid);
				_executeCheats = true;
				_cheatThread = std::thread(&Cheats::cheatRoutine, this);
			}
		}

		ImGui::SliderInt("Interval", &_perSecond, 1, 240);

		if (ImGui::RadioButton("Cheat List", _cheatList))
			_cheatList = true;

		if (ImGui::RadioButton("Text Cheat", !_cheatList))
			_cheatList = false;
	}
	ImGui::EndGroup();
}

void MungPlex::Cheats::cheatRoutine()
{
	_cheatError = false;
	sol::protected_function_result pfr = _lua.safe_script(_textCheatLua, sol::script_pass_on_error);

	if (!pfr.valid())
	{
		sol_c_assert(!pfr.valid());
		sol::error err = pfr;
		std::cout << err.what() << std::endl;
		_executeCheats = false;
		_cheatError = true;
		return;
	}

	while (_executeCheats)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / _perSecond));
		_lua.safe_script(_textCheatLua, sol::script_pass_on_error);
	}
}

void MungPlex::Cheats::SetGameID(const char* ID)
{
	GetInstance()._currentGameID = MorphText::Utf8_To_Utf16LE(ID);
}

void MungPlex::Cheats::initCheatFile()
{
	_currentCheatFile = _documentsPath + L"\\" + _currentGameID + L".json";
	if (!std::filesystem::exists(_currentCheatFile))
	{
		std::ofstream file(_currentCheatFile, std::ios::binary);

		if (file.is_open())
		{
			file << "\xEF\xBB\xBF"; //write BOM
			file << _placeholderCheatFile;
			file.close();
		}

	}


	
}

int MungPlex::Cheats::luaExceptionHandler(lua_State* L, sol::optional<const std::exception&> exception, sol::string_view description)
{
	std::cout << "An exception occurred";
	if (exception)
	{
		std::cout << "\nError: ";
		const std::exception& ex = *exception;
		std::cout << ex.what() << std::endl;
	}
	else
	{
		std::cout << "\nDetails: ";
		std::cout.write(description.data(),
			static_cast<std::streamsize>(description.size()));
		std::cout << std::endl;
	}

	return sol::stack::push(L, description);
}

int MungPlex::Cheats::getRangeIndex(uint64_t address)
{
	int rangeIndex = -1;

	for (int i = 0; i < _regions.size(); ++i)
	{
		if (address >= _regions[i].Base && address < _regions[i].Base + _regions[i].Size)
			return i;
	}

	return rangeIndex;
}