// -*- c-basic-offset: 4 -*-
/** @file 
 *
 * !! from PTOptimise.h 1951
 *
 *  functions to call the optimizer of panotools.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.h 1951 2007-04-15 20:54:49Z dangelo $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <boost/foreach.hpp>
#include "PTOptimizer.h"

#include "ImageGraph.h"
#include "panodata/StandardImageVariableGroups.h"
#include <panotools/PanoToolsOptimizerWrapper.h>
#include <panotools/PanoToolsInterface.h>
#include <algorithms/basic/CalculateCPStatistics.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/nona/CalculateFOV.h>
#include <vigra_ext/ransac.h>

#if DEBUG
#include <fstream>
#include <boost/graph/graphviz.hpp>
#endif

namespace HuginBase {

using namespace hugin_utils;

bool PTOptimizer::runAlgorithm()
{
    PTools::optimize(o_panorama);
    return true; // let's hope so.
}

// small helper class
class OptVarSpec
{
public:
    OptVarSpec(int img, std::string name)
	: m_img(img), m_name(name)
    {
    }

    double get(PanoramaData & pano) const
    {
	return pano.getImage(m_img).getVar(m_name);
    }
    void set(PanoramaData & pano, double x) const
    {
	pano.updateVariable(m_img,Variable(m_name,x));
    }
    int m_img;
    std::string m_name;
};

/** Estimator for RANSAC based adjustment of pairwise parameters */
class PTOptEstimator
{

public:

    PTOptEstimator(PanoramaData & pano, int i1, int i2, double maxError,
		   bool optHFOV, bool optB)
    {
	m_maxError = maxError;

	UIntSet imgs;
	imgs.insert(i1);
	imgs.insert(i2);
	m_localPano = (pano.getNewSubset(imgs)); // don't forget to delete
	m_li1 = (i1 < i2) ? 0 : 1;
	m_li2 = (i1 < i2) ? 1 : 0;
	// get control points
	m_cps  = m_localPano->getCtrlPoints();
	// only use 2D control points
	BOOST_FOREACH(ControlPoint & kp, m_cps) {
	    if (kp.mode == ControlPoint::X_Y) {
		m_xy_cps.push_back(kp);
	    }
	}
	
	if (optHFOV)
	    m_optvars.push_back(OptVarSpec(0,std::string("v")));
	if (optB)
	    m_optvars.push_back(OptVarSpec(0,std::string("b")));
	m_optvars.push_back(OptVarSpec(m_li2,"r"));
	m_optvars.push_back(OptVarSpec(m_li2,"p"));
	m_optvars.push_back(OptVarSpec(m_li2,"y"));
	if (optHFOV)
	    m_optvars.push_back(OptVarSpec(0,"v"));
	if (optB)
	    m_optvars.push_back(OptVarSpec(0,"b"));

	/** optimisation for first pass */
	m_opt_first_pass.resize(2);
	m_opt_first_pass[1].insert("r");
	m_opt_first_pass[1].insert("p");
	m_opt_first_pass[1].insert("y");

	/** optimisation for second pass */
	if (optHFOV || optB) {
	    m_opt_second_pass = m_opt_first_pass;
	    if (optHFOV)
		m_opt_second_pass[0].insert("v");
	    if (optB)
		m_opt_second_pass[0].insert("b");
	}

	// number of points required for estimation
	m_numForEstimate = (m_optvars.size()+1)/2;	
			    
	// extract initial parameters from pano
	m_initParams.resize(m_optvars.size());
	int i=0;
	BOOST_FOREACH(OptVarSpec & v, m_optvars) {
	    m_initParams[i] = v.get(*m_localPano);
	    DEBUG_DEBUG("get init var: " << v.m_name << ", " << v.m_img << ": " << m_initParams[i]);
	    i++;
	}
     }

			    

