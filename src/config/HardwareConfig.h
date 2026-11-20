/**
 * Configuracion centralizada de hardware y calibracion.
 *
 * Todos los pines, parametros del bus I2C y valores de calibracion viven aqui
 * para poder ajustarlos en un unico lugar.
 */
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <cstdint>

namespace HardwareConfig {

// ------------------------------- Bus I2C -----------------------------------
constexpr uint8_t  I2C_SDA_PIN        = 21;          // GPIO 21 (I2C por defecto ESP32)
constexpr uint8_t  I2C_SCL_PIN        = 22;          // GPIO 22 (I2C por defecto ESP32)
constexpr uint32_t I2C_CLOCK_FREQ_HZ  = 100000UL;    // 100 kHz

// ------------------------------- INA226 -------------------------------------
constexpr uint8_t  INA226_I2C_ADDRESS = 0x40;        // A0=A1 abiertos (valor tipico)

// ------------------------------- Adquisicion --------------------------------
constexpr uint32_t SAMPLE_INTERVAL_MS = 500UL;       // periodo de muestreo
constexpr uint32_t SERIAL_BAUD_RATE   = 115200UL;

// ------------------ Calibracion (VALORES PROVISIONALES) ----------------------
// Los valores reales deben verificarse contra el modulo fisico antes de dar
// por buenas las mediciones. Ver README.md, seccion "Parametros de calibracion".
//
// Los modulos INA226 comerciales suelen traer un shunt de 0.1 ohm, pero NO se
// debe asumir: mide el valor real (ver README) y actualiza esta constante.
constexpr float SHUNT_RESISTANCE_OHM = 0.1f;

// Corriente maxima esperada a traves del shunt. Dimensiona el registro de
// calibracion (LSB de corriente). Debe cumplir:
//   I_max * R_shunt <= 81.92 mV   (rango del INA226)
//   y respetar la potencia del shunt: P = I^2 * R.
constexpr float MAX_EXPECTED_CURRENT_A = 0.8f;

// ------------------------- Recuperacion de errores ---------------------------
constexpr uint32_t RETRY_INTERVAL_MS      = 2000UL;  // periodo entre reintentos de init
constexpr uint8_t  MAX_CONSECUTIVE_ERRORS = 5;       // errores seguidos -> modo recuperacion

}  // namespace HardwareConfig

#endif  // HARDWARE_CONFIG_H
