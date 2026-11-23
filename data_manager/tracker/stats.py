"""
Estadisticas de ventana movil: promedio, minimo, maximo (pico) y RMS.

Las muestras invalidas (NaN) se excluyen del calculo, pero el tiempo sigue
corriendo: la ventana se define sobre el tiempo transcurrido, no sobre el
numero de muestras.
"""

from __future__ import annotations

import math
from collections import deque


class RollingStats:
    """Mantiene muestras (t, valor) dentro de una ventana temporal movil."""

    def __init__(self, window_seconds: float):
        self._window = float(window_seconds)
        self._samples: deque[tuple[float, float]] = deque()

    def add(self, t: float, value: float) -> None:
        self._samples.append((t, value))
        self._prune(t)

    def clear(self) -> None:
        self._samples.clear()

    def _prune(self, now: float) -> None:
        cutoff = now - self._window
        while self._samples and self._samples[0][0] < cutoff:
            self._samples.popleft()

    def _values(self) -> list[float]:
        return [value for _, value in self._samples if not math.isnan(value)]

    def count(self) -> int:
        """Numero de muestras validas dentro de la ventana."""
        return len(self._values())

    def last(self) -> float:
        """Ultimo valor valido (NaN si no hay ninguno)."""
        values = self._values()
        return values[-1] if values else float("nan")

    def mean(self) -> float:
        values = self._values()
        return sum(values) / len(values) if values else float("nan")

    def min(self) -> float:
        values = self._values()
        return min(values) if values else float("nan")

    def max(self) -> float:
        values = self._values()
        return max(values) if values else float("nan")

    def rms(self) -> float:
        values = self._values()
        if not values:
            return float("nan")
        return math.sqrt(sum(v * v for v in values) / len(values))
