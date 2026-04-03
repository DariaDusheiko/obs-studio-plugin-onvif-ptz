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
static void init_mutex(void) {
    InitializeCriticalSection(&motion_state.mutex);
}
static void lock_mutex(void) {
    EnterCriticalSection(&motion_state.mutex);
}
static void unlock_mutex(void) {
    LeaveCriticalSection(&motion_state.mutex);
}
static void destroy_mutex(void) {
    DeleteCriticalSection(&motion_state.mutex);
}
#else
static void init_mutex(void) {
    pthread_mutex_init(&motion_state.mutex, NULL);
}
static void lock_mutex(void) {
    pthread_mutex_lock(&motion_state.mutex);
}
static void unlock_mutex(void) {
    pthread_mutex_unlock(&motion_state.mutex);
}
static void destroy_mutex(void) {
    pthread_mutex_destroy(&motion_state.mutex);
}
#endif

static void send_ptz_command(int pan, int tilt) {
    char url[512];
    char soap_request[512];
    
    snprintf(url, sizeof(url), 
        "http://%s:80/ISAPI/PTZCtrl/channels/1/continuous", CAM_IP);
    
    snprintf(soap_request, sizeof(soap_request),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<PTZData version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
        "<continuous>"
        "<panTilt>"
        "<pan>%d</pan>"
        "<tilt>%d</tilt>"
        "</panTilt>"
        "</continuous>"
        "</PTZData>", pan, tilt);
    
    CURL *curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/xml");
        
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

static void send_ptz_home(void) {
    char url[512];
    char soap_request[512];
    
    snprintf(url, sizeof(url), 
        "http://%s:80/ISAPI/PTZCtrl/channels/1/absoluteMove", CAM_IP);
    
    snprintf(soap_request, sizeof(soap_request),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<PTZData version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
        "<absoluteMove>"
        "<position>"
        "<pan>1750</pan>"
        "<tilt>450</tilt>"
        "</position>"
        "<speed>"
        "<panSpeed>%d</panSpeed>"
        "<tiltSpeed>%d</tiltSpeed>"
        "</speed>"
        "</absoluteMove>"
        "</PTZData>", PTZ_SPEED, PTZ_SPEED);
    
    CURL *curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/xml");
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, soap_request);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERNAME, CAM_USER);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, CAM_PASS);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        
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
        switch(dir) {
            case 1: tilt = PTZ_SPEED; break;
            case 2: tilt = -PTZ_SPEED; break;
            case 3: pan = -PTZ_SPEED; break;
            case 4: pan = PTZ_SPEED; break;
            default: pan = 0; tilt = 0; break;
        }
        
        if (pan != 0 || tilt != 0) {
            send_ptz_command(pan, tilt);
        } else {
            send_ptz_command(0, 0);
        }
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
    
    const char* dir_name[] = {"stop", "UP", "DOWN", "LEFT", "RIGHT"};
    blog(LOG_INFO, "[PTZ] Start: %s", dir_name[direction]);
}

static void stop_motion(void) {
    lock_mutex();
    motion_state.active_direction = 0;
    unlock_mutex();
    blog(LOG_INFO, "[PTZ] Stop");
}

static void stop_motion_complete(void) {
    if (motion_state.running) {
        motion_state.running = 0;
#ifdef _WIN32
        WaitForSingleObject(motion_state.thread, INFINITE);
        CloseHandle(motion_state.thread);
#else
        pthread_join(motion_state.thread, NULL);
#endif
    }
}

/* Обработчики кнопок */
static bool cb_up(obs_properties_t *props, obs_property_t *property, void *data) {
    start_motion(1);
    (void)props; (void)property; (void)data;
    return true;
}

static bool cb_down(obs_properties_t *props, obs_property_t *property, void *data) {
    start_motion(2);
    (void)props; (void)property; (void)data;
    return true;
}

static bool cb_left(obs_properties_t *props, obs_property_t *property, void *data) {
    start_motion(3);
    (void)props; (void)property; (void)data;
    return true;
}

static bool cb_right(obs_properties_t *props, obs_property_t *property, void *data) {
    start_motion(4);
    (void)props; (void)property; (void)data;
    return true;
}

static bool cb_stop(obs_properties_t *props, obs_property_t *property, void *data) {
    stop_motion();
    (void)props; (void)property; (void)data;
    return true;
}

static bool cb_home(obs_properties_t *props, obs_property_t *property, void *data) {
    stop_motion();
    send_ptz_home();
    blog(LOG_INFO, "[PTZ] GO HOME");
    (void)props; (void)property; (void)data;
    return true;
}

static const char *my_get_name(void *unused) {
    (void)unused;
    return "PTZ Camera Control";
}

static obs_properties_t *my_properties(void *data) {
    obs_properties_t *props = obs_properties_create();
    
    obs_properties_add_button(props, "ptz_up",    "    ▲ UP    ", cb_up);
    obs_properties_add_button(props, "ptz_down",  "  ▼ DOWN    ", cb_down);
    obs_properties_add_button(props, "ptz_left",  "  ◄ LEFT  ", cb_left);
    obs_properties_add_button(props, "ptz_right", "  RIGHT ►  ", cb_right);
    obs_properties_add_button(props, "ptz_stop",  "  ■ STOP  ", cb_stop);
    obs_properties_add_button(props, "ptz_home",  "🏠 HOME", cb_home);
    
    (void)data;
    return props;
}

static void *my_create(obs_data_t *settings, obs_source_t *source) {
    struct my_source *ctx = bzalloc(sizeof(struct my_source));
    ctx->source = source;
    (void)settings;
    return ctx;
}

static void my_destroy(void *data) {
    stop_motion_complete();
    bfree(data);
}

static void my_video_render(void *data, gs_effect_t *effect) {
    (void)data; (void)effect;
}

static uint32_t my_get_width(void *data) { 
    (void)data;
    return 1; 
}

static uint32_t my_get_height(void *data) { 
    (void)data;
    return 1; 
}

static struct obs_source_info my_source_info = {
    .id = "onvif_ptz_plugin",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = 0,
    .get_name = my_get_name,
    .create = my_create,
    .destroy = my_destroy,
    .video_render = my_video_render,
    .get_width = my_get_width,
    .get_height = my_get_height,
    .get_properties = my_properties,
};

bool obs_module_load(void) {
    init_mutex();
    blog(LOG_INFO, "========================================");
    blog(LOG_INFO, "[PTZ] PTZ Camera Control Plugin v7 (Windows)");
    blog(LOG_INFO, "[PTZ] Camera: %s", CAM_IP);
    blog(LOG_INFO, "[PTZ] Speed: %d", PTZ_SPEED);
    blog(LOG_INFO, "========================================");
    obs_register_source(&my_source_info);
    return true;
}
