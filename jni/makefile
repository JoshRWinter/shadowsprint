apk=shadow-debug.apk

all:../assets/assets ../assets/uiassets ../assets/aassets
	ndk-build -C ..
	ant debug -f ../build.xml
	adb install -r $(CURDIR)/../bin/$(apk)
	
../assets/assets:../assets_local/build.txt ../assets_local/*.tga
	cd ../assets_local/ && pack --build

../assets/uiassets:../assets_local/uibuild.txt ../assets_local/*.tga
	cd ../assets_local/ && pack --build uibuild.txt

../assets/aassets:../assets_local/apack.txt ../assets_local/*.ogg
	cd ../assets_local/ && apack --build

