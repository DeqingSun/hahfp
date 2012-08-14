######################################################################################################
##### CVC VERSIONS
#  on some builds there is a cvc file named 000 which contains additional instructions to be put into
#  flash/rom, this file needs copying to the corresponding cvc folder but is not necessarily present
#  on all builds therefore it is necessary to check the file exists before attempting to copy it to 
#  prevent errors
######################################################################################################

# ensure we always run the rules for each kalimba app to get the latest version
.PHONY : image/cvc_headset/cvc_headset.kap \
     image/hearing_aid_sim/hearing_aid_sim.kap \
	 image/sbc_decoder/sbc_decoder.kap

#  1 mic CVC
image/cvc_headset/cvc_headset.kap : 
	$(mkdir) image/cvc_headset
	$(copyfile) cvc_headset\image\cvc_headset\cvc_headset.kap $@

image.fs : image/cvc_headset/cvc_headset.kap

image/hearing_aid_sim/hearing_aid_sim.kap : 
	$(mkdir) image/hearing_aid_sim
	$(copyfile) hearing_aid_sim\image\hearing_aid_sim\hearing_aid_sim.kap $@

image.fs : image/hearing_aid_sim/hearing_aid_sim.kap

######################################################################################################
##### A2DP DECODER VERSIONS
######################################################################################################

# copy in sbc decoder 
image/sbc_decoder/sbc_decoder.kap : 
	$(mkdir) image/sbc_decoder
	$(copyfile) a2dp_headset\image\sbc_decoder\sbc_decoder.kap $@

image.fs : image/sbc_decoder/sbc_decoder.kap


