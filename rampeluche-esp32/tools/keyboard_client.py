#!/usr/bin/env python3
"""
Client clavier pour piloter le robot Rampeluche par Wi-Fi.

Se connecte au point d'acces Wi-Fi de l'ESP32 (SoftAP "ESP32_Control")
et envoie en continu la touche de direction maintenue (w/a/s/d). Le
firmware coupe automatiquement les moteurs si aucune touche n'est
recue pendant 500 ms (voir KEY_TIMEOUT_MS dans src/main.cpp), donc ce
client envoie aussi un caractere neutre quand rien n'est presse pour
un arret plus reactif.

Usage:
    pip install -r requirements.txt
    python3 keyboard_client.py [--host 192.168.4.1] [--port 1234]

Controles:
    w : avancer
    s : reculer
    a : pivoter a gauche
    d : pivoter a droite
    (relacher toutes les touches) : arret
    q ou Echap : quitter
"""
import argparse
import socket
import sys
import threading
import time

from pynput import keyboard

SEND_INTERVAL = 0.15   # s, doit rester < KEY_TIMEOUT_MS cote firmware (500 ms)
STOP_CHAR = " "         # touche "inconnue" pour le firmware -> arret moteur
KEY_PRIORITY = "wsad"   # touche prioritaire si plusieurs sont maintenues


class KeyboardClient:
    def __init__(self, host: str, port: int):
        self._sock = socket.create_connection((host, port), timeout=5)
        self._pressed = set()
        self._lock = threading.Lock()
        self._running = True

    def on_press(self, key):
        c = getattr(key, "char", None)
        if c == "q" or key == keyboard.Key.esc:
            self._running = False
            return False
        if c in "wasd":
            with self._lock:
                self._pressed.add(c)

    def on_release(self, key):
        c = getattr(key, "char", None)
        if c in "wasd":
            with self._lock:
                self._pressed.discard(c)

    def _current_key(self) -> str:
        with self._lock:
            for c in KEY_PRIORITY:
                if c in self._pressed:
                    return c
        return STOP_CHAR

    def run(self):
        listener = keyboard.Listener(on_press=self.on_press, on_release=self.on_release)
        listener.start()
        print("Connecte. w/a/s/d pour piloter, q ou Echap pour quitter.")
        try:
            while self._running:
                try:
                    self._sock.sendall(self._current_key().encode())
                except OSError as exc:
                    print(f"Connexion perdue : {exc}", file=sys.stderr)
                    break
                time.sleep(SEND_INTERVAL)
        finally:
            listener.stop()
            try:
                self._sock.sendall(STOP_CHAR.encode())
            except OSError:
                pass
            self._sock.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="192.168.4.1", help="IP du SoftAP ESP32")
    parser.add_argument("--port", type=int, default=1234, help="Port du serveur TCP")
    args = parser.parse_args()

    try:
        client = KeyboardClient(args.host, args.port)
    except OSError as exc:
        print(f"Impossible de se connecter a {args.host}:{args.port} ({exc})", file=sys.stderr)
        sys.exit(1)

    client.run()


if __name__ == "__main__":
    main()
