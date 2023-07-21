#include"Settings.h"

MungPlex::Settings::Settings()
{
	_styles.emplace_back("ImGui Default", ColorScheme());
	_styles.push_back(std::make_pair("Dark", []() -> ColorScheme
	{
		ColorScheme dark;
		dark.Background = { 0.1f, 0.1f, 0.1f, 1.0f };
		dark.Text = { 1.0f, 1.0f, 1.0f, 1.0f };
		dark.TextDisabled = { 0.5f, 0.5f, 0.5f, 1.0f };
		dark.ChildBG = { 0.125f, 0.125f, 0.125f, 1.0f };



		dark.Button = { 0.5f, 0.0f, 1.0f, 1.0f };

		return dark;
	}()));

	//_styles.emplace_back("Bright", BRIGHT);
	//_styles.emplace_back("Candy", CANDY);
	//_styles.emplace_back("Pastell", IMGUIDEFAULT);





	bool save = false;
	_generalSettings.Windows.emplace_back("Search");
	_generalSettings.Windows.emplace_back("Cheats");
	_generalSettings.Windows.emplace_back("Process Information");
	_generalSettings.Windows.emplace_back("Settings");

	std::ifstream inFile;
	inFile.open(SettingsJSON);
	std::string jsonstr;

	if (inFile)
	{
		std::string buffer;
		while (std::getline(inFile, buffer))
		{
			jsonstr.append(buffer).append("\n");
		}
	}
	inFile.close();

	try
	{
		auto doc = nlohmann::json::parse(jsonstr);
		auto& settings = doc["Settings"];

		//set general settings
		strcpy_s(_generalSettings.DocumentsPath, settings["General"]["DocumentsPath"].get<std::string>().c_str());
		_generalSettings.Scale = settings["General"]["Scale"].get<float>();
		_generalSettings.DefaultWindowSelect = settings["General"]["DefaultWindowSelect"].get<int>();
		_generalSettings.Style = settings["General"]["ColorScheme"].get<int>();

		if (!std::filesystem::is_directory(_generalSettings.DocumentsPath))
		{
			PWSTR tmp = new wchar_t[512];
			SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &tmp);
			strcpy_s(_generalSettings.DocumentsPath, MorphText::Utf16LE_To_Utf8(tmp).c_str());
			CoTaskMemFree(tmp);
			createDocFolders();
			save = true;
		}

		//set search defaults
		_searchSettings.DefaultCaseSensitive = settings["Search"]["DefaultCaseSensitive"].get<bool>();
		_searchSettings.DefaultColorWheel = settings["Search"]["DefaultColorWheel"].get<bool>();
		_searchSettings.DefaultValuesHex = settings["Search"]["DefaultValuesHex"].get<bool>();
		_searchSettings.DefaultCached = settings["Search"]["DefaultCached"].get<bool>();
		_searchSettings.DefaultAlignment = settings["Search"]["DefaultAlignment"].get<int>();

		if (_searchSettings.DefaultAlignment < 1)
			_searchSettings.DefaultAlignment = 1;
		
		//set cheats defaults
		_cheatsSettings.DefaultCheatList = settings["Cheats"]["DefaultCheatList"].get<bool>();
		_cheatsSettings.DefaultInterval = settings["Cheats"]["DefaultInterval"].get<int>();

		if (_cheatsSettings.DefaultInterval < 1)
			_cheatsSettings.DefaultInterval = 1;
		else if (_cheatsSettings.DefaultInterval > 240)
			_cheatsSettings.DefaultInterval = 240;

		if (save)
			saveSettings();
	}
	catch (const nlohmann::json::parse_error& exception)
	{
		std::cerr << "Failed parsing settings file: " << exception.what() << std::endl;
		return;
	}
}

MungPlex::ColorScheme& MungPlex::Settings::GetColorScheme(const int id)
{
	return GetInstance()._styles[id].second;
}

void MungPlex::Settings::DrawWindow()
{
	ImGui::Begin("Settings");
	
	GetInstance().drawGeneralSettings();
	GetInstance().drawSearchSettings();
	GetInstance().drawCheatSettings();
	ImGui::Separator();

	if (ImGui::Button("Save"))
	{
		GetInstance().saveSettings();
	}

	ImGui::SameLine();
	ImGui::Button("Reset");
	ImGui::End();
}

void MungPlex::Settings::drawGeneralSettings()
{
	const ImVec2 childYX(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f);
	ImGui::BeginChild("child", childYX);
	{
		ImGui::SeparatorText("General Settings");
		SetUpInputText("Documents Path:", _generalSettings.DocumentsPath, 512, 1.0f, 0.2f);
		SetUpSliderFloat("UI Scale:", &_generalSettings.Scale, 0.65f, 2.0f, "%3f", 1.0f, 0.2f);
		SetUpCombo("Default Foreground Window", _generalSettings.Windows, _generalSettings.DefaultWindowSelect, 1.0f, 0.2f);
		SetUpCombo("Color Scheme:", _styles, _generalSettings.Style, 1.0f, 0.2f);

		
	}
	ImGui::EndChild();
}

