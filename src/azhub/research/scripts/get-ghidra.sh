#!/bin/bash
# Download and unpack Ghidra into ~/tools (no sudo needed).
set -e
mkdir -p ~/tools && cd ~/tools
Z=ghidra_12.1.4_PUBLIC_20260921.zip
[ -s "$Z" ] || curl -sL -o "$Z" "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.1.4_build/$Z"
ls -l "$Z"
python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall('.')" "$Z"
ls -d ~/tools/ghidra_*/
