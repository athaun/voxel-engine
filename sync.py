# Used for syncing code for live coding sessions
# For host: python3 sync.py ./src/ shared@dev.athaun.tech:/home/shared/workspace/voxels/src/ 
# For client: python3 sync.py shared@dev.athaun.tech:/home/shared/workspace/voxels/src/ ./src/

import os, sys
import subprocess
import time
import signal

from_path = sys.argv[1]
to_path = sys.argv[2]

print(f"Running sync\n * from:\t{from_path}\n * to:\t{to_path}")

running = True

def exitApp ():
    running = False

signal.signal(signal.SIGINT, exitApp)

while running:
    os.system(f"rsync -aruzv {from_path} {to_path}")
    time.sleep(1.2)
