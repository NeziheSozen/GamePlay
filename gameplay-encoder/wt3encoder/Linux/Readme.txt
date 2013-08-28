To build the encoder:

- install fbx-sdk 2013.3
- cmake CMakeList.txt
- include library to CMakeFiles/encoder.dir/link.txt: -lfbxsdk-2013.3-static
(for some reason the CMakeList.txt doesnt write it into the link.txt file, maybe because of the '-')
- make all

Create a binary from the shell script:
- install shc
- shc -f wt3encoder.sh
- rename wt3encoder.sh.x to wt3encoder