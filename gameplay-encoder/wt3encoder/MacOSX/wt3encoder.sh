#!/bin/sh

set -e

usage() {
cat <<EOF

Usage: 	$0 [-i model_inupt_file] [-o model_output_file]
Encodes .FBX or .DAE 3d model file to .WT3

OPTIONS:
    -i  Filepath to the model file (e.g. "/home/username/teapot/teapot.fbx").
    -o  Filepath to the wt3 output file  (e.g. "/home/username/teapot/teapot.wt3").
    -h  displays this usage text

EOF
}

while getopts hi:o: OPTION
do
   case $OPTION in
   		h) 	usage
		   	exit 1;;
   		i) IN_ARG=`basename $OPTARG`;;
		o) OUT_ARG="$OPTARG";;
		\?)	usage
		     exit 1;;
		*)	usage
		    exit 1;;
   esac
done

#save current path
export ORIG_PATH=$PWD
export EXEC_PATH=`sh readlink.sh $(dirname $0)`

# sh Wikitude3dEncoder.sh <ABSOLUTE_MODELPATH> <OUTPUT_NAME> <OUTPUT_PATH>
# example: 
# sh Wikitude3dEncoder.sh ./Documents/butterfly/butterfly.fbx ./Documents/out/butterfly.wt3
export ROOT_MODEL_PATH=`sh readlink.sh $(dirname $IN_ARG)`
export MODEL_FILENAME=`basename $IN_ARG`
export MODEL_FILEPATH="$ROOT_MODEL_PATH/$MODEL_FILENAME"
export OUT_FILENAME=`basename $OUT_ARG`
export OUT_PATH=`sh readlink.sh $(dirname $OUT_ARG)`
export OUT_FILEPATH="$OUT_PATH/$OUT_FILENAME"

# create missing directories
if [ ! -d "$ROOT_MODEL_PATH/model" ]; then
	mkdir -p "$ROOT_MODEL_PATH/model"
fi

if [ ! -d "$ROOT_MODEL_PATH/model/tex" ]; then
	mkdir -p "$ROOT_MODEL_PATH/model/tex"
fi

# encode 3d-model
export LOG=$( (`"$EXEC_PATH/encoder" -gy -tex "$ROOT_MODEL_PATH/model/tex" -m "$ROOT_MODEL_PATH/model/model.material" -scene "$ROOT_MODEL_PATH/model/model.scene" "$MODEL_FILEPATH" "$ROOT_MODEL_PATH/model/model.gpb"`) 2>&1)

cp "$EXEC_PATH/version" "$ROOT_MODEL_PATH/model"

if grep -q Error <<< $LOG; then
	cd "$ROOT_MODEL_PATH"
	rm -Rf ./model
	cd "$ORIG_PATH"
	echo "$LOG"
	exit 1
fi

if [ ! -d "$OUT_PATH" ]; then
	mkdir -p "$OUT_PATH"
fi

# create .wt3
cd "$ROOT_MODEL_PATH"
zip -q -r "./$OUT_FILENAME" "./model"
rm -Rf ./model
mv "$OUT_FILENAME" "$OUT_FILEPATH"

cd "$ORIG_PATH"
exit 0
