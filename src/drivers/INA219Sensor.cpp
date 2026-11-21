#include "INA219Sensor.h"

#include <Wire.h>

#include "../config/HardwareConfig.h"

namespace {

// Espera maxima para la primera conversion del sensor tras la inicializacion.
// El INA219 en modo continuo tarda ~1 ms en completar la primera conversion;
// se espera solo durante el arranque para no mostrar ceros como datos validos.
constexpr uint32_t FIRST_CONVERSION_TIMEOUT_MS = 10UL;

}  // namespace

INA219Sensor::INA219Sensor()
    : m_ina(&Wire, HardwareConfig::INA219_I2C_ADDRESS)
    , m_address(HardwareConfig::INA219_I2C_ADDRESS)
    , m_i2cErrorCode(0)
    , m_initialized(false) {
}

bool INA219Sensor::begin() {
    Wire.begin(HardwareConfig::I2C_SDA_PIN,
               HardwareConfig::I2C_SCL_PIN,
               HardwareConfig::I2C_CLOCK_FREQ_HZ);

    m_i2cErrorCode = 0;
    m_initialized = false;

    // init() resetea el chip y devuelve false cuando el dispositivo no
    // responde (ACK) en su direccion I2C; en ese caso capturamos el codigo
    // de error nosotros.
    if (!m_ina.init()) {
        Wire.beginTransmission(m_address);
        m_i2cErrorCode = Wire.endTransmission();
        return false;
    }

    // Calibracion a partir de HardwareConfig.
    // setShuntSizeInOhms() fija la resistencia del shunt; la PGA define el
    // rango de tension del shunt: PG_320 = +-320 mV -> +-3.2 A con 0.1 ohm.
    // init() ya deja 12 bits, modo continuo y bus de 32 V, pero se reiteran
    // para que la intencion quede explicita y no dependa de los valores por
    // defecto de la biblioteca.
    m_ina.setShuntSizeInOhms(HardwareConfig::SHUNT_RESISTANCE_OHM);
    m_ina.setPGain(INA219_PG_320);
    m_ina.setBusRange(INA219_BRNG_32);

    // En modo continuo la primera lectura puede ser cero hasta que se
    // completa la primera conversion; se espera con un tope pequeno (bloquea
    // solo unos pocos ms de inicializacion, nunca en el bucle periodico).
    const uint32_t startMs = millis();
    while (!m_ina.getConversionReady() && (millis() - startMs) < FIRST_CONVERSION_TIMEOUT_MS) {
    }

    m_initialized = true;
    return true;
}

bool INA219Sensor::isDetected() {
    Wire.beginTransmission(m_address);
    m_i2cErrorCode = Wire.endTransmission();
    return (m_i2cErrorCode == 0);
}

bool INA219Sensor::reinitialize() {
    m_initialized = false;

    if (!m_ina.init()) {
        Wire.beginTransmission(m_address);
        m_i2cErrorCode = Wire.endTransmission();
        return false;
    }

    m_ina.setShuntSizeInOhms(HardwareConfig::SHUNT_RESISTANCE_OHM);
    m_ina.setPGain(INA219_PG_320);
    m_ina.setBusRange(INA219_BRNG_32);

    const uint32_t startMs = millis();
    while (!m_ina.getConversionReady() && (millis() - startMs) < FIRST_CONVERSION_TIMEOUT_MS) {
    }

    m_initialized = true;
    return true;
}

bool INA219Sensor::readMeasurement(ElectricalMeasurement& measurement) {
    measurement.timestampMs = millis();

    if (!m_initialized) {
        measurement.valid = false;
        measurement.overflowFlag = false;
        return false;
    }

    // Orden de lectura relevante: getShuntVoltage_mV() actualiza el flag
    // `overflow` de la biblioteca segun la saturacion del shunt (lo resetea
    // cuando no esta saturado), y getOverflow() despues anade el flag MOVF
    // de calculo (corriente/potencia fuera de rango).
    measurement.shuntVoltage_mV = m_ina.getShuntVoltage_mV();
    measurement.voltage_V       = m_ina.getBusVoltage_V();
    measurement.current_A       = m_ina.getCurrent_mA() / 1000.0f;  // la biblioteca reporta mA
    // Potencia calculada como P = V x I con las mismas lecturas que se
    // muestran en el informe.
    measurement.power_W         = measurement.voltage_V * measurement.current_A;

    measurement.overflowFlag = m_ina.getOverflow();

    // Limpieza del CNVR (y por tanto del MOVF) de cara a la siguiente
    // muestra: en modo continuo, leer el registro de potencia resetea el
    // CNVR, y el MOVF se limpia junto con el CNVR (semantica del INA219).
    m_ina.getConversionReady();

    // INA219_WE no expone un codigo de error I2C (a diferencia de
    // INA226_WE): verificamos el ACK con una transmision vacia propia. Un
    // codigo distinto de cero significa que los valores leidos no son
    // fiables (posible NACK por dispositivo ausente o bus caido).
    Wire.beginTransmission(m_address);
    m_i2cErrorCode = Wire.endTransmission();

    const bool communicationOk = (m_i2cErrorCode == 0);
    measurement.valid = communicationOk && !measurement.overflowFlag;
    return communicationOk;
}

uint8_t INA219Sensor::lastI2cErrorCode() const {
    return m_i2cErrorCode;
}
