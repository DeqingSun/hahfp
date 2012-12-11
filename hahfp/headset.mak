######################################################################################################
##### CVC VERSIONS
#  on some builds there is a cvc file named 000 which contains additional instructions to be put into
#  flash/rom, this file needs copying to the corresponding cvc folder but is not necessarily present
#  on all builds therefore it is necessary to check the file exists before attempting to copy it to 
#  prevent errors
######################################################################################################

# ensure we always run the rules for each kalimba app to get the latest version
.PHONY : image/cvc_headset_2mic/cvc_headset_2mic.kap \
	 image/cvc_headset_2mic/000 \
     image/hearing_aid_sim/hearing_aid_sim.kap \
	 image/sbc_decoder/sbc_decoder.kap

# 2 mic CVC
image/cvc_headset_2mic/cvc_headset_2mic.kap : 
	$(mkdir) image/cvc_headset_2mic
	$(copyfile) cvc_headset_2mic\image\cvc_headset_2mic\cvc_headset_2mic.kap $@

image.fs : image/cvc_headset_2mic/cvc_headset_2mic.kap

file := $(wildcard cvc_headset_2mic\image\cvc_headset_2mic\000)
ifneq ($(strip $(file)),)

   image/cvc_headset_2mic/000 : 
	$(mkdir) image/cvc_headset_2mic
	$(copyfile) cvc_headset_2mic\image\cvc_headset_2mic\000 $@
   image.fs : image/cvc_headset_2mic/000

endif

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


