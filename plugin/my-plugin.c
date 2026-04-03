#include <obs-module.h>
#include "ptz-core.h"

#ifdef WITH_PTZ_DOCK
extern void ptz_dock_try_register(void);
extern void ptz_dock_unregister(void);
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("my-plugin", "en-US")

struct my_source {
	obs_source_t *source;
};

static bool cb_up(obs_properties_t *p, obs_property_t *prop, void *d)
{
	(void)p;
	(void)prop;
	(void)d;
	ptz_start_motion(1);
	return true;
}

static bool cb_down(obs_properties_t *p, obs_property_t *prop, void *d)
{
	(void)p;
	(void)prop;
	(void)d;
	ptz_start_motion(2);
	return true;
}

static bool cb_left(obs_properties_t *p, obs_property_t *prop, void *d)
{
	(void)p;
	(void)prop;
	(void)d;
	ptz_start_motion(3);
	return true;
}

static bool cb_right(obs_properties_t *p, obs_property_t *prop, void *d)
{
	(void)p;
	(void)prop;
	(void)d;
	ptz_start_motion(4);
	return true;
}

static bool cb_stop(obs_properties_t *p, obs_property_t *prop, void *d)
{
	(void)p;
	(void)prop;
	(void)d;
	ptz_stop_motion();
	return true;
}

static const char *my_get_name(void *unused)
{
	(void)unused;
	return "PTZ Camera Control";
}

static obs_properties_t *my_properties(void *data)
{
	(void)data;
	obs_properties_t *props = obs_properties_create();

	obs_properties_t *move = obs_properties_create();
	obs_properties_add_button(move, "ptz_up", "     UP  ", cb_up);
	obs_properties_add_button(move, "ptz_left", "LEFT", cb_left);
	obs_properties_add_button(move, "ptz_stop", "STOP", cb_stop);
	obs_properties_add_button(move, "ptz_right", "RIGHT", cb_right);
	obs_properties_add_button(move, "ptz_down", "   DOWN   ", cb_down);

	obs_properties_add_group(props, "ptz_move", "Pan / tilt (diamond order)", OBS_GROUP_NORMAL, move);

	obs_properties_add_text(props, "ptz_hint", "Dock: Docks -> PTZ Control for a colored grid (if built with Qt).",
				OBS_TEXT_INFO);

	return props;
}

static void *my_create(obs_data_t *settings, obs_source_t *source)
{
	(void)settings;
	struct my_source *ctx = bzalloc(sizeof(struct my_source));
	ctx->source = source;
	return ctx;
}

static void my_destroy(void *data)
{
	(void)data;
	ptz_on_source_destroy();
	bfree(data);
}

static struct obs_source_info my_source_info = {
	.id = "onvif_ptz_plugin",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = my_get_name,
	.create = my_create,
	.destroy = my_destroy,
	.get_properties = my_properties,
};

bool obs_module_load(void)
{
	ptz_core_init();
	obs_register_source(&my_source_info);
#ifdef WITH_PTZ_DOCK
	ptz_dock_try_register();
#endif
	return true;
}

void obs_module_unload(void)
{
#ifdef WITH_PTZ_DOCK
	ptz_dock_unregister();
#endif
	ptz_core_free();
}
