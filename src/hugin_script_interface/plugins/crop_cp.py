#!/usr/bin/env python

from __future__ import print_function

gpl = r"""
    crop_cp.py - remove CPs from parts of the panorama
    
    Copyright (C) 2011  Kay F. Jahnke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

# @category Control Points
# @name     Crop Control Points
# @api-min  2011.1
# @api-max  2013.1

# crop_cps will function as a hugin plugin and as a standalone
# Python script. It can also serve as a template for a slightly
# more involved plugin than the ones in the Examples section.

# Please note that this script tries to use wxPython. This should
# work on Linux systems that have wxPython istalled, but as far as
# I know currently it won't work on Windows, and certainly not on
# Mac OS, where there is currently no hsi/hpi suport. If you want
# to switch off the use of wxPython, set the default parameter in
# the entry function to a value different from None, i.e. change
# def entry ( pano , inside = None ) :
# to
# def entry ( pano , inside = False ) :

import os
import sys
import hsi
import math

# we make an exception for run time errors in this script:

class CropCpError ( Exception ) :

    def __init__ ( self , text ) :
        self.text = text

    def __str__ ( self ) :
        return self.text
    
# some global variables to reduce overhead - we just reuse the same
# FDiff2D objects every time we call the transforms

transform_in = hsi.FDiff2D(0,0)
transform_out = hsi.FDiff2D(0,0)

# transform point coordinates using panotools transforms
# and the global variables for source and target

class to_pano :

    def __init__ ( self , pano , image ) :

        pano_options = pano.getOptions()
        img = pano.getImage ( image )
        w = img.getWidth()
        h = img.getHeight()
        self.wh = w / 2.0
        self.hh = h / 2.0

        # image to pano

        self.tf = hsi.Transform()
        self.tf.createInvTransform ( img , pano_options )

    def transform ( self , p ) :

        # pixel coordinates come in, but have to use center-origin
        
        transform_in.x = p[0] - self.wh
        transform_in.y = p[1] - self.hh

        if self.tf.transform ( transform_out , transform_in ) :
            return ( transform_out.x , transform_out.y )

        # KFJ 2011-06-19 I've noticed the transform failing
        # with images that have X, Y or Z values different from 0.
        # I'm not sure if this is a bug in the transform.
        
        print ( 'transform failed for %s' % str ( p ) )
        return None

# the crop_cps routine is the workhorse of this script. It does the
# actual work of looking at the CPs and removing the unwanted ones.

def crop_cps ( pano , inside ) :

    pano_options = pano.getOptions()

    size = pano_options.getSize()

    # we need these offsets because pano coordinates are center-origin
    
    wh = size.width() / 2.0
    hh = size.height() / 2.0
    
    # that's the ROI of the panorama, in corner-origin
    
    roi = pano_options.getROI()

    # out limits, now in center-origin
    
    left = roi.left() - wh
    right = roi.right() - wh
    top = roi.top() - hh
    bottom = roi.bottom() - hh

    ni = pano.getNrOfImages()
    tflist = []

    # we get a list of CPs in pano coordinates. Note that the pano
    # coordinates are taken from the coordinates in the left image only.
    
    for img in range ( ni ) :
        tflist.append ( to_pano ( pano , img ) )

    # we take the CPVector:
    
    cpv = pano.getCtrlPoints()
    print ( 'found %d CPs' % len ( cpv ) )

    # We want to put the remaining CPs into this CPVector:
    
    ncpv = hsi.CPVector()

    for cp in cpv :
        p = ( cp.x1 , cp.y1 )
        img = cp.image1Nr
        tfp = tflist[img].transform ( p )
        # KFJ 2011-06-17 If the transform fails, keep the CP
        # - best to be defensive.
        if tfp is None :
            ncpv.append ( cp )
        elif inside : # remove CPs inside ROI (or, keep those outside)
            if not ( ( left <= tfp[0] <= right ) and ( top <= tfp[1] <= bottom ) ) :
                ncpv.append ( cp )
        else :
            if ( left <= tfp[0] <= right ) and ( top <= tfp[1] <= bottom ) :
                ncpv.append ( cp )

    pano.setCtrlPoints ( ncpv )
    print ( 'removed %d CPs' % ( len ( cpv ) - len ( ncpv ) ) )

    return 0

# the remainder of the script is administration and deals with running
# it either as a plugin from hugin or as a standalone CLI program.

# first is the entry routine which is called when this script is used
# as a plugin from hugin. If the inside parameter is not passed
# (or passed as 'None') - a dialog will ask whether to remove CPs
# inside or outside of the ROI. If inside is passed as True, CPs
# inside the ROI are removed, otherwise those outside.
# Note that the 'GUI' is primitive - it offers no help or about info,
# no tool tips etc. - the help could be made from the docstrings of
# the CLI version below.

def entry ( pano , inside = None ) :

    if inside is None:

    # if we've been called as a plugin (we're not main) and inside is
    # None, the default, we assume that we're supposed to ask the user
    # whether to remove the CPs from inside or outside the ROI. If
    # inside is passed as True or False, we execute without further ado.

        import wx

        # we need a simple dialog box for this purpose which displays
        # a radio button for the inside/outside choice, plus Okay and Cancel.
        
        class crop_cp_dialog ( wx.Dialog ) :

            # excuse my clumsy wxPython ;-)
            
            def __init__(self, parent, title):

                wx.Dialog.__init__(self, parent, -1, title, size = ( 200 , 150 ) )
                choices = [ 'outside ROI' , 'inside ROI' ]
                # default choice is to remove CPs outdide ROI
                self.choice = 0

                # we create and position the UI elements                
                sizer = wx.BoxSizer ( wx.VERTICAL )
                rb = wx.RadioBox(
                        self, -1, "", wx.DefaultPosition, wx.DefaultSize,
                        choices, 2, wx.RA_SPECIFY_COLS
                        )
                self.Bind(wx.EVT_RADIOBOX, self.EvtRadioBox, rb)
                sizer.Add(rb, 0, wx.ALL, 20)
                btnsizer = wx.StdDialogButtonSizer()
                btn = wx.Button(self, wx.ID_OK)
                btn.SetDefault()
                btnsizer.AddButton(btn)
                btn = wx.Button(self, wx.ID_CANCEL)
                btnsizer.AddButton(btn)
                btnsizer.Realize()
                sizer.Add(btnsizer, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
                self.SetSizer(sizer)

            # what to do when the radio button is operated:
            def EvtRadioBox(self, event):
                self.choice = event.GetInt()
        
        # make the dialog and show it modally:
        ccd = crop_cp_dialog ( None , 'Crop CPs' )
        retval = ccd.ShowModal()
        # only if Okay was clicked the state of the radio button is used
        if retval == wx.ID_OK :
            if ccd.choice == 0 :
                inside = False
            else :
                inside = True
        # otherwise we cancel the operation and return.
        else :
            return 0

    # now the preliminaries are sorted, we can call the workhorse routine.
    
    return crop_cps ( pano , inside )
    
# the main routine is what's called if this script has been called from
# the command line. In this case we do the CLI thing, so we parse the
# argument vector and act accordingly.

def main() :

    # when called from the command line, we import the argparse module.
    # This may not be part of the standard library on your system.
    
    import argparse

    # and create an argument parser.
    
    parser = argparse.ArgumentParser (
        formatter_class=argparse.RawDescriptionHelpFormatter ,
        description = gpl + '''
    crop_cp will remove all CPs which are either outside the panorama's ROI
    or inside it's ROI if the inside flag is set
    ''' )

    parser.add_argument('-o', '--output',
                        metavar='<output file>',
                        default = None,
                        type=str,
                        help='write output to a different file')

    parser.add_argument('-i', '--inside',
                        action='store_true',
                        default = False,
                        help='remove CPs inside ROI')

    parser.add_argument('input' ,
                        metavar = '<pto file>' ,
                        type = str ,
                        help = 'pto file to be processed' )

    if len ( sys.argv ) < 2 :
        parser.print_help()
        return

    # we parse the arguments into a global variable so we can access
    # them from everywhere without having to pass them around
    
    global args
    
    args = parser.parse_args()

    if args.output :
        print ( 'output: ' , args.output )
    print ( 'ptofile:' , args.input )

    # first we see if we can open the input file

    ifs = hsi.ifstream ( args.input )
    if not ifs.good() :
        raise CropCpError ( 'cannot open input file %s' % args.input )

    pano = hsi.Panorama()
    success = pano.readData ( ifs )
    del ifs
    if success != hsi.DocumentData.SUCCESSFUL :
        raise CropCpError ( 'input file %s contains invalid data' % args.input )

    # if a separate output file was chosen, we open it now to avoid
    # later disappointments
    
    if args.output:
        ofs = hsi.ofstream ( args.output )
        if not ofs.good() :
            raise CropCpError ( 'cannot open output file %s' % args.output )

    # we've checked the args, now we call crop_cps()
    
    crop_cps ( pano , args.inside )

    # if no different output file was specified, overwrite the input
    
    if not args.output :
        ofs = hsi.ofstream ( args.input )
        if not ofs.good() :
            raise CropCpError ( 'cannot open file %s for output' % args.input )

    success = pano.writeData ( ofs )
    del ofs
    if success != hsi.DocumentData.SUCCESSFUL :
        raise CropCpError ( 'error writing pano to %s' % args.input )
    
    # done.
    
    return 0

# are we main? This means we've been called from the command line
# as a standalone program, so we have to go through the main() function.

if __name__ == "__main__":
    try:
        main()
    except CropCpError as e :
        print ( 'Run Time Error: %s' % e )
