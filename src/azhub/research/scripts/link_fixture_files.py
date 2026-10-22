# SPDX-License-Identifier: GPL-3.0-only
"""Bounded read-only NFSv2 fixture backend for isolated research, not a device export."""
import os
from pathlib import PurePosixPath, Path
import secrets
import struct


def words(*values):
    return struct.pack('!' + 'I' * len(values), *values)


def opaque(value):
    return words(len(value)) + value + b'\0' * (-len(value) % 4)


class FixtureFiles:
    def __init__(self, files):
        # Client lookups never become host paths: all files are explicitly opened
        # by the operator and assigned opaque, process-lifetime handles.
        if len(files) > 4096:
            raise ValueError('Too many fixture files')
        self.nodes = {}
        self.paths = {}
        self.active = True
        self.root = self._node('/', True)
        try:
            for virtual, local in files.items():
                path = PurePosixPath(virtual)
                if not virtual.startswith('/') or '..' in path.parts or '\\' in virtual:
                    raise ValueError('Invalid virtual path')
                parent = '/'
                for component in path.parts[1:-1]:
                    parent = (parent.rstrip('/') + '/' + component)
                    self._node(parent, True)
                identity = path.as_posix()
                if identity in self.paths:
                    raise ValueError('Duplicate virtual path')
                handle = self._node(identity, False)
                stream = Path(local).open('rb')
                self.nodes[handle]['file'] = stream
                info = os.fstat(stream.fileno())
                if not __import__('stat').S_ISREG(info.st_mode) or info.st_size > 0xffffffff:
                    raise ValueError('Expected bounded regular file')
                self.nodes[handle]['size'] = info.st_size
        except BaseException:
            self.close()
            raise

    def _node(self, path, directory):
        if path in self.paths:
            handle = self.paths[path]
            if self.nodes[handle]['directory'] != directory:
                raise ValueError('File/directory collision')
            return handle
        handle = secrets.token_bytes(32)
        self.paths[path] = handle
        self.nodes[handle] = dict(path=path, directory=directory, size=0, file=None,
                                  identity=len(self.nodes) + 1)
        return handle

    def close(self):
        self.active = False
        for node in self.nodes.values():
            if node['file'] is not None:
                node['file'].close()

    def mount(self, args):
        if not self.active:
            return words(2)
        if len(args) < 4:
            return words(5)
        size=struct.unpack_from('!I',args)[0]
        raw=args[4:4+size]
        if size>1024 or len(raw)!=size:
            return words(5)
        try:
            path=raw.decode('utf-16le' if b'\0' in raw else 'utf-8').rstrip('\0')
        except UnicodeError:
            return words(2)
        return words(0)+self.root if path in ('/C/','/C') else words(2)

    @staticmethod
    def attrs(node):
        directory = node['directory']; size = node['size']
        return words(2 if directory else 1, 0o40555 if directory else 0o100444,
                     1, 0, 0, size, 4096, 0, (size + 511)//512, 1,
                     node['identity'], 0, 0, 0, 0, 0, 0)

    def request(self, procedure, args):
        if procedure == 0:
            return b''
        if not self.active or len(args) < 32 or args[:32] not in self.nodes:
            return words(70)  # NFSERR_STALE, including handles from an old generation.
        node = self.nodes[args[:32]]
        if procedure == 1:
            return words(0) + self.attrs(node)
        if procedure == 4:
            if not node['directory']:
                return words(20)
            if len(args) < 36:
                return words(5)
            size = struct.unpack_from('!I', args, 32)[0]
            raw = args[36:36+size]
            if size > 1024 or len(raw) != size:
                return words(5)
            try:
                name = raw.decode('utf-16le' if b'\0' in raw else 'utf-8').rstrip('\0')
            except UnicodeError:
                return words(2)
            if not name or name in ('.', '..') or any(c in name for c in '/\\\0'):
                return words(2)
            path = node['path'].rstrip('/') + '/' + name
            handle = self.paths.get(path)
            if handle is None:
                return words(2)
            return words(0) + handle + self.attrs(self.nodes[handle])
        if procedure == 6:
            if node['directory']:
                return words(21)
            if len(args) != 44:
                return words(5)
            offset, count, _ = struct.unpack_from('!III', args, 32)
            if count > 8192:
                return words(5)
            node['file'].seek(offset)
            data = node['file'].read(count)
            return words(0) + self.attrs(node) + opaque(data)
        if procedure == 16:
            if not node['directory']:
                return words(20)
            if len(args) != 40:
                return words(5)
            cookie, count = struct.unpack_from('!II', args, 32)
            if count < 8 or count > 8192:
                return words(5)
            children = sorted((n for n in self.nodes.values()
                if n['path'] != '/' and str(PurePosixPath(n['path']).parent) == node['path']),
                key=lambda n: n['path'])
            if cookie > len(children):
                return words(5)
            result = b''; position = cookie
            for child in children[cookie:]:
                entry = words(1, child['identity']) + opaque(PurePosixPath(child['path']).name.encode('utf-16le')) + words(position + 1)
                if len(result) + len(entry) + 8 > count:
                    break
                result += entry; position += 1
            return words(0) + result + words(0, int(position == len(children)))
        if procedure == 17:
            return words(0, 8192, 4096, 1, 1, 1)
        return words(30)  # Read-only export; no mutation operation is implemented.
