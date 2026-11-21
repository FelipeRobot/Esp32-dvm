/**
 * Estructura de datos simple que transporta una medicion electrica completa.
 * Las unidades son explicitas en el nombre de cada campo.
 */
#ifndef ELECTRICAL_MEASUREMENT_H
#define ELECTRICAL_MEASUREMENT_H

#include <cstdint>

struct ElectricalMeasurement {
    float voltage_V       = 0.0f;   // Tension de bus en voltios
    float current_A       = 0.0f;   // Corriente a traves del shunt en amperios
    float power_W         = 0.0f;   // Potencia en vatios
    float shuntVoltage_mV = 0.0f;   // Caida en el shunt en milivoltios

    bool valid        = false;      // false cuando la lectura no es fiable.
                                    // Nunca mostrar lecturas invalidas como datos reales.
    bool overflowFlag = false;      // true cuando el INA219 indica overflow de
                                    // registro (shunt saturado o calculo fuera de rango).
    uint32_t timestampMs = 0;       // millis() en el momento de la adquisicion
};

#endif  // ELECTRICAL_MEASUREMENT_H
