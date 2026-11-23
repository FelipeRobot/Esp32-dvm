"""
Sesion de captura: consume las lineas crudas del firmware, mantiene las
estadisticas de ventana movil y escribe el CSV en tiempo real.

Separa la logica de captura de la GUI para poder probarla sin pantalla.
"""

from __future__ import annotations

import time

from tracker.csv_writer import CsvWriter
from tracker.parser import CSV_PREFIX, Sample, parse_line
from tracker.stats import RollingStats


class CaptureSession:
    """Pipeline de una sesion: parsear, estadisticas, CSV."""

    def __init__(self, writer: CsvWriter, window_seconds: float):
        self.writer = writer
        self._v = RollingStats(window_seconds)
        self._i = RollingStats(window_seconds)
        self._p = RollingStats(window_seconds)
        self._t0: float | None = None
        self._last_sample_t: float | None = None
        self.paused = False
        self.samples = 0       # muestras registradas en la sesion
        self.malformed = 0     # lineas "DATA," malformadas
        self.ignored = 0       # lineas del reporte legible

    def process_raw_line(self, raw: str) -> Sample | None:
        """Procesa una linea cruda; devuelve el Sample si era una muestra."""
        if not raw.startswith(CSV_PREFIX):
            self.ignored += 1
            return None
        sample = parse_line(raw)
        if sample is None:
            self.malformed += 1
            return None
        self.add_sample(sample)
        return sample

    def add_sample(self, sample: Sample) -> None:
        if self._t0 is None:
            self._t0 = time.monotonic()
        elapsed = time.monotonic() - self._t0
        self._last_sample_t = time.monotonic()

        self._v.add(elapsed, sample.voltage_V)
        self._i.add(elapsed, sample.current_A)
        self._p.add(elapsed, sample.power_W)
        self.writer.write_sample(sample)
        self.samples += 1

    def elapsed(self) -> float:
        """Segundos desde la primera muestra de la sesion (0 si no hay)."""
        if self._t0 is None:
            return 0.0
        return time.monotonic() - self._t0

    def clear_view(self) -> None:
        """Reinicia estadisticas y referencia temporal (no toca el CSV)."""
        self._v.clear()
        self._i.clear()
        self._p.clear()
        self._t0 = time.monotonic() if self._t0 is not None else None

    def stale_seconds(self) -> float | None:
        """Segundos sin muestras nuevas (None si aun no llego ninguna)."""
        if self._last_sample_t is None:
            return None
        return time.monotonic() - self._last_sample_t

    def stats_dict(self) -> dict:
        """Estadisticas actuales de la ventana movil, listas para la GUI."""
        return {
            "v": {
                "last": self._v.last(),
                "mean": self._v.mean(),
                "min": self._v.min(),
                "max": self._v.max(),
                "rms": self._v.rms(),
                "count": self._v.count(),
            },
            "i": {"last": self._i.last()},
            "p": {"last": self._p.last()},
        }