void MungPlex::Settings::drawSearchSettings()
{
	const ImVec2 childYX(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.5f);
	ImGui::BeginChild("child_SearchSettings", childYX);
	{
		ImGui::SeparatorText("Search Settings");
		ImGui::Checkbox("Case Senstive by default", &_searchSettings.DefaultCaseSensitive);
		ImGui::Checkbox("Color Wheel by default", &_searchSettings.DefaultColorWheel);

		if (SetUpInputInt("Default Alignment:", &_searchSettings.DefaultAlignment, 1, 1, 1.0f, 0.2f))
		{
			if (_searchSettings.DefaultAlignment < 1)
				_searchSettings.DefaultAlignment = 1;
		}

		ImGui::Checkbox("Values are hex by default", &_searchSettings.DefaultValuesHex);
		ImGui::Checkbox("Cached Searches by default", &_searchSettings.DefaultCached);
	}
	ImGui::EndChild();
}

void MungPlex::Settings::drawCheatSettings()
{
	const ImVec2 childYX(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.6f);
	ImGui::BeginChild("child_CheatSettings", childYX);
	{
		ImGui::SeparatorText("Cheats Settings");

		ImGui::Checkbox("Cheat List by default", &_cheatsSettings.DefaultCheatList);

		if (SetUpInputInt("Default Cheat interval", &_cheatsSettings.DefaultInterval, 1, 10, 1.0f, 0.2f))
		{
			if (_cheatsSettings.DefaultInterval < 1)
				_cheatsSettings.DefaultInterval = 1;
			else if (_cheatsSettings.DefaultInterval > 240)
				_cheatsSettings.DefaultInterval = 240;
		}
	}
	ImGui::EndChild();
}

bool MungPlex::Settings::saveSettings()
{
	std::ofstream file(SettingsJSON, std::ios::binary);
	const bool isOpen = file.is_open();

	if (isOpen)
	{
		nlohmann::json jsonData;
		nlohmann::json jsonChunk;

		jsonChunk["DocumentsPath"] = _generalSettings.DocumentsPath;
		if (!std::filesystem::is_directory(_generalSettings.DocumentsPath))
		{
			PWSTR path = new wchar_t[256];
			wcscpy(path, MorphText::Utf8_To_Utf16LE(_generalSettings.DocumentsPath).c_str());

			if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path)))
			{
				strcpy_s(_generalSettings.DocumentsPath, MorphText::Utf16LE_To_Utf8(path).c_str());
			}

			CoTaskMemFree(path);
		}

		createDocFolders();

		jsonChunk["Scale"] = _generalSettings.Scale;
		jsonChunk["DefaultWindowSelect"] = _generalSettings.DefaultWindowSelect;
		jsonChunk["ColorScheme"] = _generalSettings.Style;
		jsonData["Settings"]["General"] = jsonChunk;
		jsonChunk.clear();

		jsonChunk["DefaultCaseSensitive"] = _searchSettings.DefaultCaseSensitive;
		jsonChunk["DefaultColorWheel"] = _searchSettings.DefaultColorWheel;
		jsonChunk["DefaultAlignment"] = _searchSettings.DefaultAlignment;
		jsonChunk["DefaultValuesHex"] = _searchSettings.DefaultValuesHex;
		jsonChunk["DefaultCached"] = _searchSettings.DefaultCached;
		jsonData["Settings"]["Search"] = jsonChunk;
		jsonChunk.clear();

		jsonChunk["DefaultCheatList"] = _cheatsSettings.DefaultCheatList;
		jsonChunk["DefaultInterval"] = _cheatsSettings.DefaultInterval;
		jsonData["Settings"]["Cheats"] = jsonChunk;

		file << "\xEF\xBB\xBF"; //write BOM
		file << jsonData.dump(2);
		file.close();
	}

	return isOpen;
}

void MungPlex::Settings::createDocFolders() const
{
	std::string mungPlexDocsPath = std::string(_generalSettings.DocumentsPath) + "\\MungPlex";

	if (!std::filesystem::is_directory(mungPlexDocsPath))
	{
		std::filesystem::create_directory(mungPlexDocsPath);
		std::filesystem::create_directory(mungPlexDocsPath + "\\Search");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\NES");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\SNES");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\N64");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\GameCube");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Wii");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Wii U");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Switch");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\GameBoy");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\GameBoy Color");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\GameBoy Advance");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\DS");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\3DS");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Master System");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Genesis");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Saturn");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Dreamcast");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\Game Gear");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS1");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS2");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS3");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS4");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS5");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PSP");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PS Vita");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\XBOX");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\XBOX 360");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\XBOX One");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\XBOX Series");
		std::filesystem::create_directory(mungPlexDocsPath + "\\Cheats\\PC");
	}
}

MungPlex::GeneralSettings& MungPlex::Settings::GetGeneralSettings()
{
	return GetInstance()._generalSettings;
}

MungPlex::SearchSettings& MungPlex::Settings::GetSearchSettings()
{
	return GetInstance()._searchSettings;
}

MungPlex::CheatsSettings& MungPlex::Settings::GetCheatsSettings()
{
	return GetInstance()._cheatsSettings;
}
