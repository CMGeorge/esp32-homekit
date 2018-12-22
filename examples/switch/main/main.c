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

#define ACCESSORY_NAME  "Außenring"
#define ACCESSORY_NAME2  "Weiß außen"
#define ACCESSORY_NAME3  "Innenring"
#define ACCESSORY_NAME4  "Weiß innen"
#define MANUFACTURER_NAME   "TASSILO KARGE"
#define MODEL_NAME  "ULTI_LED_Warmweiss"
#define MODEL_NAME2  "ULTI_LED_Kaltweiss"
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
#define LED_STRIPE2 21
#define LED_STRIPE3 19
#define num_pixels 239
#define num_pixels2 50
#define num_pixels3 50

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

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
static int brightness = 70;
static void* brightness_ev_handle;
#define brightness_flag 'V'
static int saturation = 6500;
static void* saturation_ev_handle;
#define saturation_flag 'S'
static int hue = 3900;
static void* hue_ev_handle;
#define hue_flag 'H'
static int whiteled = 70;
static void* whiteled_ev_handle;
#define whiteled_flag 'W'

static int led2 = true;
static void* led_ev_handle2;
#define switch_flag2 'i'
static int brightness2 = 50;
static void* brightness_ev_handle2;
#define brightness_flag2 'v'
static int saturation2 = 6500;
static void* saturation_ev_handle2;
#define saturation_flag2 's'
static int hue2 = 3900;
static void* hue_ev_handle2;
#define hue_flag2 'h'
static int whiteled2 = 50;
static void* whiteled_ev_handle2;
#define whiteled_flag2 'w'

static strand_t strands[] = {
    {.rmtChannel = 0, .gpioNum = LED_STRIPE, .ledType = LED_SK6812W_V1, .brightLimit = 250, .numPixels = num_pixels,
   .pixels = NULL, ._stateVars = NULL},
    {.rmtChannel = 1, .gpioNum = LED_STRIPE2, .ledType = LED_SK6812W_V1, .brightLimit = 250, .numPixels = num_pixels2,
   .pixels = NULL, ._stateVars = NULL},
   {.rmtChannel = 2, .gpioNum = LED_STRIPE3, .ledType = LED_SK6812W_V1, .brightLimit = 250, .numPixels = num_pixels3,
   .pixels = NULL, ._stateVars = NULL}};

int STRANDCNT = sizeof(strands)/sizeof(strands[0]);

void hsvToRGB(float h, float s, float v, 
    int *red, int *green, int *blue) {
    double r = 0, g = 0, b = 0;

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
    //saturation and brightness expected in values 0-1
    hsvToRGB(hue/100.0, saturation/100.0/100.0, (brightness*led)/100.0, &r, &g, &b);

    int brightLimit = strands[0].brightLimit;
    r = (r*r * brightLimit) / 65025;
    g = (g*g * brightLimit) / 65025;
    b = (b*b * brightLimit) / 65025;
    w = (whiteled * led * brightLimit) / 255;

    printf("SET VALUES TO LED 1: ON: %d\nH: %.2f S: %.2f V: %d / R: %d G: %d B: %d W: %d\n", 
    led, hue/100.0, saturation/100.0, brightness, r, g, b, w);

    strand_t *strand = &(strands[0]);

    for (uint16_t i = 0; i < strand->numPixels; i++) {
            strand->pixels[i] = pixelFromRGBW(
                r,g,b,w);
    }
    digitalLeds_updatePixels(strand);
    gpio_set_level(LED_PORT, led || led2 ? 1 : 0);
}

void set_values_to_led2() {
    int r, g, b, w;
    //saturation and brightness expected in values 0-1
    hsvToRGB(hue2/100.0, saturation2/100.0/100.0, (brightness2*led2)/100.0, &r, &g, &b);

    int brightLimit = strands[0].brightLimit;
    r = (r*r * brightLimit) / 65025;
    g = (g*g * brightLimit) / 65025;
    b = (b*b * brightLimit) / 65025;
    w = (whiteled2 * led2 * brightLimit) / 255;

    printf("SET VALUES TO LED 2: ON: %d\nH: %.2f S: %.2f V: %d / R: %d G: %d B: %d W: %d\n", 
        led2, hue2/100.0, saturation2/100.0, brightness2, r, g, b, w);

    for (int i = 1; i < STRANDCNT; i++) {
        strand_t *strand = &(strands[i]);

        for (uint16_t i = 0; i < strand->numPixels; i++) {
                strand->pixels[i] = pixelFromRGBW(
                    r,g,b,w);
        }
        digitalLeds_updatePixels(strand);
    }
    gpio_set_level(LED_PORT, led || led2 ? 1 : 0);
}