    /** Perform exact estimate. 
     *
     *  This is actually a fake and just calles leastSquaresEstimate, as I don't know a
     *  closed form solution for fisheye images...
     */
    bool estimate(const std::vector<const ControlPoint *> & points, std::vector<double> & p) const
    {
	// reset to the initial parameters.
	p.resize(m_initParams.size());
	std::copy(m_initParams.begin(), m_initParams.end(), p.begin());

	return leastSquaresEstimate(points, p);
    }

			    

    bool leastSquaresEstimate(const std::vector<const ControlPoint *> & points, std::vector<double> & p) const 
    {
	// copy points into panorama object
	CPVector cpoints(points.size());	
	for (int i=0; i < points.size(); i++) {
	    cpoints[i] = *points[i];
	}

	m_localPano->setCtrlPoints(cpoints);

	PanoramaData * pano = const_cast<PanoramaData *>(m_localPano);
	// set parameters in pano object
	int i=0;
	BOOST_FOREACH(const OptVarSpec & v, m_optvars) {
	    v.set(*pano, p[i]);
	    DEBUG_DEBUG("Initial " << v.m_name <<  ": i1:" << pano->getImage(m_li1).getVar(v.m_name) << ", i2: " << pano->getImage(m_li2).getVar(v.m_name));
	    i++;
	}

	m_localPano->setOptimizeVector(m_opt_first_pass);
	// optimize parameters using panotools (or use a custom made optimizer here?)
	UIntSet imgs;
	imgs.insert(0);
	imgs.insert(1);
	//std::cout << "Optimizing without hfov:" << std::endl;
	//pano->printPanoramaScript(std::cerr, m_localPano->getOptimizeVector(), pano->getOptions(), imgs, true );
	PTools::optimize(*pano);
	//std::cout << "result:" << std::endl;
	//pano->printPanoramaScript(std::cerr, m_localPano->getOptimizeVector(), pano->getOptions(), imgs, true );

	if (m_opt_second_pass.size() > 0) {
	    m_localPano->setOptimizeVector(m_opt_second_pass);
	    //std::cout << "Optimizing with hfov" << std::endl;
	    //pano->printPanoramaScript(std::cerr, m_localPano->getOptimizeVector(), pano->getOptions(), imgs, true );
	    PTools::optimize(*pano);
	    //std::cout << "result:" << std::endl;
	    //pano->printPanoramaScript(std::cerr, m_localPano->getOptimizeVector(), pano->getOptions(), imgs, true );
	}

	// get optimized parameters
	i=0;
	BOOST_FOREACH(const OptVarSpec & v, m_optvars) {
	    p[i] = v.get(*pano);
	    DEBUG_DEBUG("Optimized " << v.m_name <<  ": i1:" << pano->getImage(m_li1).getVar(v.m_name) << ", i2: " << pano->getImage(m_li2).getVar(v.m_name));
	    i++;
	}
	return true;
    }


    bool agree(std::vector<double> &p, const ControlPoint & cp) const
    {
	PanoramaData * pano = const_cast<PanoramaData *>(m_localPano);
	// set parameters in pano object
	int i=0;
	BOOST_FOREACH(const OptVarSpec & v, m_optvars) {
	    v.set(*pano, p[i]);
	    i++;
	}
	// TODO: argh, this is slow, we should really construct this only once
	// and reuse it for all calls...
	PTools::Transform trafo_i1_to_pano;
	trafo_i1_to_pano.createInvTransform(m_localPano->getImage(m_li1),m_localPano->getOptions());
	PTools::Transform trafo_pano_to_i2;
	trafo_pano_to_i2.createTransform(m_localPano->getImage(m_li2),m_localPano->getOptions());

	double x1,y1,x2,y2,xt,yt,x2t,y2t;
	if (cp.image1Nr == m_li1) {
	    x1 = cp.x1;
	    y1 = cp.y1;
	    x2 = cp.x2;
	    y2 = cp.y2;
	} else {
	    x1 = cp.x2;
	    y1 = cp.y2;
	    x2 = cp.x1;
	    y2 = cp.y1;
	}   
	trafo_i1_to_pano.transformImgCoord(xt, yt, x1, y1);
	trafo_pano_to_i2.transformImgCoord(x2t, y2t, xt, yt);
	DEBUG_DEBUG("Trafo i1 (0 " << x1 << " " << y1 << ") -> ("<< xt <<" "<< yt<<") -> i2 (1 "<<x2t<<", "<<y2t<<"), real ("<<x2<<", "<<y2<<")")
	// compute error in pixels...
	x2t -= x2;
	y2t -= y2;
	double  e = hypot(x2t,y2t);
	DEBUG_DEBUG("Error ("<<x2t<<", "<<y2t<<"), " << e)
	return  e < m_maxError;
    }

