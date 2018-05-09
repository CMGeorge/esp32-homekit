#include <string.h>
#include <math.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "hap.h"


#include "esp32_digital_led_lib.h"

#define TAG "LED STRIPE"
#define PIN_CODE "123-58-197"

#define ACCESSORY_NAME  "LED STRIPE"
#define MANUFACTURER_NAME   "TASSILO KARGE"
#define MODEL_NAME  "ULTILED"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#if 1
#define EXAMPLE_ESP_WIFI_SSID "unibj"
#define EXAMPLE_ESP_WIFI_PASS "12673063"
#endif
#if 0
#define EXAMPLE_ESP_WIFI_SSID "NO_RUN"
#define EXAMPLE_ESP_WIFI_PASS "1qaz2wsx"
#endif

static gpio_num_t LED_PORT = GPIO_NUM_2;
#define LED_STRIPE 22
#define num_pixels 60

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static void* a;

/* characteristics values, their event handles and their flag:s */
static int led = true;
static void* led_ev_handle;
#define switch_flag 'I'
static int brightness = 0;
static void* brightness_ev_handle;
#define brightness_flag 'V'
static int saturation = 0;
static void* saturation_ev_handle;
#define saturation_flag 'S'
static int hue = 0;
static void* hue_ev_handle;
#define hue_flag 'H'

static strand_t strands[] = {{.rmtChannel = 0, .gpioNum = LED_STRIPE, .ledType = LED_SK6812W_V1, .brightLimit = 255, .numPixels = num_pixels,
   .pixels = NULL, ._stateVars = NULL}};

void hsvToRGBW(float hue, float saturation, float brightness, 
    int *red, int *green, int *blue, int *white) {
    
    double r = 0, g = 0, b = 0;

    double h = hue;
    double s = saturation / 100.0;
    double v = brightness / 100.0;

    if (s <= 0)
    {
        r = v;
        g = v;
        b = v;
    } else {
        int i;
        double f, p, q, t;

        if (h >= 360)
            h = 0;
        else
            h = h / 60;

        i = (int)trunc(h);
        f = h - i;

        p = v * (1.0 - s);
        q = v * (1.0 - (s * f));
        t = v * (1.0 - (s * (1.0 - f)));

        switch (i)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;

        case 1:
            r = q;
            g = v;
            b = p;
            break;

        case 2:
            r = p;
            g = v;
            b = t;
            break;

        case 3:
            r = p;
            g = q;
            b = v;
            break;

        case 4:
            r = t;
            g = p;
            b = v;
            break;

        default:
            r = v;
            g = p;
            b = q;
            break;
        }
    }
    *red = r * 255;
    *green = g * 255;
    *blue = b * 255;
}

void set_values_to_led() {
    int r, g, b, w;
    hsvToRGBW(hue/100.0, saturation/100.0, brightness*led, &r, &g, &b, &w);

    strand_t *strand = &(strands[0]);

    for (uint16_t i = 0; i < strand->numPixels; i++) {
            strand->pixels[i] = pixelFromRGBW(
                r,g,b,w);
    }
    digitalLeds_updatePixels(strand);
    gpio_set_level(LED_PORT, led ? 1 : 0);
}

void* led_read(void* arg)
{
    int c = (int) arg;
    printf("[MAIN] LED READ %d \n", c);
    switch(c) {
        case switch_flag:
            return (void *)led;
        case brightness_flag:
            return (void *)brightness;
        case hue_flag:
            return (void *)hue;
        case saturation_flag:
            return (void *)saturation;
    }

    return NULL;
}

void led_write(void* arg, void* value, int len)
{
    int c = (int) arg;
    switch(c) {
        case switch_flag:
            led = ((int) value) ? true : false;
            break;
        case brightness_flag:
            brightness = (int) value;
            break;
        case hue_flag:
            hue = (int) value;
            break;
        case saturation_flag:
            saturation = (int) value;
            break;
    }
    
    printf("[MAIN] LED WRITE. Arg: %d ON: %d H: %.2f S: %.2f V: %d \n", 
        c, led, hue/100.0, saturation/100.0, brightness);

    set_values_to_led();

    switch(c) {
        case switch_flag:
            if (led_ev_handle)
                hap_event_response(a, led_ev_handle, (void *)led);
            break;
        case brightness_flag:
            if (brightness_ev_handle)
                hap_event_response(a, brightness_ev_handle, (void *)brightness);
            break;
        case hue_flag:
            if (hue_ev_handle)
                hap_event_response(a, hue_ev_handle, (void *)hue);
            break;
        case saturation_flag:
            if (saturation_ev_handle)
                hap_event_response(a, saturation_ev_handle, (void *)saturation);
            break;
    }

    return;
}

void led_notify(void* arg, void* ev_handle, bool enable)
{
    int c = (int) arg;
    printf("[MAIN] LED NOTIFY %d \n", c);   
    
    if (enable) {
        switch(c) {
        case switch_flag:
            led_ev_handle = ev_handle;
            break;
        case brightness_flag:
            brightness_ev_handle = ev_handle;
            break;
        case hue_flag:
            hue_ev_handle = ev_handle;
            break;
        case saturation_flag:
            saturation_ev_handle = ev_handle;
            break;
        }
    } else {
        switch(c) {
        case switch_flag:
            led_ev_handle = NULL;
            break;
        case brightness_flag:
            brightness_ev_handle = NULL;
            break;
        case hue_flag:
            hue_ev_handle = NULL;
            break;
        case saturation_flag:
            saturation_ev_handle = NULL;
            break;
        }
    }
}

static bool _identifed = false;
void *identify_read(void* arg)
{
    int c = (int) arg;
    printf("[MAIN] IDENTIFY READ %d \n", c);

    return (void*)true;
}

void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(a);
    
    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.1", NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

    struct hap_characteristic cc[] = {
        {HAP_CHARACTER_ON, (void*)led, (void*)switch_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_BRIGHTNESS, (void*)brightness, (void*)brightness_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_HUE, (void*)hue, (void*)hue_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_SATURATION, (void*)saturation, (void*)saturation_flag, led_read, led_write, led_notify}
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_LIGHTBULB, cc, ARRAY_SIZE(cc));
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        {
            hap_init();

            uint8_t mac[6];
            esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
            char accessory_id[32] = {0,};
            sprintf(accessory_id, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            hap_accessory_callback_t callback;
            callback.hap_object_init = hap_object_init;
            a = hap_accessory_register((char*)ACCESSORY_NAME, accessory_id, (char*)PIN_CODE, (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_LIGHTBULB, 811, 1, NULL, &callback);
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    gpio_pad_select_gpio(LED_PORT);
    gpio_set_direction(LED_PORT, GPIO_MODE_OUTPUT);

    /* for the led stripe */
    gpio_pad_select_gpio((gpio_num_t) LED_STRIPE);
    gpio_set_direction((gpio_num_t) LED_STRIPE, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t) LED_STRIPE, 0);

    digitalLeds_initStrands(strands, 1);

    //set defaults when powering on
    set_values_to_led();
    
    wifi_init_sta();
}
