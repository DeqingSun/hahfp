SET BUILD_ENV=D:\CSR_SDK\Audio-Adaptor-SDK-2009.R1

cd lib
call rebuildVmlib.bat
cd ..

%BUILD_ENV%/tools/bin/make -R BLUELAB=%BUILD_ENV%/tools -f audio_adaptor.batch.mak build
@if errorlevel == 1 goto end
%BUILD_ENV%/tools/bin/BlueFlashCmd -TRANS "SPITRANS=USB SPIPORT=0" merge

:end
