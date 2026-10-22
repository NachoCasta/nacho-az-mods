#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Native UI/engine four-Off/two-On control for the managed localhost runner."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time


class LocalDeckTopology:
    def __init__(self, root, output, guest, logs, expected_disk='sdc'):
        spec=importlib.util.spec_from_file_location('topology_suite',Path(__file__).with_name('test-safe-eject-emulator.py'))
        self.suite=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.suite)
        assert self.suite.emu.azemu.SSH_PORT==2222
        assert self.suite.emu.azemu.QMP==('127.0.0.1',4446)
        self.suite.logs=logs
        self.root,self.output,self.guest,self.logs=root,output,guest,logs
        self.samples=[];self.pid=None;self.row=None
        assert expected_disk in ('sda','sdb','sdc')
        self.expected_disk=expected_disk

    def state(self):
        state=json.loads(self.guest('cat /run/azhub-eject-status').stdout)
        if self.pid is None:self.pid=state['pid']
        assert state['pid']==self.pid
        return state

    def save(self):
        (self.output/'local-deck-topology.json').write_text(json.dumps({'pid':self.pid,'samples':self.samples,
            'expected_source_backing':self.expected_disk+'1','expected_content_id':495,'audio_signal_captured':False},indent=2))

    def shot(self,name):
        subprocess.run([sys.executable,'C:/Users/icast/xdj-az-emu/tools/az-drive.py','shot',name],
            env=dict(os.environ,AZ_SHOT_DIR=str(self.output)),check=True)

    def select(self,deck):
        self.suite.drive('press',deck if deck<=2 else deck-2,'decksel'+str(deck),'.35')

    def waveform(self):
        for _ in range(3):
            if self.state()['view']==4:return
            self.suite.drive('panel','browse',1,delay=1)
        assert self.state()['view']==4,'Native waveform required'

    def browse(self):
        self.suite.center_knob()
        self.suite.drive('source',delay=1)
        self.shot('topology-source-selection')
        choice=self.output/'topology-source-row.txt'
        if self.row is None:
            if not choice.exists():
                print('TOPOLOGY_SOURCE_SELECTION_REQUIRED '+str(choice),flush=True)
                deadline=time.monotonic()+120
                while not choice.exists():
                    assert time.monotonic()<deadline,'Review emulator source screenshot and record verified hub row'
                    time.sleep(.2)
            row=int(choice.read_text().strip())
        else:row=self.row
        assert -1<=row<=7
        self.row=row;self.suite.browse_a(source_row=row)
        self.shot('topology-before-load')

    def load(self,deck):
        marker=len(self.logs());self.suite.load(deck,delay=1)
        deadline=time.monotonic()+30
        while True:
            info=self.state()['decks'][deck-1]
            if info['loaded']==1 and info['content_id']==495:break
            if time.monotonic()>=deadline:
                self.samples.append({'phase':'load failed deck'+str(deck),'state':self.state()});self.save()
                self.shot('topology-load-failed-'+str(deck))
                raise AssertionError(('Native load did not complete',deck,info))
            time.sleep(.2)
        self.suite.assert_load_disk(marker,deck,self.expected_disk)
        self.samples.append({'phase':'load verified deck'+str(deck),'state':self.state()});self.save()
        self.shot('topology-loaded-'+str(deck))

    def play_loop(self,deck):
        self.select(deck);side=deck if deck<=2 else deck-2
        if not self.state()['decks'][deck-1]['playing']:
            self.suite.drive('press',side,'play','.35')
        self.suite.drive('press',side,'loop4','.35')

    def observe(self,phase,decks):
        state=self.state()
        for deck in decks:
            info=state['decks'][deck-1]
            assert info['loaded']==1 and info['playing']==1 and info['content_id']==495,(phase,deck,info)
        self.samples.append({'phase':phase,'state':state});self.save()
        return state

    def sample(self,phase,decks):
        captured=[]
        for _ in range(8):
            state=self.state()
            for deck in decks:
                info=state['decks'][deck-1]
                assert info['loaded']==1 and info['playing']==1 and info['content_id']==495,(phase,deck,info)
            captured.append(state)
            self.samples.append({'phase':phase,'state':state});self.save();time.sleep(.35)
        for deck in decks:
            positions=[state['decks'][deck-1]['pos_s'] for state in captured]
            assert max(positions)-min(positions)>.05,(phase,deck,positions)

    def unload(self,deck):
        self.select(deck);self.waveform()
        self.suite.drive('tap',440,66,delay=.8)
        self.suite.drive('tap',310,188 if deck in (1,3) else 596,delay=.8)
        self.suite.drive('tap',1080,1000,delay=.8)
        deadline=time.monotonic()+15
        while self.state()['decks'][deck-1]['loaded']:
            assert time.monotonic()<deadline,('Native unload did not complete',deck)
            time.sleep(.2)

    def begin_on(self):
        assert all(not deck['loaded'] for deck in self.state()['decks'])
        self.select(1);self.select(2);self.browse()
        for deck in (1,2):
            if deck==2:self.browse()
            self.load(deck)
        for deck in (1,2):self.play_loop(deck)
        self.select(1);self.select(2);self.waveform();self.sample('initial Link On',(1,2))
        assert all(not self.state()['decks'][deck-1]['loaded'] for deck in (3,4))
        self.shot('topology-initial-two-On')

    def while_off(self):
        self.browse()
        for deck in (3,4):
            if deck==4:self.browse()
            self.load(deck)
        for deck in (3,4):self.play_loop(deck)
        self.select(1);self.select(2);self.waveform()
        self.suite.drive('tap',690,66,delay=.8)
        self.sample('Link Off four local decks',(1,2,3,4));self.shot('topology-four-Off')
        for deck in (3,4):self.unload(deck)
        self.select(1);self.select(2)
        self.sample('Link Off secondary decks unloaded',(1,2))

    def returned_on(self):
        self.sample('Link On restored',(1,2))
        # LOAD exits to waveform. Return to the retained native track browser;
        # this phase needs no encoder calibration or eject-selector ownership.
        for _ in range(3):
            if self.state()['view']==2:break
            self.suite.drive('panel','browse',1,delay=1)
        assert self.state()['view']==2,'Native retained track browser required'
        self.shot('topology-secondary-load-selection')
        approval=self.output/'topology-secondary-load-ready.txt'
        print('TOPOLOGY_TRACK_SELECTION_REVIEW_REQUIRED '+str(approval),flush=True)
        deadline=time.monotonic()+120
        while not approval.exists():
            assert time.monotonic()<deadline,'Verify retained native selected track before secondary negative load controls'
            time.sleep(.2)
        assert approval.read_text().strip()=='selected-track-verified'

        for deck in (3,4):self.suite.load(deck,delay=1)
        time.sleep(2)
        assert all(not self.state()['decks'][deck-1]['loaded'] for deck in (3,4)), 'Native Link On must keep decks3/4 unavailable'
        self.waveform();self.sample('Link On rejected secondary loads',(1,2));self.shot('topology-final-two-On')
        for deck in (1,2):self.unload(deck)
        assert all(not deck['loaded'] for deck in self.state()['decks'])
        self.save()
