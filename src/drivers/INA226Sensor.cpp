#include "INA226Sensor.h"

#include "../config/HardwareConfig.h"

INA226Sensor::INA226Sensor()
    : m_ina(&Wire, HardwareConfig::INA226_I2C_ADDRESS)
    , m_address(HardwareConfig::INA226_I2C_ADDRESS)
    , m_i2cErrorCode(0)
    , m_initialized(false) {
}

bool INA226Sensor::begin() {
    Wire.begin(HardwareConfig::I2C_SDA_PIN,
               HardwareConfig::I2C_SCL_PIN,
               HardwareConfig::I2C_CLOCK_FREQ_HZ);

    m_i2cErrorCode = 0;
    m_initialized = false;

    // init() devuelve false cuando el dispositivo no responde (ACK) en su
    // direccion I2C; en ese caso capturamos el codigo de error nosotros.
    if (!m_ina.init()) {
        Wire.beginTransmission(m_address);
        m_i2cErrorCode = Wire.endTransmission();
        return false;
    }

    // Calibracion a partir de HardwareConfig: recalcula el registro de
    // calibracion para el shunt y el rango de corriente configurados.
    m_ina.setResistorRange(HardwareConfig::SHUNT_RESISTANCE_OHM,
                           HardwareConfig::MAX_EXPECTED_CURRENT_A);

    // En modo continuo la primera lectura puede ser cero hasta que se
    // completa la primera conversion; se espera una vez (bloquea solo
    // durante unos pocos ms de inicializacion, no en el bucle periodico).
    m_ina.waitUntilConversionCompleted();

    m_initialized = true;
    return true;
}

bool INA226Sensor::isDetected() {
    Wire.beginTransmission(m_address);
    m_i2cErrorCode = Wire.endTransmission();
    return (m_i2cErrorCode == 0);
}

bool INA226Sensor::reinitialize() {
    m_initialized = false;

    if (!m_ina.init()) {
        Wire.beginTransmission(m_address);
        m_i2cErrorCode = Wire.endTransmission();
        return false;
    }

    m_ina.setResistorRange(HardwareConfig::SHUNT_RESISTANCE_OHM,
                           HardwareConfig::MAX_EXPECTED_CURRENT_A);
    m_ina.waitUntilConversionCompleted();

    m_initialized = true;
    return true;
}

bool INA226Sensor::readMeasurement(ElectricalMeasurement& measurement) {
    measurement.timestampMs = millis();

    if (!m_initialized) {
        measurement.valid = false;
        measurement.overflowFlag = false;
        return false;
    }

    measurement.shuntVoltage_mV = m_ina.getShuntVoltage_mV();
    measurement.voltage_V       = m_ina.getBusVoltage_V();
    measurement.current_A       = m_ina.getCurrent_A();
    measurement.power_W         = m_ina.getBusPower() / 1000.0f;  // la biblioteca reporta mW

    // Actualiza el flag publico `overflow` del objeto de la biblioteca.
    m_ina.readAndClearFlags();
    measurement.overflowFlag = m_ina.overflow;

    // La biblioteca guarda aqui el resultado del ultimo acceso a registro;
    // un codigo distinto de cero significa que los valores leidos no son
    // fiables (posible NACK por dispositivo ausente o bus caido).
    m_i2cErrorCode = m_ina.getI2cErrorCode();

    const bool communicationOk = (m_i2cErrorCode == 0);
    measurement.valid = communicationOk && !measurement.overflowFlag;
    return communicationOk;
}

uint8_t INA226Sensor::lastI2cErrorCode() const {
    return m_i2cErrorCode;
}
