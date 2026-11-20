#include "MeasurementService.h"

#include "../config/HardwareConfig.h"

MeasurementService::MeasurementService()
    : m_state(State::NotInitialized)
    , m_lastSampleMs(0)
    , m_lastRetryMs(0)
    , m_consecutiveErrors(0) {
}

void MeasurementService::begin() {
    if (m_sensor.begin()) {
        m_state = State::Ok;
    } else {
        m_state = State::NotDetected;
        m_lastRetryMs = millis();
    }
}

bool MeasurementService::update() {
    const State previousState = m_state;
    const uint32_t now = millis();
    bool sampleAcquired = false;

    switch (m_state) {
        case State::NotInitialized:
            break;

        case State::NotDetected:
        case State::CommunicationError:
            if (now - m_lastRetryMs >= HardwareConfig::RETRY_INTERVAL_MS) {
                m_lastRetryMs = now;
                attemptRecovery();
            }
            break;

        case State::Ok:
            if (now - m_lastSampleMs >= HardwareConfig::SAMPLE_INTERVAL_MS) {
                m_lastSampleMs = now;
                acquireSample();
                sampleAcquired = true;
            }
            break;
    }

    return sampleAcquired || (m_state != previousState);
}

const ElectricalMeasurement& MeasurementService::latestMeasurement() const {
    return m_measurement;
}

MeasurementService::State MeasurementService::state() const {
    return m_state;
}

bool MeasurementService::attemptRecovery() {
    if (m_sensor.reinitialize()) {
        m_state = State::Ok;
        m_consecutiveErrors = 0;
        return true;
    }
    return false;
}

void MeasurementService::acquireSample() {
    // communicationOk == true significa que los registros se leyeron sin
    // errores I2C; la medicion puede quedar igualmente marcada como invalida
    // por overflow (eso NO es un problema de comunicacion y no debe disparar
    // la recuperacion).
    const bool communicationOk = m_sensor.readMeasurement(m_measurement);

    if (communicationOk) {
        m_consecutiveErrors = 0;
    } else {
        ++m_consecutiveErrors;
        if (m_consecutiveErrors >= HardwareConfig::MAX_CONSECUTIVE_ERRORS) {
            m_state = State::CommunicationError;
            m_lastRetryMs = millis();
        }
    }
}