    ~PTOptEstimator()
    {
	delete m_localPano;
    }

    int numForEstimate() const
    {
	return m_numForEstimate;
    }
	
public:
    CPVector m_xy_cps;
    std::vector<double> m_initParams;
    std::vector<OptVarSpec> m_optvars;

private:
    int m_li1, m_li2;
    double m_maxError;
    PanoramaData * m_localPano;
    CPVector m_cps;    
    std::vector<std::set<std::string> > m_opt_first_pass;
    std::vector<std::set<std::string> > m_opt_second_pass;
    int m_numForEstimate;
};


std::vector<int> RANSACOptimizer::findInliers(PanoramaData & pano, int i1, int i2, double maxError, Mode rmode)
{
    bool optHFOV = false;
    bool optB = false;
    switch (rmode) {
    case HOMOGRAPHY:
    case RPYV:
	optHFOV =  true;
	break;
    case RPYVB:
	optHFOV = true;
	optB = true;
    case AUTO:
    case RPY:
	break;
    }

    DEBUG_DEBUG("Optimizing HFOV:" << optHFOV << " b:" << optB)
    PTOptEstimator estimator(pano, i1, i2, maxError, optHFOV, optB);

    std::vector<double> parameters(estimator.m_initParams.size());
    std::copy(estimator.m_initParams.begin(),estimator.m_initParams.end(), parameters.begin());
    std::vector<int> inlier_idx;
    DEBUG_DEBUG("Number of control points: " << estimator.m_xy_cps.size() << " Initial parameter[0]" << parameters[0]);
    std::vector<const ControlPoint *> inliers = Ransac::compute(parameters, inlier_idx, estimator, estimator.m_xy_cps, 0.999, 0.3);
    DEBUG_DEBUG("Number of inliers:" << inliers.size() << "optimized parameter[0]" << parameters[0]);

    // set parameters in pano object
    int i=0;
    BOOST_FOREACH(const OptVarSpec & v, estimator.m_optvars) {
	// TODO: check when to use i1..
	pano.updateVariable(i2, Variable(v.m_name, parameters[i]));
	i++;
    }
    
    
    // TODO: remove bad control points from pano
    return inlier_idx;
}    
    

bool RANSACOptimizer::runAlgorithm()
{
    o_inliers = findInliers(o_panorama, o_i1, o_i2, o_maxError, o_mode);
    return true; // let's hope so.
}
    

