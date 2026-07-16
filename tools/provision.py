#!/usr/bin/env python3

import argparse
import base64
import getpass
from pathlib import Path
import subprocess
import time

import serial


def encode(value: str) -> str:
    return base64.b64encode(value.encode("utf-8")).decode("ascii")


def active_ssid() -> str:
    result = subprocess.run(
        ["nmcli", "-t", "-f", "ACTIVE,SSID", "device", "wifi"],
        check=True,
        capture_output=True,
        text=True,
    )
    for line in result.stdout.splitlines():
        if line.startswith("yes:"):
            return line.split(":", 1)[1]
    return ""


def main() -> None:
    parser = argparse.ArgumentParser(description="Provisiona el panel sin guardar secretos en el proyecto")
    parser.add_argument("--port", required=True)
    parser.add_argument("--device-ip", required=True)
    parser.add_argument("--device-id", required=True)
    parser.add_argument("--key-file", required=True, type=Path)
    args = parser.parse_args()

    detected_ssid = active_ssid()
    prompt = f"SSID Wi-Fi [{detected_ssid}]: " if detected_ssid else "SSID Wi-Fi: "
    ssid = input(prompt).strip() or detected_ssid
    if not ssid:
        raise SystemExit("No se indicó un SSID")
    password = getpass.getpass("Contraseña Wi-Fi (oculta): ")
    local_key = args.key_file.read_text().strip()
    if len(local_key) != 16:
        raise SystemExit("La clave local Tuya temporal no es válida")

    command = "CFG {} {} {} {} {}\n".format(
        encode(ssid), encode(password), args.device_ip, args.device_id, encode(local_key)
    )

    with serial.Serial(args.port, 115200, timeout=0.25) as connection:
        connection.reset_input_buffer()
        connection.write(command.encode("ascii"))
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = connection.readline().decode("utf-8", "replace").strip()
            if line == "PROVISIONING_OK":
                args.key_file.unlink(missing_ok=True)
                print("Configuración guardada en la placa; clave temporal eliminada.")
                return
            if line.startswith("PROVISIONING_ERROR"):
                raise SystemExit(line)
    raise SystemExit("La placa no confirmó el aprovisionamiento")


if __name__ == "__main__":
    main()
