# since this plugin actually references members of the hsi
# module (like, hsi.calcCtrlPointErrors below) we need to
# import hsi to make these names known:

import hsi

# another demo plugin: this one will keep at most five control
# points per image pair. Selection is by CP error, the points
# with the smallest error are kept.

def entry ( pano , *args ) :

    # this plugin doesn't take any extra arguments

    if args :
        print('ignoring extra arguments %s' % str(args))

    # on loading the CP errors are unknown - we have to have them
    # calculated before we can start:
    
    hsi.calcCtrlPointErrors(pano)

    # First we find out what pairs there are. We make a set
    # of these pairs and use them as index in a dictionary
    # with an empty list as value
    
    cpv = pano.getCtrlPoints()
    pairs = set ( ( cp.image1Nr , cp.image2Nr ) for cp in cpv )
    cpdict = dict ( ( pair , [] ) for pair in pairs )

    # now each CP goes into the list indexed by the pair of
    # images it connects - but in a tuple with the error in front
    # se we can easily sort by error
    
    for cp in cpv :
        cpdict[(cp.image1Nr, cp.image2Nr)].append((cp.error, cp))

    # we extract the five values with the smallest error
    # from every list. The CPs in there are our top five,
    # so we put them into cpv
    
    cpv = []
    for pair in pairs :
        top_five = [ t[1] for t in sorted(cpdict[pair])[:5] ]
        cpv.extend ( top_five )

    # finally we attach the new CP vector to the panorama
    
    pano.setCtrlPoints ( cpv )
    
    return 0

