/**
 * Driver del sensor INA219.
 *
 * Envuelve la biblioteca INA219_WE (composicion, sin herencia) y expone una
 * interfaz con verificacion de errores para el resto del firmware.
 *
 * Responsabilidades:
 *  - Inicializacion del bus I2C y del sensor
 *  - Calibracion (resistencia del shunt / rango de la PGA)
 *  - Lectura de registros con validacion
 *  - Verificacion de conexion y reporte de errores
 */
#ifndef INA219_SENSOR_H
#define INA219_SENSOR_H

#include <INA219_WE.h>

#include "../models/ElectricalMeasurement.h"

class INA219Sensor {
public:
    INA219Sensor();

    /**
     * Inicializa el bus I2C y el sensor.
     * Devuelve false si el dispositivo no responde en la direccion configurada.
     */
    bool begin();

    /** true cuando el dispositivo responde (ACK) en su direccion I2C. */
    bool isDetected();

    /** Reinicializa el sensor tras un fallo de comunicacion. */
    bool reinitialize();

    /**
     * Lee tension, corriente, potencia y tension de shunt en `measurement`.
     * Ante cualquier error I2C la medicion queda marcada como invalida
     * (nunca se representa como cero). Devuelve true si la comunicacion
     * con el sensor fue correcta.
     */
    bool readMeasurement(ElectricalMeasurement& measurement);

    /** Ultimo codigo de error I2C de la verificacion de ACK (0 = sin error). */
    uint8_t lastI2cErrorCode() const;

private:
    INA219_WE m_ina;             // objeto de la biblioteca (composicion)
    uint8_t m_address;           // direccion I2C, para verificaciones de conexion
    uint8_t m_i2cErrorCode;      // codigo de error cacheado
    bool m_initialized;          // true tras una inicializacion correcta
};

#endif  // INA219_SENSOR_H
