/**
 * Servicio de adquisicion de medidas electricas.
 *
 * Es propietario del driver del sensor y gestiona la adquisicion periodica y
 * la recuperacion ante fallos de comunicacion.
 *
 * No bloqueante: update() debe llamarse desde loop() tan a menudo como sea
 * posible; adquiere una muestra cada SAMPLE_INTERVAL_MS usando millis().
 */
#ifndef MEASUREMENT_SERVICE_H
#define MEASUREMENT_SERVICE_H

#include "../drivers/INA219Sensor.h"
#include "../models/ElectricalMeasurement.h"

class MeasurementService {
public:
    enum class State {
        NotInitialized,      // begin() aun no llamado
        Ok,                  // muestreo normal
        NotDetected,         // el dispositivo no responde; reintentando
        CommunicationError   // se perdio la comunicacion; reinicializando
    };

    MeasurementService();

    /** Inicializa el sensor. Llamar una vez al arranque. */
    void begin();

    /**
     * Actualizacion periodica no bloqueante. Devuelve true cuando se adquirio
     * una muestra nueva o cambio el estado (p. ej. error o recuperacion).
     */
    bool update();

    /** Ultima muestra adquirida (siempre legible; consultar su flag `valid`). */
    const ElectricalMeasurement& latestMeasurement() const;

    /** Estado actual del sensor/sistema. */
    State state() const;

private:
    bool attemptRecovery();
    void acquireSample();

    INA219Sensor m_sensor;
    ElectricalMeasurement m_measurement;
    State m_state;
    uint32_t m_lastSampleMs;
    uint32_t m_lastRetryMs;
    uint8_t m_consecutiveErrors;
};

#endif  // MEASUREMENT_SERVICE_H