void* led_read(void* arg)
{
    int c = (int) arg;
    printf("[MAIN] LED 1 READ %c \n", (char)c);
    switch(c) {
        case switch_flag:
            return (void *)led;
        case brightness_flag:
            return (void *)brightness;
        case hue_flag:
            return (void *)hue;
        case saturation_flag:
            return (void *)saturation;
        case whiteled_flag:
            return (void *)whiteled;
    }

    return NULL;
}

void* led_read2(void* arg)
{
    int c = (int) arg;
    printf("[MAIN] LED 2 READ %c \n", (char)c);
    switch(c) {
        case switch_flag2:
            return (void *)led2;
        case brightness_flag2:
            return (void *)brightness2;
        case hue_flag2:
            return (void *)hue2;
        case saturation_flag2:
            return (void *)saturation2;
        case whiteled_flag2:
            return (void *)whiteled2;
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
            //hue is float, but does not come multiplied with 100 as expected
            hue = (int) value * 100;
            break;
        case saturation_flag:
            //hue is float, but does not come multiplied with 100 as expected
            saturation = (int) value * 100;
            break;
        case whiteled_flag:
            whiteled = (int) value;
            break;
    }

    printf("[MAIN] LED WRITE. Arg: %c\n", (char)c);

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
        case whiteled_flag:
            if (whiteled_ev_handle)
                hap_event_response(a, whiteled_ev_handle, (void *)whiteled);
            break;
    }

    return;
}

void led_write2(void* arg, void* value, int len)
{
    int c = (int) arg;
    switch(c) {
        case switch_flag2:
            led2 = ((int) value) ? true : false;
            break;
        case brightness_flag2:
            brightness2 = (int) value;
            break;
        case hue_flag2:
            //hue is float, but does not come multiplied with 100 as expected
            hue2 = (int) value * 100;
            break;
        case saturation_flag2:
            //hue is float, but does not come multiplied with 100 as expected
            saturation2 = (int) value * 100;
            break;
        case whiteled_flag2:
            whiteled2 = (int) value;
            break;
    }

    printf("[MAIN] LED WRITE. Arg: %c\n", (char)c);

    set_values_to_led2();

    switch(c) {
        case switch_flag2:
            if (led_ev_handle2)
                hap_event_response(a, led_ev_handle2, (void *)led2);
            break;
        case brightness_flag2:
            if (brightness_ev_handle2)
                hap_event_response(a, brightness_ev_handle2, (void *)brightness2);
            break;
        case hue_flag2:
            if (hue_ev_handle2)
                hap_event_response(a, hue_ev_handle2, (void *)hue2);
            break;
        case saturation_flag2:
            if (saturation_ev_handle2)
                hap_event_response(a, saturation_ev_handle2, (void *)saturation2);
            break;
        case whiteled_flag2:
            if (whiteled_ev_handle)
                hap_event_response(a, whiteled_ev_handle2, (void *)whiteled2);
            break;
    }

    return;
}

void led_notify(void* arg, void* ev_handle, bool enable)
{
    int c = (int) arg;
    printf("[MAIN] LED NOTIFY %c \n", (char)c);   
    
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
        case whiteled_flag:
            whiteled_ev_handle = ev_handle;
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
        case whiteled_flag:
            whiteled_ev_handle = NULL;
        }
    }
}

void led_notify2(void* arg, void* ev_handle, bool enable)
{
    int c = (int) arg;
    printf("[MAIN] LED NOTIFY %c \n", (char)c);   
    
    if (enable) {
        switch(c) {
        case switch_flag2:
            led_ev_handle2 = ev_handle;
            break;
        case brightness_flag2:
            brightness_ev_handle2 = ev_handle;
            break;
        case hue_flag2:
            hue_ev_handle2 = ev_handle;
            break;
        case saturation_flag2:
            saturation_ev_handle2 = ev_handle;
            break;
        case whiteled_flag2:
            whiteled_ev_handle2 = ev_handle;
        }
    } else {
        switch(c) {
        case switch_flag2:
            led_ev_handle2 = NULL;
            break;
        case brightness_flag2:
            brightness_ev_handle2 = NULL;
            break;
        case hue_flag2:
            hue_ev_handle2 = NULL;
            break;
        case saturation_flag2:
            saturation_ev_handle2 = NULL;
            break;
        case whiteled_flag2:
            whiteled_ev_handle2 = NULL;
        }
    }
}

