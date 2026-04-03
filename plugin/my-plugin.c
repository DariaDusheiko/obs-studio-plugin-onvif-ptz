#include <obs-module.h>
#include <curl/curl.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define usleep(x) Sleep((x)/1000)
    #include <process.h>
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("my-plugin", "en-US")

#define CAM_IP   "172.18.212.18"
#define CAM_USER "admin"
#define CAM_PASS "Supervisor"
#define PTZ_SPEED 50

struct my_source {
    obs_source_t *source;
};

static struct {
    int active_direction; 
#ifdef _WIN32
    HANDLE thread;
    int running;
    CRITICAL_SECTION mutex;
#else
    pthread_t thread;
    int running;
    pthread_mutex_t mutex;
#endif
} motion_state = {0, 0, 0};

#ifdef _WIN32
static void init_mutex(void) { InitializeCriticalSection(&motion_state.mutex); }
static void lock_mutex(void) { EnterCriticalSection(&motion_state.mutex); }
static void unlock_mutex(void) { LeaveCriticalSection(&motion_state.mutex); }
static void destroy_mutex(void) { DeleteCriticalSection(&motion_state.mutex); }
#else
static void init_mutex(void) { pthread_mutex_init(&motion_state.mutex, NULL); }
static void lock_mutex(void) { pthread_mutex_lock(&motion_state.mutex); }
static void unlock_mutex(void) { pthread_mutex_unlock(&motion_state.mutex); }
static void destroy_mutex(void) { pthread_mutex_destroy(&motion_state.mutex); }
#endif

static void send_ptz_command(int pan, int tilt) {
    char url[512];
    char soap_request[512];
    snprintf(url, sizeof(url), "http://%s:80/ISAPI/PTZCtrl/channels/1/continuous", CAM_IP);
    snprintf(soap_request, sizeof(soap_request),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<PTZData version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
        "<continuous><panTilt><pan>%d</pan><tilt>%d</tilt></panTilt></continuous>"
        "</PTZData>", pan, tilt);

    CURL *curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/xml");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, soap_request);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERNAME, CAM_USER);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, CAM_PASS);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

#ifdef _WIN32
static unsigned __stdcall motion_thread(void* arg) {
#else
static void* motion_thread(void* arg) {
#endif
    (void)arg;
    while (motion_state.running) {
        lock_mutex();
        int dir = motion_state.active_direction;
        unlock_mutex();

        int pan = 0, tilt = 0;
        if(dir == 1) tilt = PTZ_SPEED;
        else if(dir == 2) tilt = -PTZ_SPEED;
        else if(dir == 3) pan = -PTZ_SPEED;
        else if(dir == 4) pan = PTZ_SPEED;

        send_ptz_command(pan, tilt);
        usleep(100000);
    }
    return 0;
}

static void start_motion(int direction) {
    lock_mutex();
    if (!motion_state.running) {
        motion_state.running = 1;
#ifdef _WIN32
        motion_state.thread = (HANDLE)_beginthreadex(NULL, 0, motion_thread, NULL, 0, NULL);
#else
        pthread_create(&motion_state.thread, NULL, motion_thread, NULL);
#endif
    }
    motion_state.active_direction = direction;
    unlock_mutex();
}

static void stop_motion(void) {
    lock_mutex();
    motion_state.active_direction = 0;
    unlock_mutex();
}

static bool cb_up(obs_properties_t *p, obs_property_t *prop, void *d) { start_motion(1); return true; }
static bool cb_down(obs_properties_t *p, obs_property_t *prop, void *d) { start_motion(2); return true; }
static bool cb_left(obs_properties_t *p, obs_property_t *prop, void *d) { start_motion(3); return true; }
static bool cb_right(obs_properties_t *p, obs_property_t *prop, void *d) { start_motion(4); return true; }
static bool cb_stop(obs_properties_t *p, obs_property_t *prop, void *d) { stop_motion(); return true; }

static const char *my_get_name(void *unused) { return "PTZ Camera Control"; }

static obs_properties_t *my_properties(void *data) {
    obs_properties_t *props = obs_properties_create();
    obs_properties_add_button(props, "ptz_up", "UP", cb_up);
    obs_properties_add_button(props, "ptz_down", "DOWN", cb_down);
    obs_properties_add_button(props, "ptz_left", "LEFT", cb_left);
    obs_properties_add_button(props, "ptz_right", "RIGHT", cb_right);
    obs_properties_add_button(props, "ptz_stop", "STOP", cb_stop);
    return props;
}

static void *my_create(obs_data_t *settings, obs_source_t *source) {
    struct my_source *ctx = bzalloc(sizeof(struct my_source));
    ctx->source = source;
    return ctx;
}

static void my_destroy(void *data) {
    motion_state.running = 0;
#ifdef _WIN32
    WaitForSingleObject(motion_state.thread, 1000);
    CloseHandle(motion_state.thread);
#else
    pthread_join(motion_state.thread, NULL);
#endif
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

bool obs_module_load(void) {
    init_mutex();
    obs_register_source(&my_source_info);
    return true;
}