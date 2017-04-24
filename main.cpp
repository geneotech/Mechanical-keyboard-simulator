#include <array>
#include <random>

#include <Windows.h>

#include "augs/audio/audio_manager.h"
#include "augs/audio/sound_buffer.h"
#include "augs/audio/sound_source.h"

#include "augs/misc/typesafe_sscanf.h"
#include "augs/filesystem/directory.h"
#include "augs/filesystem/file.h"
#include "augs/misc/enum_array.h"
#include "augs/math/vec2.h"
#include "augs/math/si_scaling.h"

#include "augs/window_framework/translate_windows_enums.h"
#include "augs/templates/container_templates.h"

#include <AL/al.h>
#include <AL/alc.h>

struct vec3 {
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
};

std::istream& operator>>(std::istream& out, vec3& x) {
	std::string chunk;
	out >> chunk;
	typesafe_sscanf(chunk, "(%x;%x;%x)", x.x, x.y, x.z);
	return out;
}

struct key_settings {
	struct sound_pair {
		std::string down_sound_path;
		std::string up_sound_path;
	};

	vec3 position;
	std::vector<sound_pair> pairs;
	std::size_t next_pair_to_be_played = 0u;
	bool pressed = false;
};

using namespace augs::window::event::keys;

int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	augs::create_directories("generated/logs/");

	const auto cfg = augs::get_file_lines("config.cfg");

	std::random_device rd;
	std::mt19937 rng(rd());

	/* CONFIG SETTINGS */
	float volume = -1.f;
	bool enable_hrtf = false;
	bool mix_all_sounds_to_mono = false;
	vec3 listener_position = { -1.f, -1.f, -1.f };
	std::array<float, 6> listener_orientation = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
	std::string output_device;
	/* END OF CONFIG SETTINGS */

	typesafe_sscanf(cfg[0], "volume %x", volume);
	typesafe_sscanf(cfg[1], "enable_hrtf %x", enable_hrtf);
	typesafe_sscanf(cfg[2], "mix_all_sounds_to_mono %x", mix_all_sounds_to_mono);
	typesafe_sscanf(cfg[3], "listener_position %x", listener_position);
	
	typesafe_sscanf(cfg[4], "listener_orientation (%x;%x;%x;%x;%x;%x)", 
		listener_orientation[0], 
		listener_orientation[1], 
		listener_orientation[2], 
		listener_orientation[3], 
		listener_orientation[4], 
		listener_orientation[5]
	);

	typesafe_sscanf(cfg[5], "output_device \"%x\"", output_device);

	ensure_eq("keys:", cfg[6]);
	
	std::size_t current_line = 7;

	augs::audio_manager::generate_alsoft_ini(
		enable_hrtf,
		1024
	);

	augs::audio_manager manager(output_device);

	augs::set_listener_velocity(si_scaling(), {0.f, 0.f});
	augs::set_listener_orientation(listener_orientation);
	alListener3f(AL_POSITION, listener_position.x, listener_position.y, listener_position.z);

	std::unordered_map<std::string, augs::single_sound_buffer> sound_buffers;
	
	augs::enum_array<key_settings, key> keys;

	const auto make_buffer = [&sound_buffers, mix_all_sounds_to_mono](const std::string path){
		if (sound_buffers.find(path) != sound_buffers.end()) {
			return;
		}

		auto& buf = sound_buffers[path];

		const auto samples = augs::get_sound_samples_from_file(path);

		if (samples.channels > 1 && mix_all_sounds_to_mono) {
			buf.set_data(mix_stereo_to_mono(samples));
		}
		else {
			buf.set_data(samples);
		}
	};

	while (current_line < cfg.size()) {
		const auto& line = cfg[current_line];

		key_settings next_key;
		std::string key_name;
		std::string sound_pairs;

		typesafe_sscanf(
			line, 
			"name=\"%x\" position=%x pairs: %x", 
			key_name, 
			next_key.position, 
			sound_pairs
		);

		std::istringstream is(sound_pairs);
		
		std::string down_sound_path;
		std::string up_sound_path;
		std::string separator;

		while (std::getline(is, separator, '\"')) {
			std::getline(is, down_sound_path, '\"');
			std::getline(is, separator, '\"');
			std::getline(is, up_sound_path, '\"');

			make_buffer(down_sound_path);
			make_buffer(up_sound_path);

			next_key.pairs.push_back({
				down_sound_path, 
				up_sound_path
			});
		} 

		const auto key_id = wstring_to_key(std::wstring(key_name.begin(), key_name.end()));
		std::shuffle(next_key.pairs.begin(), next_key.pairs.end(), rng);
		keys[key_id] = next_key;
		
		++current_line;
	}


	std::vector<augs::sound_source> sound_sources;

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
			
			auto& subject_key = keys[id];

			if (res == -32767) {
				if(!subject_key.pressed) {
					subject_key.pressed = true;
					
					if (subject_key.pairs.empty()) {
						continue;
					}

					const auto& pair = subject_key.pairs[subject_key.next_pair_to_be_played];

					const auto name = key_to_wstring(id);
					
					LOG("DOWN " + std::string(name.begin(), name.end()));

					augs::sound_source src;
					src.bind_buffer(sound_buffers[pair.down_sound_path]);
					src.set_gain(volume);
					
					alSource3f(
						src.get_id(), 
						AL_POSITION, 
						subject_key.position.x, 
						subject_key.position.y, 
						subject_key.position.z
					);

					src.play();

					sound_sources.emplace_back(std::move(src));
					break;
				}
			}
			else if (res == 0) {
				if(subject_key.pressed) {
					subject_key.pressed = false;

					if (subject_key.pairs.empty()) {
						continue;
					}

					const auto& pair = subject_key.pairs[subject_key.next_pair_to_be_played];

					//const auto name = key_to_wstring(id);
					//
					//LOG("UP " + std::string(name.begin(), name.end()));
			
					augs::sound_source src;
					src.bind_buffer(sound_buffers[pair.up_sound_path]);
					src.set_gain(volume);

					alSource3f(
						src.get_id(), 
						AL_POSITION, 
						subject_key.position.x, 
						subject_key.position.y, 
						subject_key.position.z
					);

					src.play();

					sound_sources.emplace_back(std::move(src));
					
					++subject_key.next_pair_to_be_played;
					
					if(subject_key.next_pair_to_be_played == subject_key.pairs.size()) {
						std::shuffle(subject_key.pairs.begin(), subject_key.pairs.end(), rng);
						subject_key.next_pair_to_be_played = 0u;
					}

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