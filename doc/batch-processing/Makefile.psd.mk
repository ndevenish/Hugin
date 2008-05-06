#!/usr/bin/make -f

# Creates a multilayer PSD file consisting of both enblend input and output,
# useful for manual retouching in the Gimp or Photoshop.
# 
# Usage for blended panorama:
#   make -f Makefile.psd.mk ldr_psd PTO=myproject.pto
#
# Usage for blended fused stacks:
#   make -f Makefile.psd.mk ldr_fused_psd PTO=myproject.pto

PTTIFF2PSD=PTtiff2psd

include $(PTO).mk

.PHONY : ldr_psd ldr_fused_psd

ldr_psd : $(LDR_LAYERS) $(LDR_BLENDED)
	$(PTTIFF2PSD) -o $(LDR_REMAPPED_PREFIX).psd \
	$(LDR_LAYERS_SHELL) $(LDR_BLENDED_SHELL)

ldr_fused_psd : $(LDR_STACKS) $(LDR_STACKED_BLENDED)
	$(PTTIFF2PSD) -o $(LDR_REMAPPED_PREFIX)_fused.psd \
	$(LDR_STACKS_SHELL) $(LDR_STACKED_BLENDED_SHELL)

