"""
Parser de la linea CSV que emite el firmware del ESP32.

El firmware imprime por Serial, junto al reporte legible para humanos, una
linea por muestra con el prefijo "DATA,":

    DATA,12345,5.100,0.150,0.765,15.000,1,0

Campos: timestamp_ms (millis del ESP32), voltage_V, current_A, power_W,
shunt_voltage_mV, valid (0/1), overflow (0/1). Una muestra invalida lleva los
campos numericos vacios (nunca ceros); aqui se convierten a NaN.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import math
import time

CSV_PREFIX = "DATA,"

EXPECTED_FIELDS = 8


@dataclass
class Sample:
    """Una muestra electrica recibida del firmware."""

    timestamp_ms: int          # millis() del ESP32 en la adquisicion
    voltage_V: float           # tension de bus (NaN si es invalida)
    current_A: float           # corriente por el shunt (NaN si es invalida)
    power_W: float             # potencia calculada (NaN si es invalida)
    shunt_voltage_mV: float    # caida en el shunt (NaN si es invalida)
    valid: bool                # false cuando la lectura no es fiable
    overflow: bool             # true cuando el INA219 indica overflow
    received_at: float = field(default_factory=time.monotonic)  # reloj local


def _parse_float(field: str) -> float:
    """Convierte un campo numerico; el campo vacio es NaN, no cero."""
    value = field.strip()
    if value == "":
        return float("nan")
    return float(value)


def parse_line(line: str) -> Sample | None:
    """
    Interpreta una linea cruda del Serial.

    Devuelve un Sample para lineas "DATA,..." bien formadas, o None en
    cualquier otro caso (lineas del reporte legible, lineas malformadas).
    """
    stripped = line.strip()
    if not stripped.startswith(CSV_PREFIX):
        return None

    fields = stripped.split(",")
    if len(fields) != EXPECTED_FIELDS:
        return None

    try:
        _, ts, v, i, p, shunt, valid, overflow = fields
        return Sample(
            timestamp_ms=int(ts),
            voltage_V=_parse_float(v),
            current_A=_parse_float(i),
            power_W=_parse_float(p),
            shunt_voltage_mV=_parse_float(shunt),
            valid=(valid.strip() == "1"),
            overflow=(overflow.strip() == "1"),
        )
    except ValueError:
        return None
