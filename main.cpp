#include <Windows.h>

#include "augs/audio/audio_manager.h"
#include "augs/audio/sound_buffer.h"
#include "augs/audio/sound_source.h"

int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	augs::audio_manager manager;
	
	manager.generate_alsoft_ini(
		true,
		1024
	);

	augs::single_sound_buffer buf;
	buf.set_data(augs::get_sound_samples_from_file("keydown1.wav"));

	augs::sound_source src;
	src.bind_buffer(buf);
	src.play();

	while (src.is_playing()) {
		Sleep(16);
	}

	return 0;
}