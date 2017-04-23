#include "augs/ensure.h"

void cleanup_proc() {
	global_log::save_complete_log("generated/logs/ensure_failed_debug_log.txt");

#ifdef PLATFORM_WINDOWS
	__debugbreak();
#endif
}
