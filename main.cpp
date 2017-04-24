#include <array>
#include <random>
#include <chrono>
#include <thread>

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

#define LOG_PRESSES 0

using namespace augs::window::event::keys;

struct vec3 {
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	vec3 operator+(const vec3 b) const {
		return { x + b.x, y + b.y, z + b.z };
	}

	vec3& operator*=(float s) {
		*this = vec3{ x * s, y * s, z * s };
		return *this;
	}
};

std::istream& operator>>(std::istream& out, vec3& x) {
	std::string chunk;
	out >> chunk;
	typesafe_sscanf(chunk, "(%x;%x;%x)", x.x, x.y, x.z);
	return out;
}

std::wostream& operator<<(std::wostream& out, const vec3& x) {
	out << typesafe_sprintf(L"(%x;%x;%x)", x.x, x.y, x.z);
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

augs::enum_array<key_settings, key> keys;

struct key_metric {
	vec3 lt_pos;
	vec3 center_pos;
	vec3 size;
};

augs::enum_array<key_metric, key> metrics;

void vertical_scanline(vec3 current_pos) {

}

template <class... A>
void vertical_scanline(vec3 current_pos, vec3 arg, A... args) {
	current_pos.y += arg.y;
	vertical_scanline(current_pos, args...);
}

template <class... A>
void vertical_scanline(vec3 current_pos, key arg, A... args) {
	metrics[arg].lt_pos = current_pos;
	current_pos.y += metrics[arg].size.y;

	vertical_scanline(current_pos, args...);
}


void horizontal_scanline(vec3 current_pos) {

}

template <class... A>
void horizontal_scanline(vec3 current_pos, vec3 arg, A... args) {
	current_pos.x += arg.x;
	horizontal_scanline(current_pos, args...);
}

template <class... A>
void horizontal_scanline(vec3 current_pos, key arg, A... args) {
	metrics[arg].lt_pos = current_pos;
	current_pos.x += metrics[arg].size.x;

	horizontal_scanline(current_pos, args...);
}

void set_default_keyboard_metrics() {
	auto pos = [](auto id) { return metrics[id].center_pos; };
	auto lt_pos = [](auto id) { return metrics[id].lt_pos; };
	auto sz = [](auto id) { return metrics[id].size; };

	vec3 standard_sz = { 45, 45 };
	vec3 ctrl_alt_win_fn_lshift_sz = { 55, 45 };
	vec3 space_sz = { 55, 45 };
	vec3 caps_sz = { 80, 45 };
	vec3 tab_sz = { 65, 45 };
	vec3 backspace_sz = { 90, 45 };
	vec3 add_sz = { 45, 90 };
	vec3 enter_sz = { 95, 45 };
	vec3 rshift_sz = { 120, 45 };
	vec3 lshift_sz = { 120, 45 };
	vec3 backslash_sz = { 65, 45 };
	
	vec3 standard_sep = { 20, 45 } ;
	vec3 f4_f5_sep = { 20, 45 } ;
	vec3 esc_tilde_sep = { 45, 20 };

	for (auto& k : metrics) {
		k.size = standard_sz;
	}

	sz(key::TAB) = tab_sz;
	sz(key::SPACE) = space_sz;
	sz(key::CAPSLOCK) = caps_sz;
	sz(key::BACKSPACE) = backspace_sz;
	sz(key::ADD) = add_sz;
	sz(key::ENTER) = enter_sz;

	sz(key::LCTRL) = ctrl_alt_win_fn_lshift_sz;
	sz(key::RCTRL) = ctrl_alt_win_fn_lshift_sz;
	sz(key::LALT) = ctrl_alt_win_fn_lshift_sz;
	sz(key::RALT) = ctrl_alt_win_fn_lshift_sz;
	sz(key::LWIN) = ctrl_alt_win_fn_lshift_sz;
	sz(key::RWIN) = ctrl_alt_win_fn_lshift_sz;
	
	sz(key::BACKSLASH) = backslash_sz;
	sz(key::LSHIFT) = lshift_sz;
	sz(key::RSHIFT) = rshift_sz;

	vertical_scanline(
		{ 0.f, 0.f, 0.f },
		key::ESC,
		esc_tilde_sep,
		key::DASH,
		key::TAB,
		key::CAPSLOCK,
		key::LSHIFT,
		key::LCTRL
	);

	horizontal_scanline(
		lt_pos(key::ESC),
		key::ESC,
		standard_sep,
		key::F1,
		key::F2,
		key::F3,
		key::F4,
		f4_f5_sep,
		key::F5,
		key::F6,
		key::F7,
		key::F8,
		standard_sep,
		key::F9,
		key::F10,
		key::F11,
		key::F12,
		f4_f5_sep,
		key::PRINTSCREEN,
		key::SCROLL,
		key::PAUSE
	);

	horizontal_scanline(
		lt_pos(key::DASH),
		key::DASH,
		key::_1,
		key::_2,
		key::_3,
		key::_4,
		key::_5,
		key::_6,
		key::_7,
		key::_8,
		key::_9,
		key::_0,
		key::MINUS,
		key::PLUS,
		key::BACKSPACE,
		standard_sep,
		key::INSERT,
		key::HOME,
		key::PAGEUP,
		standard_sep,
		key::NUMLOCK,
		standard_sep,
		key::MULTIPLY,
		key::SUBTRACT
	);

	horizontal_scanline(
		lt_pos(key::TAB),
		key::TAB,
		key::Q,
		key::W,
		key::E,
		key::R,
		key::T,
		key::Y,
		key::U,
		key::I,
		key::O,
		key::P,
		key::OPEN_SQUARE_BRACKET,
		key::CLOSE_SQUARE_BRACKET,
		key::BACKSLASH,
		f4_f5_sep,
		key::DEL,
		key::END,
		key::PAGEDOWN,
		f4_f5_sep,
		key::NUMPAD7,
		key::NUMPAD8,
		key::NUMPAD9,
		key::ADD
	);

	horizontal_scanline(
		lt_pos(key::CAPSLOCK),
		key::CAPSLOCK,
		key::A,
		key::S,
		key::D,
		key::F,
		key::G,
		key::H,
		key::J,
		key::K,
		key::L,
		key::SEMICOLON,
		key::APOSTROPHE,
		key::ENTER,
		key::CLOSE_SQUARE_BRACKET,
		key::ENTER,
		f4_f5_sep,
		standard_sep,
		standard_sep,
		standard_sep,
		f4_f5_sep,
		key::NUMPAD4,
		key::NUMPAD5,
		key::NUMPAD6
	);

	horizontal_scanline(
		lt_pos(key::LSHIFT),
		key::LSHIFT,
		key::Z,
		key::X,
		key::C,
		key::V,
		key::B,
		key::N,
		key::M,
		key::COMMA,
		key::PERIOD,
		key::SLASH,
		key::RSHIFT,
		f4_f5_sep,
		standard_sep,
		key::UP,
		standard_sep,
		f4_f5_sep,
		key::NUMPAD1,
		key::NUMPAD2,
		key::NUMPAD3
	);

	horizontal_scanline(
		lt_pos(key::LCTRL),
		key::LWIN,
		key::LALT,
		key::RWIN,
		standard_sep,
		key::RCTRL,
		f4_f5_sep,
		key::LEFT,
		key::DOWN,
		key::RIGHT
	);

	for (size_t i = 0; i < metrics.size(); ++i) {
		metrics[i].center_pos = metrics[i].lt_pos + vec3(metrics[i].size);
	}
}


int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	augs::create_directories("generated/logs/");

	set_default_keyboard_metrics();

	const auto cfg = augs::get_file_lines("config.cfg");

	std::random_device rd;
	std::mt19937 rng(rd());

	/* CONFIG SETTINGS */
	float volume = -1.f;
	bool enable_hrtf = false;
	bool mix_all_sounds_to_mono = false;
	vec3 listener_position = { -1.f, -1.f, -1.f };
	std::array<float, 6> listener_orientation = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
	float scale_key_positions = 1.f;
	std::string output_device;
	unsigned long long sleep_every_iteration_for_microseconds = 0u;
	/* END OF CONFIG SETTINGS */
	
	std::size_t current_line = 0;

	typesafe_sscanf(cfg[current_line++], "volume %x", volume);
	typesafe_sscanf(cfg[current_line++], "enable_hrtf %x", enable_hrtf);
	typesafe_sscanf(cfg[current_line++], "mix_all_sounds_to_mono %x", mix_all_sounds_to_mono);
	typesafe_sscanf(cfg[current_line++], "listener_position %x", listener_position);
	
	typesafe_sscanf(cfg[current_line++], "listener_orientation (%x;%x;%x;%x;%x;%x)", 
		listener_orientation[0], 
		listener_orientation[1], 
		listener_orientation[2], 
		listener_orientation[3], 
		listener_orientation[4], 
		listener_orientation[5]
	);

	typesafe_sscanf(cfg[current_line++], "scale_key_positions %x", scale_key_positions);
	typesafe_sscanf(cfg[current_line++], "output_device \"%x\"", output_device);
	typesafe_sscanf(cfg[current_line++], "sleep_every_iteration_for_microseconds %x", sleep_every_iteration_for_microseconds);

	ensure_eq("keys:", cfg[current_line++]);

	augs::audio_manager::generate_alsoft_ini(
		enable_hrtf,
		1024
	);

	augs::audio_manager manager(output_device);

	augs::set_listener_velocity(si_scaling(), {0.f, 0.f});
	augs::set_listener_orientation(listener_orientation);
	alListener3f(AL_POSITION, listener_position.x, listener_position.y, listener_position.z);

	std::unordered_map<std::string, augs::single_sound_buffer> sound_buffers;
	
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

		if (line.size() > 0 && line[0] == '%') {
			++current_line;
			continue;
		}

		std::string key_name;
		std::string sound_pairs;
		std::string position_str;

		typesafe_sscanf(
			line, 
			"name=\"%x\" position=%x pairs: %x", 
			key_name, 
			position_str, 
			sound_pairs
		);
		
		key_settings next_key;
		
		const auto key_id = wstring_to_key(std::wstring(key_name.begin(), key_name.end()));

		if (position_str == "default") {
			next_key.position = metrics[key_id].center_pos;
		}
		else {
			typesafe_sscanf(position_str, "%x", next_key.position);
		}

		next_key.position *= scale_key_positions;

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

		std::shuffle(next_key.pairs.begin(), next_key.pairs.end(), rng);

		keys[key_id] = next_key;

		++current_line;
	}

	{
		std::wstring positions;

		for (size_t i = 0; i < keys.size(); ++i) {
			if (keys[i].pairs.size() > 0) {
				positions += typesafe_sprintf(L"Key: %x Pos: %x\n", key_to_wstring(key(i)), keys[i].position);
			}
		}

		augs::create_text_file(
			std::string("generated/logs/debug_all_key_positions.txt"),
			std::string(positions.begin(), positions.end())
		);
	}


	std::vector<augs::sound_source> sound_sources;

	while (true) {
		if (sleep_every_iteration_for_microseconds > 0) {
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1us * sleep_every_iteration_for_microseconds);
		}

		for (int i = 0xFF - 1; i >= 0; --i) {
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
				LOG_NVPS(i);
				if(!subject_key.pressed) {
					subject_key.pressed = true;
					
					if (subject_key.pairs.empty()) {
						continue;
					}

					const auto& pair = subject_key.pairs[subject_key.next_pair_to_be_played];
#if LOG_PRESSES
					const auto name = key_to_wstring(id);
					
					LOG("DOWN " + std::string(name.begin(), name.end()));
#endif
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

#if LOG_PRESSES
					const auto name = key_to_wstring(id);
					
					LOG("UP " + std::string(name.begin(), name.end()));
#endif
			
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