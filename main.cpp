#include <array>
#include <Windows.h>

#include "augs/audio/audio_manager.h"
#include "augs/audio/sound_buffer.h"
#include "augs/audio/sound_source.h"

#include "augs/misc/parsing_utils.h"
#include "augs/filesystem/directory.h"
#include "augs/filesystem/file.h"
#include "augs/math/vec2.h"

#include "augs/window_framework/translate_windows_enums.h"

int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	augs::create_directories("generated/logs/");

	const auto cfg = augs::get_file_lines("config.cfg");
	size_t current_line = 0;

	bool enable_hrtf = false;
	vec2 listener_position = { -1.f, -1.f };
	float volume = -1.f;

	auto get_property = [&](auto& into, const auto supposed_property_name){
		std::string actual_property_name;
		std::istringstream is(cfg[current_line]);
		is >> actual_property_name >> into;

		ensure_eq(supposed_property_name, actual_property_name);
		++current_line;
	};

	get_property(volume, "volume");
	get_property(enable_hrtf, "enable_hrtf");
	get_property(listener_position, "listener_position");

	while (current_line < cfg.size()) {
		const auto& line = cfg[current_line];

		std::istringstream is(line);
		++current_line;
	}
	
	augs::audio_manager manager;

	manager.generate_alsoft_ini(
		enable_hrtf,
		1024
	);

	augs::single_sound_buffer buf;
	buf.set_data(augs::get_sound_samples_from_file("keydown1.wav"));

	augs::sound_source src;
	src.bind_buffer(buf);
	src.play();

	struct key_settings {
		struct sound_pair {
			augs::single_sound_buffer down_sound;
			augs::single_sound_buffer up_sound;
		};

		std::vector<sound_pair> pairs;
	};

	std::array<key_settings, 256> keys;

	while (src.is_playing()) {
		Sleep(16);
	}

	return 0;
}