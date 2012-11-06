######################################################################################################
##### AUDIO ADAPTOR
#####
##### Copy DSP images depending on MP3 encoder configuration
######################################################################################################

.PHONY : image/sbc_encoder/sbc_encoder.kap  \
	image/sbc_encoder/000  

# the sbc_encoder algorithms come from the dsp apps included as part of the project
image/sbc_encoder/sbc_encoder.kap : 
	$(mkdir) image/sbc_encoder
	$(copyfile) audio_adaptor\image\sbc_encoder\sbc_encoder.kap $@
image/sbc_encoder/000 : 
	$(mkdir) image/sbc_encoder
	$(copyfile) audio_adaptor\image\sbc_encoder\000 $@	
	
image.fs : image/sbc_encoder/sbc_encoder.kap  \
	image/sbc_encoder/000  
	