void AutoOptimise::autoOptimise(PanoramaData& pano, bool optRoll)
{
    // DGSW FIXME - Unreferenced
    //	unsigned nImg = unsigned(pano.getNrOfImages());
    // build a graph over all overlapping images
    CPGraph graph;
    createCPGraph(pano,graph);
    
#if DEBUG
    {
        std::ofstream gfile("cp_graph.dot");
        // output doxygen graph
        boost::write_graphviz(gfile, graph);
    }
#endif
    std::set<std::string> optvars;
    if(optRoll)
    {
        optvars.insert("r");
    };
    optvars.insert("p");
    optvars.insert("y");
    
    unsigned int startImg = pano.getOptions().optimizeReferenceImage;
    
    // start a breadth first traversal of the graph, and optimize
    // the links found (every vertex just once.)
    
    OptimiseVisitor optVisitor(pano, optvars);
    
    boost::queue<boost::graph_traits<CPGraph>::vertex_descriptor> qu;
    boost::breadth_first_search(graph, startImg,
                                color_map(get(boost::vertex_color, graph)).
                                visitor(optVisitor));
    /*
#ifdef DEBUG
     // print optimized script to cout
     DEBUG_DEBUG("after local optim:");
     VariableMapVector vars = optVisitor.getVariables();
     for (unsigned v=0; v < pano.getNrOfImages(); v++) {
         printVariableMap(std::cerr, vars[v]);
         std::cerr << std::endl;
     }
#endif
     
     // apply variables to input panorama
     pano.updateVariables(optVisitor.getVariables());
     
#ifdef DEBUG
     UIntSet allImg;
     fill_set(allImg,0, pano.getNrOfImages()-1);
     // print optimized script to cout
     DEBUG_DEBUG("after updateVariables():");
     pano.printPanoramaScript(std::cerr, pano.getOptimizeVector(), pano.getOptions(), allImg, false);
#endif
     */
}


