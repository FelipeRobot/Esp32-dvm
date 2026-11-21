/**
 * ESP32 - Monitor de la rama de carga del generador DC (INA219).
 *
 * Punto de entrada del firmware. Coordina el MeasurementService y presenta el
 * informe periodico por Serial. No contiene logica de negocio: solo
 * inicializacion, escaneo I2C de arranque y presentacion.
 */
#include <Arduino.h>
#include <Wire.h>
#include <cstdio>

#include "config/HardwareConfig.h"
#include "services/MeasurementService.h"

namespace {

// Unica instancia de servicio del programa. Alcance de archivo (linkage
// interno): es la raiz de composicion y el unico objeto a nivel de modulo.
MeasurementService g_measurementService;

const char* stateLabel(MeasurementService::State state) {
    switch (state) {
        case MeasurementService::State::Ok:
            return "OK";
        case MeasurementService::State::NotDetected:
            return "NOT DETECTED (revisar cableado/direccion)";
        case MeasurementService::State::CommunicationError:
            return "COMM ERROR (recuperando...)";
        case MeasurementService::State::NotInitialized:
            return "NOT INITIALIZED";
    }
    return "UNKNOWN";
}

// Imprime una linea "etiqueta + valor + unidad". Con show == false muestra
// "n/a" en lugar de un numero: una lectura invalida nunca se presenta
// como si fuera un cero real.
void printValue(const char* label, float value, const char* unit, bool show) {
    char buffer[16];
    if (show) {
        snprintf(buffer, sizeof(buffer), "%8.3f", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%8s", "n/a");
    }
    Serial.printf("%-14s%s %s\n", label, buffer, unit);
}

// Escanea el bus I2C en el rango configurado y lista los dispositivos que
// responden. Sirve para verificar la presencia del INA219 sin asumir que
// esta conectado ni en que direccion responde.
void scanI2cBus() {
    Serial.printf("Escaneo I2C (0x%02X-0x%02X)...\n",
                  static_cast<unsigned>(HardwareConfig::I2C_SCAN_FIRST_ADDRESS),
                  static_cast<unsigned>(HardwareConfig::I2C_SCAN_LAST_ADDRESS));

    uint8_t foundCount = 0;
    for (uint8_t address = HardwareConfig::I2C_SCAN_FIRST_ADDRESS;
         address <= HardwareConfig::I2C_SCAN_LAST_ADDRESS;
         ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Dispositivo encontrado en 0x%02X\n",
                          static_cast<unsigned>(address));
            ++foundCount;
        }
    }

    if (foundCount == 0) {
        Serial.println("  Ningun dispositivo encontrado: revisar SDA/SCL y alimentacion");
        return;
    }

    Wire.beginTransmission(HardwareConfig::INA219_I2C_ADDRESS);
    const uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("  AVISO: el INA219 configurado (0x%02X) no responde\n",
                      static_cast<unsigned>(HardwareConfig::INA219_I2C_ADDRESS));
    }
}

void printReport() {
    const ElectricalMeasurement& m = g_measurementService.latestMeasurement();

    if (g_measurementService.state() == MeasurementService::State::Ok &&
        m.overflowFlag) {
        Serial.println("INA219 STATUS: OVERFLOW - shunt saturado / rango insuficiente");
    } else {
        Serial.printf("INA219 STATUS: %s\n", stateLabel(g_measurementService.state()));
    }

    const bool showValues = m.valid;
    printValue("Voltage:",       m.voltage_V,       "V",  showValues);
    printValue("Current:",       m.current_A,       "A",  showValues);
    printValue("Power:",         m.power_W,         "W",  showValues);
    printValue("Shunt Voltage:", m.shuntVoltage_mV, "mV", showValues);
    Serial.println("------------------------");
}

}  // namespace

void setup() {
    Serial.begin(HardwareConfig::SERIAL_BAUD_RATE);

    Serial.println();
    Serial.println("=== Monitor de carga DC (INA219) - Generador ===");
    Serial.printf("I2C: SDA=GPIO%u SCL=GPIO%u @ %lu Hz, INA219 en 0x%02X\n",
                  static_cast<unsigned>(HardwareConfig::I2C_SDA_PIN),
                  static_cast<unsigned>(HardwareConfig::I2C_SCL_PIN),
                  static_cast<unsigned long>(HardwareConfig::I2C_CLOCK_FREQ_HZ),
                  static_cast<unsigned>(HardwareConfig::INA219_I2C_ADDRESS));
    Serial.printf("Shunt (PROVISIONAL): %.3f ohm\n",
                  static_cast<double>(HardwareConfig::SHUNT_RESISTANCE_OHM));
    Serial.printf("Sample interval: %lu ms\n",
                  static_cast<unsigned long>(HardwareConfig::SAMPLE_INTERVAL_MS));
    Serial.println("Verifica la calibracion contra tu modulo antes de fiarte de las lecturas!");
    Serial.println("------------------------");

    // Inicializacion explicita del bus I2C (GPIO21/GPIO22) para el escaneo.
    // El driver del INA219 vuelve a llamar a Wire.begin() en su begin();
    // en el core Arduino-ESP32 repetir la inicializacion es inocuo.
    Wire.begin(HardwareConfig::I2C_SDA_PIN,
               HardwareConfig::I2C_SCL_PIN,
               HardwareConfig::I2C_CLOCK_FREQ_HZ);
    scanI2cBus();
    Serial.println("------------------------");

    g_measurementService.begin();
    printReport();  // muestra el estado inicial (OK o NOT DETECTED)
}

void loop() {
    if (g_measurementService.update()) {
        printReport();
    }
}
