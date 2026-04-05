// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <string.h>

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// #define BUF_SIZE 64

// int main(void)
// {
//     const struct device *dev;
//     uint8_t c;
//     char buf[BUF_SIZE];
//     int idx = 0;

//     if (!gpio_is_ready_dt(&led)) return 1;
//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

//     if (usb_enable(NULL)) return 1;

//     dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
//     if (!device_is_ready(dev)) return 1;

//     // Wymuszenie gotowości linii
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_RTS, 1);

//     memset(buf, 0, BUF_SIZE);

//     while (1) {
//         if (uart_poll_in(dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     buf[idx] = '\0';
//                     if (strstr(buf, "LED:1")) {
//                         gpio_pin_set_dt(&led, 1);
//                     } else if (strstr(buf, "LED:0")) {
//                         gpio_pin_set_dt(&led, 0);
//                     }
//                     idx = 0;
//                     memset(buf, 0, BUF_SIZE);
//                 }
//             } else {
//                 if (idx < BUF_SIZE - 1) {
//                     buf[idx++] = c;
//                 } else {
//                     idx = 0; // Zabezpieczenie przed przepełnieniem
//                     memset(buf, 0, BUF_SIZE);
//                 }
//             }
//         }
//         k_msleep(1);
//     }
//     return 0;
// }













// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/usb/usb_device.h>
// #include <string.h>
// #include <stdio.h> // Potrzebne do sprintf

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
// #define BUF_SIZE 64

// int main(void) {
//     const struct device *dev;
//     uint8_t c;
//     char buf[BUF_SIZE];
//     char tx_buf[64];
//     int idx = 0;
//     uint32_t uptime = 0;

//     gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//     if (usb_enable(NULL)) return 1;

//     dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_DTR, 1);
//     uart_line_ctrl_set(dev, UART_LINE_CTRL_RTS, 1);

//     while (1) {
//         // 1. ODBIERANIE (Toggle LED)
//         if (uart_poll_in(dev, &c) == 0) {
//             if (c == '\n' || c == '\r') {
//                 if (idx > 0) {
//                     buf[idx] = '\0';
//                     if (strstr(buf, "LED:1")) gpio_pin_set_dt(&led, 1);
//                     else if (strstr(buf, "LED:0")) gpio_pin_set_dt(&led, 0);
//                     idx = 0;
//                 }
//             } else if (idx < BUF_SIZE - 1) {
//                 buf[idx++] = c;
//             }
//         }

//         // 2. WYSYŁANIE DANYCH (Co 1 sekundę - prosta metoda)
//         // Używamy licznika pętli lub k_uptime_get()
//         if (k_uptime_get() % 1000 < 10) { 
//             uptime = k_uptime_get() / 1000;
//             // Generujemy udawaną temperaturę (np. 20-30 stopni)
//             float temp = 20.0f + (uptime % 10); 
            
//             sprintf(tx_buf, "DATA:T=%.1f;U=%d\r\n", (double)temp, uptime);
            
//             for (int i = 0; i < strlen(tx_buf); i++) {
//                 uart_poll_out(dev, tx_buf[i]);
//             }
//             k_msleep(10); // Mały odstęp, żeby nie słać serii ramek w 10ms
//         }

//         k_msleep(1);
//     }
//     return 0;
// }











#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <stdio.h>
#include <string.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

void send_str(const char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        uart_poll_out(uart_dev, str[i]);
    }
}

int main(void) {
    uint8_t c;
    char rx_buf[64];
    int idx = 0;
    bool last_state = false;

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&btn, GPIO_INPUT);

    usb_enable(NULL);
    uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DTR, 1);

    while (1) {
        // 1. Obsługa LED (PC -> STM32)
        if (uart_poll_in(uart_dev, &c) == 0) {
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    rx_buf[idx] = '\0';
                    if (strstr(rx_buf, "LED:1")) gpio_pin_set_dt(&led, 1);
                    else if (strstr(rx_buf, "LED:0")) gpio_pin_set_dt(&led, 0);
                    idx = 0;
                }
            } else if (idx < 63) {
                rx_buf[idx++] = c;
            }
        }

        // 2. Obsługa Przycisku (STM32 -> PC)
        bool current_state = gpio_pin_get_dt(&btn);
        if (current_state != last_state) {
            if (current_state) {
                send_str("BTN:1\r\n");
            } else {
                send_str("BTN:0\r\n");
            }
            last_state = current_state;
        }

        k_msleep(20); // Mały delay dla stabilności
    }
    return 0;
}