#include "motor.hpp"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/queue.h"
#include "cJSON.h" // Biblioteca nativa da ESP-IDF para ler o JSON

#define MOTOR_PWM_PIN 27
#define MOTOR_MIN_PULSEWIDTH 204  
#define MOTOR_MAX_PULSEWIDTH 1024
#define SERVO_PWM_PIN 14
#define SERVO_MIN_PULSEWIDTH 204  
#define SERVO_MAX_PULSEWIDTH 1024

gpio_num_t motor_frente = GPIO_NUM_26;
gpio_num_t motor_re = GPIO_NUM_25;

// Estrutura que guarda os comandos traduzidos
typedef struct {
    float eixoX;
    float eixoY;
} ComandoControle;

typedef struct {
    char mensagem[128];
    char type;
} MensagemLog;

// A nossa "caixa de correio" entre o Server e o Motor
extern QueueHandle_t fila_controle;

void motor_setup(void)
{
    // =================================================================
    // 1. CONFIGURAÇÃO DO SERVO (Direção) - Mantém 50Hz
    // =================================================================
    ledc_timer_config_t servo_timer = {};
    servo_timer.speed_mode      = LEDC_HIGH_SPEED_MODE;
    servo_timer.duty_resolution = LEDC_TIMER_13_BIT; // 8192 fatias por ciclo
    servo_timer.timer_num       = LEDC_TIMER_0;
    servo_timer.freq_hz         = 50;                // STRICT 50Hz para servo
    servo_timer.clk_cfg         = LEDC_AUTO_CLK;
    
    ledc_timer_config(&servo_timer);

    ledc_channel_config_t servo_channel = {};
    servo_channel.gpio_num      = SERVO_PWM_PIN;
    servo_channel.speed_mode    = LEDC_HIGH_SPEED_MODE;
    servo_channel.channel       = LEDC_CHANNEL_0;
    servo_channel.timer_sel     = LEDC_TIMER_0;
    servo_channel.duty          = 0;
    servo_channel.hpoint        = 0;

    ledc_channel_config(&servo_channel);

    // =================================================================
    // 2. CONFIGURAÇÃO DO MOTOR DC (Propulsão) - Alta Frequência
    // =================================================================
    ledc_timer_config_t motor_timer = {};
    motor_timer.speed_mode      = LEDC_HIGH_SPEED_MODE;
    motor_timer.duty_resolution = LEDC_TIMER_13_BIT; // Mesma resolução (0 a 8191)
    motor_timer.timer_num       = LEDC_TIMER_1;      // TIMER 1 (Independente do servo)
    motor_timer.freq_hz         = 5000;              // 5kHz para giro suave e silencioso
    motor_timer.clk_cfg         = LEDC_AUTO_CLK;
    
    ledc_timer_config(&motor_timer);

    ledc_channel_config_t motor_channel = {};
    motor_channel.gpio_num      = MOTOR_PWM_PIN;     // Pino 27
    motor_channel.speed_mode    = LEDC_HIGH_SPEED_MODE;
    motor_channel.channel       = LEDC_CHANNEL_1;    // CANAL 1 (Independente do servo)
    motor_channel.timer_sel     = LEDC_TIMER_1;      // Vincula ao Timer 1
    motor_channel.duty          = 0;
    motor_channel.hpoint        = 0;

    ledc_channel_config(&motor_channel);

    gpio_reset_pin(motor_frente);
    gpio_set_direction(motor_frente, GPIO_MODE_OUTPUT);
    gpio_reset_pin(motor_re);
    gpio_set_direction(motor_re, GPIO_MODE_OUTPUT);
}

int return_pulso_medio_servo(void) {
    return SERVO_MIN_PULSEWIDTH + (SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) / 2;
}

void volante(float eixoX) {
    // Multiplica o eixoX (-1.0 a 1.0) pela variação máxima (410)
    // e soma com o centro (614)
    float realx = eixoX * (-1.0);
    int pulso_medio = return_pulso_medio_servo();
    int pulso_pwm = pulso_medio + (int)(realx * pulso_medio);

    // Garante que não vai passar dos limites (Trava de segurança)
    if (pulso_pwm < SERVO_MIN_PULSEWIDTH) pulso_pwm = SERVO_MIN_PULSEWIDTH;
    if (pulso_pwm > SERVO_MAX_PULSEWIDTH) pulso_pwm = SERVO_MAX_PULSEWIDTH;

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, pulso_pwm);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void motor(float eixoY) {
    // Multiplica o eixoY (-1.0 a 1.0) pela variação máxima (410)
    // e soma com o centro (614)
    int pulso_pwm = MOTOR_MAX_PULSEWIDTH * eixoY;

    // Garante que não vai passar dos limites (Trava de segurança)
    if (pulso_pwm > MOTOR_MAX_PULSEWIDTH) pulso_pwm = MOTOR_MAX_PULSEWIDTH;

    if (eixoY < 0) {
        // Se eixoY for negativo, inverte a direção do motor
        gpio_set_level(motor_frente, 1);
        gpio_set_level(motor_re, 0);
    } else {
        // Se eixoY for positivo, mantém a direção normal
        gpio_set_level(motor_frente, 0);
        gpio_set_level(motor_re, 1);
    }

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, pulso_pwm);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

}

void iniciar_motor_task(void *pvParameters) {
    ComandoControle cmd;

    motor_setup();

    while (1) {
        // Tenta ler a fila. Espera no máximo 500ms (pdMS_TO_TICKS(500))
        if (xQueueReceive(fila_controle, &cmd, pdMS_TO_TICKS(500)) == pdTRUE) {
            volante(cmd.eixoX); // Mapeia o eixoX (-1.0 a 1.0) para o ângulo do servo
            motor(cmd.eixoY); // Mapeia o eixoY (-1.0 a 1.0) para a velocidade do motor
        } else {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, return_pulso_medio_servo());
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
        }
    }
}