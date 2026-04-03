#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ptz_core_init(void);
void ptz_core_free(void);

void ptz_start_motion(int direction);
void ptz_stop_motion(void);

/** Called when the OBS source instance is destroyed (stops background thread). */
void ptz_on_source_destroy(void);

#ifdef __cplusplus
}
#endif
