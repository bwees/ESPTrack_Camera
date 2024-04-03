#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include "jpeg_decoder.h"

void setup_camera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_scl = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;


    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 10;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1); // flip it back
        s->set_brightness(s, 1); // up the brightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }

    // drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);
    // s->set_special_effect(s, 2);
}

void camera_cap(void * pvParameters);
void frame_processor(void * pvParameters);

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    setup_camera();

    // create task for frame processor
    xTaskCreatePinnedToCore(frame_processor, "frame_processor", 4096, NULL, 5, NULL, 1);

    // create task for camera capture
    xTaskCreatePinnedToCore(camera_cap, "camera_cap", 4096, NULL, 5, NULL, 1);
}

int last;
int count;

// make queue
QueueHandle_t queue = xQueueCreate(10, sizeof(camera_fb_t *));

void frame_processor(void * pvParameters) {
    while (true) {
        camera_fb_t * fb;
        // wait for frame buffer
        if (xQueueReceive(queue, &fb, portMAX_DELAY) == pdTRUE) {
            // decode jpg from framebuffer with 
            // jpeg_decode function from jpeg_decoder.h
            uint8_t * out = (uint8_t *)malloc(fb->height * fb->width * 2);

            jpeg_error_t ret = jpeg_decode(fb->buf, fb->len, out);

            if (ret != JPEG_ERR_OK) {
                Serial.printf("JPEG decode failed - %d\n", (int)ret);
                break;
            }

            // clear buffers
            free(out);
            esp_camera_fb_return(fb);
        }
    }
}

void camera_cap(void * pvParameters) {
    while (true) {
        // capture frame from camera
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return;
        }

        // send address of frame buffer to queue
        xQueueSend(queue, &fb, pdMS_TO_TICKS(20));
        count++;

        // print fps ever second
        if (millis() - last > 1000) {
            Serial.printf("FPS: %d\n", count);
            last = millis();
            count = 0;
        }
    }
}

void loop() {
    delay(1000);
}