static bool _identifed = false;
void *identify_read(void* arg)
{
    int c = (int) arg;
    printf("[MAIN] IDENTIFY READ %c \n", (char)c);

    return (void*)true;
}

void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(a);
    
    struct hap_characteristic cs1[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.3", NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs1, ARRAY_SIZE(cs1));

    struct hap_characteristic cc1[] = {
        {HAP_CHARACTER_ON, (void*)led, (void*)switch_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_BRIGHTNESS, (void*)brightness, (void*)brightness_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_HUE, (void*)hue, (void*)hue_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_SATURATION, (void*)saturation, (void*)saturation_flag, led_read, led_write, led_notify}
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_LIGHTBULB, cc1, ARRAY_SIZE(cc1));

    void* accessory_object2 = hap_accessory_add(a);

    struct hap_characteristic cs2[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME2, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.3", NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(a, accessory_object2, HAP_SERVICE_ACCESSORY_INFORMATION, cs2, ARRAY_SIZE(cs2));

    struct hap_characteristic cc2[] = {
        {HAP_CHARACTER_ON, (void*)led, (void*)switch_flag, led_read, led_write, led_notify},
        {HAP_CHARACTER_BRIGHTNESS, (void*)whiteled, (void*)whiteled_flag, led_read, led_write, led_notify}
    };
    hap_service_and_characteristics_add(a, accessory_object2, HAP_SERVICE_LIGHTBULB, cc2, ARRAY_SIZE(cc2));

    void* accessory_object3 = hap_accessory_add(a);
    
    struct hap_characteristic cs3[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME2, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME3, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.3", NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(a, accessory_object3, HAP_SERVICE_ACCESSORY_INFORMATION, cs3, ARRAY_SIZE(cs3));

    struct hap_characteristic cc3[] = {
        {HAP_CHARACTER_ON, (void*)led2, (void*)switch_flag2, led_read2, led_write2, led_notify2},
        {HAP_CHARACTER_BRIGHTNESS, (void*)brightness2, (void*)brightness_flag2, led_read2, led_write2, led_notify2},
        {HAP_CHARACTER_HUE, (void*)hue2, (void*)hue_flag2, led_read2, led_write2, led_notify2},
        {HAP_CHARACTER_SATURATION, (void*)saturation2, (void*)saturation_flag2, led_read2, led_write2, led_notify2}
    };
    hap_service_and_characteristics_add(a, accessory_object3, HAP_SERVICE_LIGHTBULB, cc3, ARRAY_SIZE(cc3));

    void* accessory_object4 = hap_accessory_add(a);

    struct hap_characteristic cs4[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME2, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME4, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.3", NULL, NULL, NULL, NULL},
    };

    hap_service_and_characteristics_add(a, accessory_object4, HAP_SERVICE_ACCESSORY_INFORMATION, cs4, ARRAY_SIZE(cs4));

    struct hap_characteristic cc4[] = {
        {HAP_CHARACTER_ON, (void*)led2, (void*)switch_flag2, led_read2, led_write2, led_notify2},
        {HAP_CHARACTER_BRIGHTNESS, (void*)whiteled2, (void*)whiteled_flag2, led_read2, led_write2, led_notify2}
    };
    hap_service_and_characteristics_add(a, accessory_object4, HAP_SERVICE_LIGHTBULB, cc4, ARRAY_SIZE(cc4));
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

    /* for the led stripes */
    gpio_pad_select_gpio((gpio_num_t) LED_STRIPE);
    gpio_set_direction((gpio_num_t) LED_STRIPE, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t) LED_STRIPE, 0);

    gpio_pad_select_gpio((gpio_num_t) LED_STRIPE2);
    gpio_set_direction((gpio_num_t) LED_STRIPE2, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t) LED_STRIPE2, 0);

    gpio_pad_select_gpio((gpio_num_t) LED_STRIPE3);
    gpio_set_direction((gpio_num_t) LED_STRIPE3, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t) LED_STRIPE3, 0);

    digitalLeds_initStrands(strands, STRANDCNT);

    //set defaults when powering on
    set_values_to_led();
    set_values_to_led2();

    wifi_init_sta();
}
