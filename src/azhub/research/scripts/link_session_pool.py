# SPDX-License-Identifier: GPL-3.0-only
"""Bounded connection ownership for isolated research services."""
import socket
import threading


class SessionPool:
    def __init__(self, capacity=8):
        if not 1 <= capacity <= 32:
            raise ValueError('Invalid session capacity')
        self.capacity=capacity
        self.lock=threading.Lock()
        self.sessions={}
        self.closed=False

    def submit(self, client, handler):
        with self.lock:
            if self.closed or len(self.sessions)>=self.capacity:
                client.close()
                return False
            def run():
                try:
                    with client:handler(client)
                finally:
                    with self.lock:self.sessions.pop(threading.current_thread(),None)
            worker=threading.Thread(target=run)
            self.sessions[worker]=client
            worker.start()
            return True

    def close(self):
        with self.lock:
            self.closed=True
            owned=list(self.sessions.items())
        for worker,client in owned:
            try:client.shutdown(socket.SHUT_RDWR)
            except OSError:pass
            client.close()
        for worker,_ in owned:worker.join(timeout=3)
        if any(worker.is_alive() for worker,_ in owned):
            raise RuntimeError('Session failed to stop')
