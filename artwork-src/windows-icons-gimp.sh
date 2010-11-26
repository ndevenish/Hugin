#!/bin/bash

gimp -i -b - <<EOF
(
;Iconify.scm
;===========================
;Author: Giuseppe Bilotta
;Modified for Gimp 2.4.6+ by Ouch67
;http://registry.gimp.org/files/iconify2.scm_0.txt
;Modified to generate Hugin icons for Windows in batch mode by Matthew Petroff
;Description: Iconify plug-in converts a single layer of a single image into a multi-layered image ready to be saved as a Windows icon.
;The new image will contain all standard sizes (16x16, 32x32, 48x48) at all standard bit depths (16 colors, 256 colors, 32-bit RGBA), with transparency support.
;===========================

; Create a new image. We size it to 256x256 to allow for
; the future Windows Vista icons; it's also easy to add
; 128x128 Macintosh icons, or other sizes
(let* (
       (img (car (gimp-file-load RUN-NONINTERACTIVE "${ICONV}" "${ICONV}")))
       (drawable (car (gimp-image-get-active-layer img)))
       (new-img (car (gimp-image-new 256 256 0)))
       (new-name 0)
       (work-layer 0)
       (layer-x 0)
       (layer-y 0)
       (max-dim 0)
       (temp-layer 0)

       (temp-img 0)
       (layers 0)
       (layernum 0)
       (layers-array 0)
       (layer 0)
       (eigth-bit 0)
       (four-bit 0)
       )
; Set the name of the new image by replacing the extension with .ico
; FIXME this doesn't work as intended for files without extension
; or files with multiple extensions.
(set! new-name
(append
(butlast
(strbreakup (car (gimp-image-get-filename img)) ".")
)
'(".ico")
)
)
(set! new-name (eval (cons string-append new-name)))
(gimp-image-set-filename new-img new-name)

; Create a new layer
(set! work-layer (car (gimp-layer-new-from-drawable drawable new-img)))

; Give it a name
(gimp-layer-set-name work-layer "Work layer")

; Add the new layer to the new image
(gimp-image-add-layer new-img work-layer 0)

; Autocrop the layer
(plug-in-autocrop-layer 1 new-img work-layer)

; Now, resize the layer so that it is square,
; by making the shorter dimension the same as
; the longer one. The layer content is centered.
(set! layer-x (car (gimp-drawable-width work-layer)))
(set! layer-y (car (gimp-drawable-height work-layer)))
(set! max-dim (max layer-x layer-y))
(gimp-layer-resize work-layer max-dim max-dim (/ (- max-dim layer-x) 2) (/ (- max-dim layer-y) 2))

; Move the layer to the origin of the image
(gimp-layer-set-offsets work-layer 0 0)

; Now, we create as many layers as needed, resizing to
; 16x16, 32x32, 48x48, 128x128, 256x256

(define (resize-to-dim dim)

(set! temp-layer (car (gimp-layer-copy work-layer 0)))
(gimp-image-add-layer new-img temp-layer 0)
(gimp-drawable-transform-scale temp-layer 0 0 dim dim 0 2 1 3 0)
)

; We don't do the two biggest sizes, for now
; TODO make this a parameter
(mapcar resize-to-dim '(16 32 48 128))

; We can now get rid of the working layer
(gimp-image-remove-layer new-img work-layer)

; These two functions allow us to create new layers which are
; clones of the existing ones but at different color depths.
; We have to use two functions and pass through intermediate
; images because otherwise the second color reduction would dupe
; the layers, thus giving an unneeded extra set of layers
; TODO a potential study should be done on whether it's better
; to go straight to the lowest number of color (as we do), or
; passing through intermediate number of colors.
; Observe that no dithering is done. This is intentional, since
; it gives the best results.
(define (palettize-image pal-type num)
(set! temp-img (car (gimp-image-duplicate new-img)))
(gimp-image-convert-indexed temp-img 0 pal-type num 0 1 "")
temp-img)
(define (plop-image temp-img)
(set! layers (gimp-image-get-layers temp-img))
(set! layernum (car layers))
(set! layers-array (cadr layers))
(while (> layernum 1)
(set! layer (car
(gimp-layer-new-from-drawable
(aref layers-array (- layernum 1)) new-img)
)
)
(gimp-image-add-layer new-img layer 0)
(set! layernum (- layernum 1))
)
)

; The 256 color image, using an optimized palette
(set! eigth-bit (palettize-image 0 256))
; The 16 color image, using the 'official' 'web' optimized palette
; It must be done with that palette because otherwise we can't
; save to 16-color in Windows .ico format
(set! four-bit (palettize-image 2 16))

; Now we put the new layers back in the original image
(plop-image eigth-bit)
;(plop-image four-bit)

; We display the new image
;(gimp-display-new new-img)

; And we flush the display
;(gimp-displays-flush)

; Save .ico
(file-ico-save 0 new-img drawable "${P}${O_WIN}" "${P}${O_WIN}")
)

; Quit GIMP
(gimp-quit 0)
)
EOF
