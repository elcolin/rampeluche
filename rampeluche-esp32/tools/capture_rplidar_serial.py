#!/usr/bin/env python3
"""Capture brute d'un port serie vers un fichier binaire.

Sert a recuperer un fichier de test reel pour le decodeur RPLIDAR A2M8
(cf. test/fixtures/README.md) : pendant la capture, l'ESP32 doit relayer
tel quel les octets recus sur Serial2 (le port du lidar) vers Serial (USB) --
voir test/fixtures/README.md pour l'extrait de code temporaire a mettre dans
loop().

Usage:
    python3 tools/capture_rplidar_serial.py <port> [duree_s] [fichier_sortie]

Exemple:
    python3 tools/capture_rplidar_serial.py /dev/tty.usbmodem14201 5 \\
        test/fixtures/rplidar_a2m8_express_scan.bin

Necessite pyserial : pip install pyserial
"""
import sys
import time

try:
    import serial
except ImportError:
    print("pyserial n'est pas installe : pip install pyserial", file=sys.stderr)
    raise


def capture(port: str, duration_s: float, out_path: str) -> int:
    with serial.Serial(port, 115200, timeout=1) as ser:
        time.sleep(2.0)  # laisse l'ESP32 finir son demarrage / le scan s'amorcer
        ser.reset_input_buffer()

        data = bytearray()
        deadline = time.time() + duration_s
        while time.time() < deadline:
            chunk = ser.read(4096)
            if chunk:
                data.extend(chunk)

    with open(out_path, "wb") as f:
        f.write(data)

    return len(data)


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    port = sys.argv[1]
    duration_s = float(sys.argv[2]) if len(sys.argv) > 2 else 5.0
    out_path = sys.argv[3] if len(sys.argv) > 3 else "capture.bin"

    written = capture(port, duration_s, out_path)
    print(f"{written} octets ecrits dans {out_path}")
    if written == 0:
        print(
            "Aucune donnee recue : verifiez le port, que le scan a bien "
            "demarre (LidCtl.startExpressScan()) et le cablage du lidar.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
