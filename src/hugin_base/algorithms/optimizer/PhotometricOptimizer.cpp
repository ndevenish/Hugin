// -*- c-basic-offset: 4 -*-
/** @file PhotometricOptimizer.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PhotometricOptimizer.cpp 1998 2007-05-10 06:26:46Z dangelo $
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

#include "PhotometricOptimizer.h"

#include <fstream>
#include <foreign/levmar/lm.h>
#include <photometric/ResponseTransform.h>

#ifdef DEBUG
#define DEBUG_LOG_VIG 1
#endif


namespace HuginBase {
    
using namespace std;


/** expects the abs(error) values */
inline double weightHuber(double x, double sigma)
{
    if (x > sigma) {
        x = sqrt(sigma* (2*x - sigma));
    }
    return x;
}



PhotometricOptimizer::OptimData::OptimData(const PanoramaData & pano, const OptimizeVector & optvars,
                                           const std::vector<vigra_ext::PointPairRGB> & data,
                                           double mEstimatorSigma, bool symmetric,
                                           int maxIter, AppBase::ProgressReporter & progress)
  : m_pano(pano), m_data(data), huberSigma(mEstimatorSigma), symmetricError(symmetric),
    m_maxIter(maxIter), m_progress(progress)
{
    assert(pano.getNrOfImages() == optvars.size());
        
    for (unsigned i=0; i < pano.getNrOfImages(); i++) {
        m_imgs.push_back(pano.getSrcImage(i));
    }

    std::vector<std::set<std::string> > usedVars(pano.getNrOfImages());

    // create variable map with param <-> var assignments
    for (unsigned i=0; i < optvars.size(); i++) 
    {
        const std::set<std::string> vars = optvars[i];
        const SrcPanoImage & img_i = pano.getImage(i);
        for (std::set<std::string>::const_iterator it = vars.begin();
             it != vars.end(); ++it)
        {
            VarMapping var;
            var.type = *it;
            //check if variable is yet included
            if(set_contains(usedVars[i],var.type))
                continue;
            var.imgs.insert(i);
            usedVars[i].insert(var.type);
            //now check all linked images and add image nr
#define CheckLinked(name)\
            if(img_i.name##isLinked())\
            {\
                for(unsigned j=i+1;j<pano.getNrOfImages();j++)\
                    if(img_i.name##isLinkedWith(pano.getImage(j)))\
                    {\
                        var.imgs.insert(j);\
                        usedVars[j].insert(var.type);\
                    };\
            }

            if(var.type=="Eev")
            {
                CheckLinked(ExposureValue)
            };
            if(var.type=="Er")
            {
                CheckLinked(WhiteBalanceRed)
            };
            if(var.type=="Eb")
            {
                CheckLinked(WhiteBalanceBlue)
            };
            if(var.type[0]=='R')
            {
                CheckLinked(EMoRParams)
            };
            if(var.type=="Va" || var.type=="Vb" || var.type=="Vc" || var.type=="Vd")
            {
                CheckLinked(RadialVigCorrCoeff)
            }
            if(var.type=="Vx" || var.type=="Vy")
            {
                CheckLinked(RadialVigCorrCenterShift)
            };
#undef CheckLinked
            m_vars.push_back(var);
        }
    }
}

void PhotometricOptimizer::OptimData::ToX(double * x)
{
    for (size_t i=0; i < m_vars.size(); i++)
    {
        assert(m_vars[i].imgs.size() > 0);
            // get corresponding image number
        unsigned j = *(m_vars[i].imgs.begin());
            // get value
        x[i] = m_imgs[j].getVar(m_vars[i].type);
        // TODO: transform some variables, such as the vignetting center!
    }
}


void PhotometricOptimizer::OptimData::FromX(double * x)
{
    for (size_t i=0; i < m_vars.size(); i++)
    {
        // TODO: transform some variables, such as the vignetting center!
        assert(m_vars[i].imgs.size() > 0);
            // copy value int all images
        for (std::set<unsigned>::const_iterator it = m_vars[i].imgs.begin();
             it != m_vars[i].imgs.end(); ++it)
        {
            m_imgs[*it].setVar(m_vars[i].type, x[i]);
        }
    }
}



void PhotometricOptimizer::photometricError(double *p, double *x, int m, int n, void * data)
{
#ifdef DEBUG_LOG_VIG
    static int iter = 0;
#endif
    typedef Photometric::ResponseTransform<vigra::RGBValue<double> > RespFunc;
    typedef Photometric::InvResponseTransform<vigra::RGBValue<double>, vigra::RGBValue<double> > InvRespFunc;

    int xi = 0 ;

    OptimData * dat = (OptimData *) data;
    dat->FromX(p);
#ifdef DEBUG_LOG_VIG
    ostringstream oss;
    oss << "vig_log_" << iter;
    iter++;
    ofstream log(oss.str().c_str());
    log << "VIGparams = [";
    for (int i = 0; i < m; i++) {
        log << p[i] << " ";
    }
    log << " ]; " << std::endl;
    // TODO: print parameters of images.
    std::ofstream script("vig_test.pto");
    OptimizeVector optvars(dat->m_pano.getNrOfImages());
    UIntSet imgs = dat->m_pano.getActiveImages();
    dat->m_pano.printPanoramaScript(script, optvars, dat->m_pano.getOptions(), imgs, false, "");
#endif

    size_t nImg = dat->m_imgs.size();
    std::vector<RespFunc> resp(nImg);
    std::vector<InvRespFunc> invResp(nImg);
    for (size_t i=0; i < nImg; i++) {
        resp[i] = RespFunc(dat->m_imgs[i]);
        invResp[i] = InvRespFunc(dat->m_imgs[i]);
        // calculate the monotonicity error
        double monErr = 0;
        if (dat->m_imgs[i].getResponseType() == SrcPanoImage::RESPONSE_EMOR) {
            // calculate monotonicity error
            int lutsize = resp[i].m_lutR.size();
            for (int j=0; j < lutsize-1; j++)
            {
                double d = resp[i].m_lutR[j] - resp[i].m_lutR[j+1];
                if (d > 0) {
                    monErr += d*d*lutsize;
                }
            }
        }
        x[xi++] = monErr;
		// enforce a montonous response curves
		resp[i].enforceMonotonicity();
		invResp[i].enforceMonotonicity();
    }

    double sqerror=0;
    // loop over all points to calculate the error
#ifdef DEBUG_LOG_VIG
    log << "VIGval = [ ";
#endif

    for (std::vector<vigra_ext::PointPairRGB>::const_iterator it = dat->m_data.begin();
         it != dat->m_data.end(); ++it)
    {
        vigra::RGBValue<double> l2 = invResp[it->imgNr2](it->i2, it->p2);
        vigra::RGBValue<double> i2ini1 = resp[it->imgNr1](l2, it->p1);
        vigra::RGBValue<double> error = it->i1 - i2ini1;


        // if requested, calcuate the error in image 2 as well.
        //TODO: weighting dependent on the pixel value? check if outside of i2 range?
        vigra::RGBValue<double> l1 = invResp[it->imgNr1](it->i1, it->p1);
        vigra::RGBValue<double> i1ini2 = resp[it->imgNr2](l1, it->p2);
        vigra::RGBValue<double> error2 = it->i2 - i1ini2;

        for (int i=0; i < 3; i++) {
            sqerror += error[i]*error[i];
            sqerror += error2[i]*error2[i];
        }

        // use huber robust estimator
        if (dat->huberSigma > 0) {
            for (int i=0; i < 3; i++) {
                x[xi++] = weightHuber(fabs(error[i]), dat->huberSigma);
                x[xi++] = weightHuber(fabs(error2[i]), dat->huberSigma);
            }
        } else {
            x[xi++] = error[0];
            x[xi++] = error[1];
            x[xi++] = error[2];
            x[xi++] = error2[0];
            x[xi++] = error2[1];
            x[xi++] = error2[2];
        }

#ifdef DEBUG_LOG_VIG
        log << it->i1.green()  << " "<< l1.green()  << " " << i1ini2.green() << "   " 
             << it->i2.green()  << " "<< l2.green()  << " " << i2ini1.green() << ";  " << std::endl;
#endif

    }
#ifdef DEBUG_LOG_VIG
    log << std::endl << "VIGerr = [";
    for (int i = 0; i < n; i++) {
        log << x[i] << std::endl;
    }
    log << " ]; " << std::endl;
#endif

    DEBUG_DEBUG("squared error: " << sqerror);
}

int PhotometricOptimizer::photometricVis(double *p, double *x, int m, int n, int iter, double sqerror, void * data)
{
    OptimData * dat = (OptimData *) data;
    char tmp[200];
    tmp[199] = 0;
    double error = sqrt(sqerror/n)*255;
    snprintf(tmp,199, "Iteration: %d, error: %f", iter, error);
    return dat->m_progress.increaseProgress(0.0, tmp) ? 1 : 0 ;
}

void PhotometricOptimizer::optimizePhotometric(PanoramaData & pano, const OptimizeVector & vars,
                                               const std::vector<vigra_ext::PointPairRGB> & correspondences,
                                               AppBase::ProgressReporter & progress,
                                               double & error)
{

    OptimizeVector photometricVars;
    // keep only the photometric variables
    unsigned int optCount=0;
    for (OptimizeVector::const_iterator it=vars.begin(); it != vars.end(); ++it)
    {
        std::set<std::string> cvars;
        for (std::set<std::string>::const_iterator itv = (*it).begin();
             itv != (*it).end(); ++itv)
        {
            if ((*itv)[0] == 'E' || (*itv)[0] == 'R' || (*itv)[0] == 'V') {
                cvars.insert(*itv);
            }
        }
        photometricVars.push_back(cvars);
        optCount+=cvars.size();
    }
    //if no variables to optimize return
    if(optCount==0)
    {
        return;
    };

    int nMaxIter = 250;
    OptimData data(pano, photometricVars, correspondences, 5/255.0, false, nMaxIter, progress);

    int ret;
    //double opts[LM_OPTS_SZ];
    double info[LM_INFO_SZ];

    // parameters
    int m=data.m_vars.size();
    vigra::ArrayVector<double> p(m, 0.0);

    // vector for errors
    int n=2*3*correspondences.size()+pano.getNrOfImages();
    vigra::ArrayVector<double> x(n, 0.0);

    data.ToX(p.begin());
#ifdef DEBUG
    printf("Parameters before optimisation: ");
    for(int i=0; i<m; ++i)
        printf("%.7g ", p[i]);
    printf("\n");
#endif

    // covariance matrix at solution
    vigra::DImage cov(m,m);
    // TODO: setup optimisation options with some good defaults.
    double optimOpts[5];
    
    optimOpts[0] = 1E-03;  // init mu
    // stop thresholds
    optimOpts[1] = 1e-5;   // ||J^T e||_inf
    optimOpts[2] = 1e-5;   // ||Dp||_2
    optimOpts[3] = 1e-1;   // ||e||_2
    // difference mode
    optimOpts[4] = LM_DIFF_DELTA;
    
    ret=dlevmar_dif(&photometricError, &photometricVis, &(p[0]), &(x[0]), m, n, nMaxIter, optimOpts, info, NULL, &(cov(0,0)), &data);  // no jacobian
    // copy to source images (data.m_imgs)
    data.FromX(p.begin());
    // calculate error at solution
    data.huberSigma = 0;
    photometricError(&(p[0]), &(x[0]), m, n, &data);
    error = 0;
    for (int i=0; i<n; i++) {
        error += x[i]*x[i];
    }
    error = sqrt(error/n);

#ifdef DEBUG
    printf("Levenberg-Marquardt returned %d in %g iter, reason %g\nSolution: ", ret, info[5], info[6]);
    for(int i=0; i<m; ++i)
        printf("%.7g ", p[i]);
    printf("\n\nMinimization info:\n");
    for(int i=0; i<LM_INFO_SZ; ++i)
        printf("%g ", info[i]);
    printf("\n");
#endif

    // copy settings to panorama
    for (unsigned i=0; i<pano.getNrOfImages(); i++) {
        pano.setSrcImage(i, data.m_imgs[i]);
    }
}

void SmartPhotometricOptimizer::smartOptimizePhotometric(PanoramaData & pano, PhotometricOptimizeMode mode,
                                                          const std::vector<vigra_ext::PointPairRGB> & correspondences,
                                                          AppBase::ProgressReporter & progress,
                                                          double & error)
{
    std::vector<SrcPanoImage> ret;
    PanoramaOptions opts = pano.getOptions();
    if (mode == OPT_PHOTOMETRIC_LDR || mode == OPT_PHOTOMETRIC_LDR_WB) {
        // optimize exposure
        int vars = OPT_EXP;
        optimizePhotometric(pano, 
                            createOptVars(pano, vars, opts.colorReferenceImage),
                            correspondences, progress, error);

        // optimize vignetting & exposure
        vars = OPT_EXP | OPT_VIG;
        optimizePhotometric(pano, 
                            createOptVars(pano, vars, opts.colorReferenceImage),
                            correspondences, progress, error);

        if (mode == OPT_PHOTOMETRIC_LDR_WB) {
            // optimize vignetting & exposure & wb & response
            vars = OPT_EXP | OPT_VIG | OPT_RESP | OPT_WB;
        } else {
            vars = OPT_EXP | OPT_VIG | OPT_RESP;
        }
        // optimize vignetting & exposure & wb & response
        optimizePhotometric(pano, 
                            createOptVars(pano, vars, opts.colorReferenceImage),
                            correspondences, progress, error);
    } else if (mode == OPT_PHOTOMETRIC_HDR || mode == OPT_PHOTOMETRIC_HDR_WB) {
        // optimize vignetting
        int vars = OPT_VIG;
        optimizePhotometric(pano, 
                            createOptVars(pano, vars, opts.colorReferenceImage),
                            correspondences, progress, error);

        // optimize vignetting, wb and response
        if (mode == OPT_PHOTOMETRIC_HDR_WB) {
            vars = OPT_VIG | OPT_RESP | OPT_WB;
        } else {
            vars =  OPT_VIG | OPT_RESP;
        }
        optimizePhotometric(pano, 
                            createOptVars(pano, vars, opts.colorReferenceImage),
                            correspondences, progress, error);
    } else {
        assert(0 && "Unknown photometric optimisation mode");
    }
    // adjust 
}


bool PhotometricOptimizer::runAlgorithm()
{
    // is this correct? how much progress requierd?
    AppBase::ProgressReporter* progRep = 
        AppBase::ProgressReporterAdaptor::newProgressReporter(getProgressDisplay(), 0.0);    
    
    optimizePhotometric(o_panorama, 
                        o_vars, o_correspondences,
                        *progRep, o_resultError);
    
    delete progRep;
    
    // optimizePhotometric does not tell us if it's cancelled
    if(hasProgressDisplay())
    {
        if(getProgressDisplay()->wasCancelled())
            cancelAlgorithm();
    }
    
    return wasCancelled(); // let's hope so.
}

bool SmartPhotometricOptimizer::runAlgorithm()
{
    AppBase::ProgressReporter* progRep;
    
    if(hasProgressDisplay()) {
        // is this correct? how much progress requierd?
        progRep = new AppBase::ProgressReporterAdaptor(*getProgressDisplay(), 0.0); 
    } else {
        progRep = new AppBase::DummyProgressReporter();
    }
    
    smartOptimizePhotometric(o_panorama,
                             o_optMode,
                             o_correspondences,
                             *progRep, o_resultError);
    
    delete progRep;
    
    // smartOptimizePhotometric does not tell us if it's cancelled
    if(hasProgressDisplay())
    {
        if(getProgressDisplay()->wasCancelled())
            cancelAlgorithm();
    }
    
    return !wasCancelled(); // let's hope so.
}

} //namespace