void SmartOptimise::smartOptimize(PanoramaData& optPano)
{
    // use m-estimator with sigma 2
    PanoramaOptions opts = optPano.getOptions();
    double oldSigma = opts.huberSigma;
    opts.huberSigma = 2;
    optPano.setOptions(opts);
    
    // remove vertical and horizontal control points
    CPVector cps = optPano.getCtrlPoints();
    CPVector newCP;
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); it++) {
        if (it->mode == ControlPoint::X_Y)
        {
            newCP.push_back(*it);
        }
    }
    optPano.setCtrlPoints(newCP);
    AutoOptimise::autoOptimise(optPano);
    
    // do global optimisation of position with all control points.
    optPano.setCtrlPoints(cps);
    OptimizeVector optvars = createOptVars(optPano, OPT_POS, optPano.getOptions().optimizeReferenceImage);
    optPano.setOptimizeVector(optvars);
    PTools::optimize(optPano);
    
    //Find lenses.
    StandardImageVariableGroups variable_groups(optPano);
    
    // allow parameter evaluation.
    // this could be probably done in better way because this
    // will optimize them also in case they are intentionally set to 0
    double origLensPar[5];
    origLensPar[0] = const_map_get(optPano.getImageVariables(0),"a").getValue();
    origLensPar[1] = const_map_get(optPano.getImageVariables(0),"b").getValue();
    origLensPar[2] = const_map_get(optPano.getImageVariables(0),"c").getValue();
    origLensPar[3] = const_map_get(optPano.getImageVariables(0),"d").getValue();
    origLensPar[4] = const_map_get(optPano.getImageVariables(0),"e").getValue();
    bool alreadyCalibrated = false;
    for (int i = 0; i < 5; i++) {
    	if (origLensPar[i] != 0) {
    		alreadyCalibrated = true;
    		break;
    	}
    }
    // check if lens parameter values were loaded from ini file
    // and should not be changed
    if (!alreadyCalibrated) {
        //---------------------------------------------------------------
        // Now with lens distortion
        
        // force inherit for all d/e values
        for (unsigned i=0; i< variable_groups.getLenses().getNumberOfParts(); i++)
        {
            variable_groups.getLenses().linkVariablePart(ImageVariableGroup::IVE_RadialDistortion, i);
            variable_groups.getLenses().linkVariablePart(ImageVariableGroup::IVE_RadialDistortionCenterShift, i);
        }
        
        int optmode = OPT_POS;
        double origHFOV = const_map_get(optPano.getImageVariables(0),"v").getValue();
        
        // determine which parameters to optimize
        double rmin, rmax, rmean, rvar, rq10, rq90;
        CalculateCPStatisticsRadial::calcCtrlPntsRadiStats(optPano, rmin, rmax, rmean, rvar, rq10, rq90);
        
        DEBUG_DEBUG("Ctrl Point radi statistics: min:" << rmin << " max:" << rmax << " mean:" << rmean << " var:" << rvar << " q10:" << rq10 << " q90:" << rq90);
        
        if (origHFOV > 60) {
            // only optimize principal point if the hfov is high enough for sufficient perspective effects
            optmode |= OPT_DE;
        }
        
        // heuristics for distortion and fov optimisation
        if ( (rq90 - rq10) >= 1.2) {
            // very well distributed control points 
            // TODO: other criterion when to optimize HFOV, too
            optmode |= OPT_AC | OPT_B;
        } else if ( (rq90 - rq10) > 1.0) {
            optmode |= OPT_AC | OPT_B;
        } else {
            optmode |= OPT_B;
        }
        
        // check if this is a 360 deg pano.
        CenterHorizontally(optPano).run();
        //FDiff2D fov = CalculateFOV(optPano).run<CalculateFOV>().getResultFOV();
            FDiff2D fov = CalculateFOV::calcFOV(optPano);
        
        if (fov.x >= 359) {
            // optimize HFOV for 360 deg panos
            optmode |= OPT_HFOV;
        }
        
        DEBUG_DEBUG("second optimization: " << optmode);

        // save old variables, might be needed if optimization went wrong
        VariableMapVector oldVars = optPano.getVariables();
        DEBUG_DEBUG("oldVars[0].b: " << const_map_get(oldVars[0],"b").getValue());
        optvars = createOptVars(optPano, optmode, optPano.getOptions().optimizeReferenceImage);
        optPano.setOptimizeVector(optvars);
        // global optimisation.
        DEBUG_DEBUG("before opt 1: newVars[0].b: " << const_map_get(optPano.getVariables()[0],"b").getValue());
        PTools::optimize(optPano);
        // --------------------------------------------------------------
        // do some plausibility checks and reoptimize with less variables
        // if something smells fishy
        bool smallHFOV = false;
        bool highDist = false;
        bool highShift = false;
        const VariableMapVector & vars = optPano.getVariables();
        DEBUG_DEBUG("after opt 1: newVars[0].b: " << const_map_get(vars[0],"b").getValue());
        DEBUG_DEBUG("after opt 1: oldVars[0].b: " << const_map_get(oldVars[0],"b").getValue());
        for (VariableMapVector::const_iterator it = vars.begin() ; it != vars.end(); it++)
        {
            if (const_map_get(*it,"v").getValue() < 1.0) smallHFOV = true;
            if (fabs(const_map_get(*it,"a").getValue()) > 0.8) highDist = true;
            if (fabs(const_map_get(*it,"b").getValue()) > 0.8) highDist = true;
            if (fabs(const_map_get(*it,"c").getValue()) > 0.8) highDist = true;
            if (fabs(const_map_get(*it,"d").getValue()) > 2000) highShift = true;
            if (fabs(const_map_get(*it,"e").getValue()) > 2000) highShift = true;
        }

        if (smallHFOV || highDist || highShift) {
            DEBUG_DEBUG("Optimization with strange result. status: HFOV: " << smallHFOV << " dist:" << highDist << " shift:" << highShift);
            // something seems to be wrong
            if (smallHFOV) {
                // do not optimize HFOV
                optmode &= ~OPT_HFOV;
            }
            if (highDist) {
                optmode &= ~OPT_AC;
            }
            if (highShift) {
                optmode &= ~OPT_DE;
            }
            
            // revert and redo optimisation
            DEBUG_DEBUG("oldVars[0].b: " << const_map_get(oldVars[0],"b").getValue());
            optPano.updateVariables(oldVars);
            DEBUG_DEBUG("before opt 2: newVars[0].b: " << const_map_get(optPano.getVariables()[0],"b").getValue());
            optvars = createOptVars(optPano, optmode, optPano.getOptions().optimizeReferenceImage);
            optPano.setOptimizeVector(optvars);
            DEBUG_DEBUG("recover optimisation: " << optmode);
            // global optimisation.
            PTools::optimize(optPano);
    
            // check again, maybe b shouldn't be optimized either
            bool highDist = false;
            const VariableMapVector & vars = optPano.getVariables();
            DEBUG_DEBUG("after opt 2: newVars[0].b: " << const_map_get(vars[0],"b").getValue());
            DEBUG_DEBUG("after opt 2: oldVars[0].b: " << const_map_get(oldVars[0],"b").getValue());
            for (VariableMapVector::const_iterator it = vars.begin() ; it != vars.end(); it++)
            {
                if (fabs(const_map_get(*it,"b").getValue()) > 0.8) highDist = true;
            }
            if (highDist) {
                optmode &= ~OPT_B;
                DEBUG_DEBUG("recover optimisation (2): " << optmode);
                // revert and redo optimisation
                optPano.updateVariables(oldVars);
                DEBUG_DEBUG("before opt 3: newVars[0].b: " << const_map_get(optPano.getVariables()[0],"b").getValue());
                optvars = createOptVars(optPano, optmode, optPano.getOptions().optimizeReferenceImage);
                optPano.setOptimizeVector(optvars);
                // global optimisation.
                PTools::optimize(optPano);
                const VariableMapVector & vars = optPano.getVariables();
                DEBUG_DEBUG("after opt 3: newVars[0].b: " << const_map_get(vars[0],"b").getValue());
                DEBUG_DEBUG("after opt 3: oldVars[0].b: " << const_map_get(oldVars[0],"b").getValue());

            }
        }
    }
    opts.huberSigma = oldSigma;
    optPano.setOptions(opts);
}

