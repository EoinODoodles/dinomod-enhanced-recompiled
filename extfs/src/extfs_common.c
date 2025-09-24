#include "extfs_common.h"

#include "recomputils.h"

ExtFsLoadStage extfsLoadStage = EXTFS_STAGE_UNINITIALIZED;

void extfs_assert(_Bool condition, const char *fmt, ...) {
    va_list args;
	va_start(args, fmt);

    if (!condition) {
        recomp_exit_with_error(recomp_vsprintf_helper(fmt, args));
    }

    va_end(args);
}

void extfs_log(const char *fmt, ...) {
    va_list args;
	va_start(args, fmt);

    // TODO: gate behind config setting

    recomp_vprintf(fmt, args);

    va_end(args);
}

void extfs_log_error(const char *fmt, ...) {
    va_list args;
	va_start(args, fmt);

    recomp_veprintf(fmt, args);

    va_end(args);
}

void extfs_error_box(const char *fmt, ...) {
    va_list args;
	va_start(args, fmt);

    recomp_error_message_box(recomp_vsprintf_helper(fmt, args));

    va_end(args);
}
