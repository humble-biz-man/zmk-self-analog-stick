#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zmk/hid.h>
#include <zmk/events.h>
#include <zmk/keymap.h>

#define ADC_NODE        DT_NODELABEL(adc)
#define ADC_RESOLUTION  12
#define ADC_GAIN        ADC_GAIN_1
#define ADC_REFERENCE   ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define STICK_CENTER    2048 // 仮の中央値（12bit ADCの中央）
#define STICK_DEADZONE  100  // デッドゾーン
#define STICK_SCALE     20   // スケーリング

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

static int16_t read_adc_channel(uint8_t channel)
{
    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN,
        .reference = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id = channel,
    };
    adc_channel_setup(adc_dev, &channel_cfg);

    int16_t buf;
    struct adc_sequence sequence = {
        .channels = BIT(channel),
        .buffer = &buf,
        .buffer_size = sizeof(buf),
        .resolution = ADC_RESOLUTION,
    };
    adc_read(adc_dev, &sequence);
    return buf;
}

void analog_stick_task(void)
{
    while (1) {
        int16_t x = read_adc_channel(0); // X軸
        int16_t y = read_adc_channel(1); // Y軸

        int16_t dx = x - STICK_CENTER;
        int16_t dy = y - STICK_CENTER;

        if (abs(dx) < STICK_DEADZONE) dx = 0;
        if (abs(dy) < STICK_DEADZONE) dy = 0;

        dx /= STICK_SCALE;
        dy /= STICK_SCALE;

        if (dx != 0 || dy != 0) {
            struct zmk_hid_mouse_report report = {
                .x = dx,
                .y = dy,
            };
            zmk_hid_mouse_movement_report(&report);
        }
        k_msleep(10);
    }
}

K_THREAD_DEFINE(analog_stick_tid, 1024, analog_stick_task, NULL, NULL, NULL, 7, 0, 0); 