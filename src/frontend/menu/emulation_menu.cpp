#include "emulation_menu.hpp"
#include "../config.hpp"
#include <imgui.h>
#include <nfd.hpp>
namespace NESterpiece
{
	void EmulationMenu::open()
	{
		auto &config = Configuration::get();
	}

	void EmulationMenu::draw()
	{
		using namespace ImGui;
		auto &config = Configuration::get();

		auto cb = [](ImGuiInputTextCallbackData *data) -> int32_t
		{
			if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				auto str = reinterpret_cast<std::vector<char> *>(data->UserData);
				str->resize(data->BufSize);
				data->Buf = str->data();
			}
			return 0;
		};

		if (BeginTable("Emulation Toggles", 2, ImGuiTableFlags_SizingFixedFit))
		{
			TableNextColumn();
			Checkbox("Allow RAM saving", &config.emulation.allow_sram_saving);
			TableNextColumn();
			SetNextItemWidth(100);
			InputInt("Save Interval (Seconds)", reinterpret_cast<int32_t *>(&config.emulation.sram_save_interval), 5, 100, config.emulation.allow_sram_saving ? 0 : ImGuiInputTextFlags_ReadOnly);
			TableNextRow();
			TableNextColumn();
			EndTable();
		}
	}
}