#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline C packet comparison with the previously exercised Python prototype.

Execute only its status_service AST with in-memory socket/time stand-ins. No
network socket, namespace, device or source fixture is opened by this control.
"""
import ast
from pathlib import Path
import struct
import subprocess
import sys
from types import SimpleNamespace
from link_activation import Activation


def status_packet(kind):
    state = {"done": False, "sent": []}
    request = bytearray(48)
    request[:10] = b"Qspt1WmJOL"
    request[10] = kind
    request[47] = 4

    class Receiver:
        def __enter__(self):
            return self

        def __exit__(self, *args):
            return False

        def settimeout(self, value):
            pass

        def recvfrom(self, size):
            return bytes(request), ("192.0.2.15", 50002)

        def sendto(self, packet, address):
            state["sent"].append(bytes(packet))
            state["done"] = True

    tree = ast.parse(Path(__file__).with_name("link-public-provider-probe.py").read_text())
    function = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "status_service")
    namespace = dict(Activation=Activation, struct=struct, LABEL="Hub USB1 Research", BIND="192.0.2.2",
                     time=SimpleNamespace(monotonic=lambda: 0), emit=lambda *args, **kwargs: None,
                     bind_socket=lambda *args: None,
                     socket=SimpleNamespace(AF_INET=2, SOCK_DGRAM=2, socket=lambda *args: Receiver()))
    exec(compile(ast.Module(body=[function], type_ignores=[]), str(__file__), "exec"), namespace)
    namespace["status_service"](SimpleNamespace(is_set=lambda: state["done"]), 41, 2546, None)
    assert len(state["sent"]) == 1
    return state["sent"][0]


def main():
    assert len(sys.argv) == 2, "C_PACKET_TEST_BINARY"
    output = subprocess.check_output([sys.argv[1], "dump"], text=True)
    actual = [bytes.fromhex(line) for line in output.splitlines()]
    activation = Activation(41)
    expected = [activation.packet(0x16), activation.packet(0x47), status_packet(0x10), status_packet(5)]
    assert actual[:4] == expected, "Prototype status/activation packet mismatch"
    assert len(actual[4]) == 50 and actual[4][36:50] == bytes((192, 0, 2, 2, 2, 0, 0, 0, 17, 41, 41, 6, 4, 1))
    assert len(actual[5]) == 54 and actual[5][38:48] == bytes((2, 0, 0, 0, 17, 41, 192, 0, 2, 2))
    print("PASS: C activation/hello/media replies byte-match unchanged prototype routines; identity/keepalive fields match fixture")


if __name__ == "__main__":
    main()
