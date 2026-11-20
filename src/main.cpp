/**
 * ESP32 Electrical Monitor - punto de entrada del firmware.
 *
 * Coordina el MeasurementService y presenta el informe periodico por Serial.
 * No contiene logica de negocio: solo inicializacion y presentacion.
 */
#include <Arduino.h>
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

void printReport() {
    const ElectricalMeasurement& m = g_measurementService.latestMeasurement();

    if (g_measurementService.state() == MeasurementService::State::Ok &&
        m.overflowFlag) {
        Serial.println("INA226 STATUS: OVERFLOW - aumentar rango de corriente");
    } else {
        Serial.printf("INA226 STATUS: %s\n", stateLabel(g_measurementService.state()));
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
    Serial.println("=== ESP32 Electrical Monitor ===");
    Serial.printf("I2C: SDA=GPIO%u SCL=GPIO%u @ %lu Hz, address 0x%02X\n",
                  static_cast<unsigned>(HardwareConfig::I2C_SDA_PIN),
                  static_cast<unsigned>(HardwareConfig::I2C_SCL_PIN),
                  static_cast<unsigned long>(HardwareConfig::I2C_CLOCK_FREQ_HZ),
                  static_cast<unsigned>(HardwareConfig::INA226_I2C_ADDRESS));
    Serial.printf("Shunt (PROVISIONAL): %.3f ohm | Max current (PROVISIONAL): %.3f A\n",
                  static_cast<double>(HardwareConfig::SHUNT_RESISTANCE_OHM),
                  static_cast<double>(HardwareConfig::MAX_EXPECTED_CURRENT_A));
    Serial.printf("Sample interval: %lu ms\n",
                  static_cast<unsigned long>(HardwareConfig::SAMPLE_INTERVAL_MS));
    Serial.println("Verifica la calibracion contra tu modulo antes de fiarte de las lecturas!");
    Serial.println("------------------------");

    g_measurementService.begin();
    printReport();  // muestra el estado inicial (OK o NOT DETECTED)
}

void loop() {
    if (g_measurementService.update()) {
        printReport();
    }
}
