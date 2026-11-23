"""
Escritura del CSV de captura en tiempo real: un archivo por sesion.

Los valores invalidos (NaN) se escriben como campos vacios, nunca como cero,
igual que hace el firmware en su salida Serial. Se hace flush en cada muestra
(a 2 Hz el coste es despreciable) para no perder datos ante un corte.
"""

from __future__ import annotations

import csv
import math
import time
from pathlib import Path

CSV_HEADER = [
    "elapsed_s",        # segundos desde la primera muestra de la sesion
    "timestamp_ms",     # millis() del ESP32 en la adquisicion
    "voltage_V",
    "current_A",
    "power_W",
    "shunt_voltage_mV",
    "valid",            # 0/1
    "overflow",         # 0/1
]


class CsvWriter:
    """Crea el archivo de captura de la sesion y escribe una fila por muestra."""

    def __init__(self, captures_dir: Path, enabled: bool = True):
        self._enabled = enabled
        self._dir = Path(captures_dir)
        self._file = None
        self._writer = None
        self._t0: float | None = None
        self.path: Path | None = None  # ruta del CSV de la sesion

    @property
    def enabled(self) -> bool:
        return self._enabled

    def start(self) -> None:
        """Crea el archivo de la sesion (data_AAAA-MM-DD_HH-MM-SS.csv)."""
        if not self._enabled:
            return
        self._dir.mkdir(parents=True, exist_ok=True)
        stamp = time.strftime("%Y-%m-%d_%H-%M-%S")
        self.path = self._dir / f"data_{stamp}.csv"
        self._file = self.path.open("w", newline="", encoding="utf-8")
        self._writer = csv.writer(self._file)
        self._writer.writerow(CSV_HEADER)
        self._file.flush()

    @staticmethod
    def _fmt(value: float) -> str:
        if math.isnan(value):
            return ""
        return f"{value:.6f}"

    def write_sample(self, sample) -> None:
        """Escribe una fila; sin efecto si la captura esta deshabilitada."""
        if not self._enabled or self._writer is None:
            return
        if self._t0 is None:
            self._t0 = time.monotonic()
        elapsed = time.monotonic() - self._t0

        self._writer.writerow([
            f"{elapsed:.3f}",
            str(sample.timestamp_ms),
            self._fmt(sample.voltage_V),
            self._fmt(sample.current_A),
            self._fmt(sample.power_W),
            self._fmt(sample.shunt_voltage_mV),
            int(sample.valid),
            int(sample.overflow),
        ])
        self._file.flush()

    def close(self) -> None:
        if self._file is not None:
            self._file.close()
            self._file = None
            self._writer = None