OptimizeVector SmartOptimizerStub::createOptVars(const PanoramaData& optPano, int mode, unsigned anchorImg)
{
    OptimizeVector optvars;
    const SrcPanoImage & anchorImage = optPano.getImage(anchorImg);
    for (unsigned i=0; i < optPano.getNrOfImages(); i++) {
        std::set<std::string> imgopt;
        // do not optimize anchor image's stack for position.
        const SrcPanoImage & iImage = optPano.getImage(i);
        if (!iImage.RollisLinkedWith(anchorImage) &&
            !iImage.PitchisLinkedWith(anchorImage) &&
            !iImage.YawisLinkedWith(anchorImage))
        {
            if (mode & OPT_POS) {
                imgopt.insert("r");
                imgopt.insert("p");
                imgopt.insert("y");
            }
        }
        //we need to optimize roll and pitch to level the pano
        //possible after introduction of line finder
        if((i==anchorImg) && (mode & OPT_POS))
        {
            imgopt.insert("r");
            imgopt.insert("p");
        };
        // do not optimise anchor image for exposure.
        if (i!=anchorImg)
        {
            if (mode & OPT_EXP) {
                imgopt.insert("Eev");
            }
            if (mode & OPT_WB) {
                imgopt.insert("Er");
                imgopt.insert("Eb");
            }
        }
        if (mode & OPT_HFOV) {
            imgopt.insert("v");
        }
        if (mode & OPT_B)
            imgopt.insert("b");
        if (mode & OPT_AC) {
            imgopt.insert("a");
            imgopt.insert("c");
        }
        if (mode & OPT_DE) {
            imgopt.insert("d");
            imgopt.insert("e");
        }
        if (mode & OPT_GT) {
            imgopt.insert("g");
            imgopt.insert("t");
        }
        if (mode & OPT_VIG) {
            imgopt.insert("Vb");
            imgopt.insert("Vc");
            imgopt.insert("Vd");
        }
        if (mode & OPT_VIGCENTRE) {
            imgopt.insert("Vx");
            imgopt.insert("Vy");
        }
        if (mode & OPT_RESP) {
            imgopt.insert("Ra");
            imgopt.insert("Rb");
            imgopt.insert("Rc");
            imgopt.insert("Rd");
            imgopt.insert("Re");
        }
        optvars.push_back(imgopt);
    }
    
    return optvars;
}

} //namespace
