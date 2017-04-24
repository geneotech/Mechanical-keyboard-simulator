#include <array>
#include <Windows.h>

#include "augs/audio/audio_manager.h"
#include "augs/audio/sound_buffer.h"
#include "augs/audio/sound_source.h"

#include "augs/misc/typesafe_sscanf.h"
#include "augs/filesystem/directory.h"
#include "augs/filesystem/file.h"
#include "augs/misc/enum_array.h"
#include "augs/math/vec2.h"

#include "augs/window_framework/translate_windows_enums.h"
#include "augs/templates/container_templates.h"

int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	augs::create_directories("generated/logs/");

	const auto cfg = augs::get_file_lines("config.cfg");

	float volume = -1.f;
	bool enable_hrtf = false;
	vec2 listener_position = { -1.f, -1.f };
	std::string output_device;

	typesafe_sscanf(cfg[0], "volume %x", volume);
	typesafe_sscanf(cfg[1], "enable_hrtf %x", enable_hrtf);
	typesafe_sscanf(cfg[2], "listener_position %x", listener_position);
	typesafe_sscanf(cfg[3], "output_device \"%x\"", output_device);

	size_t current_line = 4;

	while (current_line < cfg.size()) {
		const auto& line = cfg[current_line];

		std::istringstream is(line);
		++current_line;
	}
	
	augs::audio_manager manager(output_device);

	manager.generate_alsoft_ini(
		enable_hrtf,
		1024
	);

	augs::single_sound_buffer buf;
	buf.set_data(augs::get_sound_samples_from_file("keydown1.wav"));

	using namespace augs::window::event::keys;

	std::vector<augs::sound_source> sound_sources;

	MSG msg;

	struct key_settings {
		struct sound_pair {
			augs::single_sound_buffer down_sound;
			augs::single_sound_buffer up_sound;
		};

		std::vector<sound_pair> pairs;
		std::size_t next_pair_to_be_played = 0u;
		bool pressed = false;
	};

	augs::enum_array<key_settings, key> keys;

	while (true) {
		Sleep(1);

		for(int i = 0xFF - 1; i >= 0; --i) {
			const auto id = translate_virtual_key(i);
			
			if (
				id == key::CTRL 
				|| id == key::SHIFT
				|| id == key::ALT
			) {
				continue;
			}

			const bool is_fake_lctrl =
				id == key::LCTRL
				&& keys[key::RALT].pressed
			;

			if (is_fake_lctrl) {
				continue;
			}

			const auto res = GetAsyncKeyState(i);

			if (res == -32767) {
				if(!keys[id].pressed) {
					const auto name = key_to_wstring(id);

					LOG("DOWN " + std::string(name.begin(), name.end()));
					keys[id].pressed = true;

					augs::sound_source src;
					src.bind_buffer(buf);
					src.set_gain(volume);
					src.play();

					sound_sources.emplace_back(std::move(src));
					break;
				}
			}
			else if (res == 0) {
				if(keys[id].pressed) {
					const auto name = key_to_wstring(id);
			
					LOG("UP " + std::string(name.begin(), name.end()));
					keys[id].pressed = false;
			
					augs::sound_source src;
					src.bind_buffer(buf);
					src.set_gain(volume);
					src.play();
			
					sound_sources.emplace_back(std::move(src));
					break;
				}
			}
		}

		erase_remove(
			sound_sources, 
			[](const auto& s){
				return !s.is_playing();
			}
		);
	}

	return 0;
}