#ifndef _VIG_QUOTIENT_ESTIMATOR_H_
#define _VIG_QUOTIENT_ESTIMATOR_H_

#include <common/math.h>

#include "RansacParameterEstimator.h"

#include <iostream>

#include <boost/random.hpp>

#include <vigra_ext/ROIImage.h>

#include "ransac.h"

// uncomment this to use our version of the linear solver
#define HUGIN_VIG_USE_UBLAS

#ifdef HUGIN_VIG_USE_UBLAS

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>

#else

#include "common/lu.h"

#endif

namespace vigra_ext
{

struct PointPair
{
    PointPair()
    {
    }

    PointPair(double i1, const FDiff2D & p1, double r1,
              double i2, const FDiff2D & p2, double r2)
    : i1(i1), p1(p1), r1(r1), i2(i2), p2(p2), r2(r2)
    {
    }

    double i1;
    FDiff2D p1;
    double r1;
    double i2;
    FDiff2D p2;
    double r2;
};

/// function to calculate the vignetting correction: 1 + p[0]*r^2 + p[1]*r^4 + p[2]*r^6
template <class PITER>
double calcVigCorrPoly(PITER p, double r)
{
    const unsigned n=3;
    double rsq = r*r;
    double res = p[n-1] * rsq; 
    for (int j=n-2; j >=0; j--) 
    {
       res = (res + p[j]) * rsq;
    }
    // add constant offset
    return res + 1;
}


// structure with various information about the fitting process
struct VigQuotientEstimateResult
{
    int nUsedPoints;
    double brightnessRMSE;
};

/**
 * This class estimates the parameters of the vignetting curve.
 *
 * We assume the following function:
 *  I1/I2 = (1 + p_1*r_1^2 + p_2*r_1^4 + p_3*r_1^6) / (1 + a*r_2^2 + b*r_2^4 + c*r_2^6)
 *
 * Author: Pablo d'Angelo
 */


class VigQuotientEstimator : public RansacParameterEstimator
{

protected:
        double m_delta;

public:

#ifdef HUGIN_VIG_USE_UBLAS
    typedef boost::numeric::ublas::vector<double> Param;
#else
    typedef std::vector<double> Param;
//    typedef double[3] Param;
#endif

    VigQuotientEstimator (double delta)
    : RansacParameterEstimator(3), m_delta(delta)
    {
    }

    /**  Compute the vignetting curve defined by 3 points.
     *
     *  This results in the following equations:
     *   z = I_1 / I_2
     * 
     *   A*p = 1-z;
     *    with A = [ r_1^2-r_2^2  r_1^4-r_2^4   r_1^6-r_2^6 ]
     *                   ..           ..            ..
     * 
     * @param data A vector containing three 2D points.
     * @param parameters This vector is filled with the computed parameters, if fitting
     *                   was succesfull
     * @return fitting was succesfull
     */
    virtual bool estimate(const std::vector<const PointPair *> &data,
                          Param &p) const
    {
        const size_t n=3;
        if(data.size() < n) {
            return false;
        }
        // estimate point


#ifdef HUGIN_VIG_USE_UBLAS
        p.resize(n);
        // create linear equation 
        boost::numeric::ublas::permutation_matrix<size_t> permut(n);
        boost::numeric::ublas::matrix<double> A(n,n);

        for (unsigned i = 0; i < n; i++) {
            double r1sq = data[i]->r1 * data[i]->r1;
            double r2sq = data[i]->r2 * data[i]->r2;
            double r1f = r1sq;
            double r2f = r2sq;
            for (unsigned j=0;j < n; j++) {
                A(i,j) = r2f - r1f;
                r1f*=r1sq;
                r2f*=r2sq;
            }
            double c = (data[i]->i1 / data[i]->i2);
            p[i] =  1 - c;
         }

         try {
//             std::cout << "before factorize A:" << A << std::endl << "p: " << p << std::endl;
             lu_factorize(A,permut);
//             std::cout << "after factorize A:" << A << std::endl << "perm: " << permut << std::endl;
             lu_substitute(A,permut,p); 
//             std::cout << "after substitute A:" << A << std::endl << "p: " << p << std::endl;
         } catch (boost::numeric::ublas::singular){
             return false;
         }
         return true;
#else
         p.resize(n);
         double matrix[n*(n+1)];
         for (unsigned i = 0; i < n; i++) {
             double r1sq = data[i]->r1 * data[i]->r1;
             double r2sq = data[i]->r2 * data[i]->r2;
             double r1f = r1sq;
             double r2f = r2sq;
             for (unsigned j=0;j < n; j++) {
                 matrix[i + j*n] = r2f - r1f;
                 r1f*=r1sq;
                 r2f*=r2sq;
             }
             double c = (data[i]->i1 / data[i]->i2);
             matrix[i + n*n] =  1 - c;
         }

         double sol[3];
         bool ok =math_lu_solve(matrix, sol, n) != 0;
         p[0] = sol[0];
         p[1] = sol[1];
         p[2] = sol[2];
         return ok;
#endif
    }


    /**
     * Compute a least squares estimate.
     */
    bool leastSquaresEstimate(const std::vector<const PointPair *> &data,
                              Param & p) const
    {
        /*
        {
            std::ofstream of("hugin_vigquot_least_sq_points");
            for (std::vector<const PointPair *>::const_iterator it = data.begin(); it != data.end(); ++it) {
                of << (*it)->r1 << " " << (*it)->i1 << " " << (*it)->r2 << " " << (*it)->i2 << std::endl;
            }
        }
        */

        const size_t n=3;
        if(data.size() < n) {
            return false;
        }
        
#ifdef HUGIN_VIG_USE_UBLAS
        // calculate least squares, based on pseudoinverse.
        // A'A x = A' b.
        boost::numeric::ublas::permutation_matrix<size_t> permut(n);
        boost::numeric::ublas::matrix<double> AtA(n,n);
        for (unsigned i=0; i< n; i++){
            for (unsigned j=0; j< n; j++){
                AtA(i,j) = 0;
            }
        }
        // is this the right way to clear a boost matrix?
//        AtA = boost::numeric::ublas::zero_matrix<double>(n,n);
        p.resize(n);
        // is this the right way to clear a boost vector?
        for (unsigned i=0; i< n; i++){
            p[i] = 0;
        }

        for(unsigned k=0; k < data.size(); k++) {
            // calculate one row of A
            double Arow[n];
            double r1sq = data[k]->r1 * data[k]->r1;
            double r2sq = data[k]->r2 * data[k]->r2;
            double r1f = r1sq;
            double r2f = r2sq;
            for (unsigned j=0;j < n; j++) {
                Arow[j] = r2f - r1f;
                r1f*=r1sq;
                r2f*=r2sq;
            }
            double c = (data[k]->i1 / data[k]->i2);
            double bRow =  1 - c;

            // add to pseudoinverse
            for( unsigned i=0; i<n; ++i)
            {
                // calculate  Atb
                p[i]+=Arow[i]*bRow;
                for( unsigned j=0; j<n; ++j)
                {
                    AtA(i,j)+=Arow[i]*Arow[j];
                }
            }
        }

        // solve system
        try {
            lu_factorize(AtA,permut);
            lu_substitute(AtA,permut,p); 
//            std::cout << "lms solution: AtA (after solve):" << AtA << std::endl << "p: " << p << std::endl;
        } catch (boost::numeric::ublas::singular){
             return false;
        }
        return true;
#else
        utils::LMS_Solver solver(n);
        for(unsigned k=0; k < data.size(); k++) {
            // calculate one row of A
            double Arow[n];
            double r1sq = data[k]->r1 * data[k]->r1;
            double r2sq = data[k]->r2 * data[k]->r2;
            double r1f = r1sq;
            double r2f = r2sq;
            for (unsigned j=0;j < n; j++) {
                Arow[j] = r2f - r1f;
                r1f*=r1sq;
                r2f*=r2sq;
            }
            double c = (data[k]->i1 / data[k]->i2);
            double bRow =  1 - c;
            solver.addRow(&Arow[0], bRow);
        }
        return solver.solve(p);
#endif
    }

    /**
     * Return true if the distance between the line defined by the parameters and the
     * given point is smaller than 'delta' (see constructor).
     * @param parameters The line parameters [n_x,n_y,a_x,a_y].
     * @param data Check that the distance between this point and the line is smaller than 'delta'.
     */
    bool agree(Param &p, const PointPair &data) const
    {
        const int n = 3;

        double r1sq = data.r1 * data.r1;
        double r2sq = data.r2 * data.r2;
        double poly1 = p[n-1] * r1sq; 
        double poly2 = p[n-1] * r2sq; 
        for (int j=n-2; j >=0; j--) 
        {
            poly1 = (poly1 + p[j]) * r1sq;
            poly2 = (poly2 + p[j]) * r2sq;
        }
        poly1 += 1;
        poly2 += 1;
        double e;
        if (data.r1 < data.r2) {
            // I1 should be bigger than I2
            e = data.i2 / data.i1 - poly2/poly1;
        } else {
            e = data.i1 / data.i2 - poly1/poly2;
        }
        return fabs(e) < m_delta;
    }
};



template<class ImageType, class CoordType>
void extractRandomPoints(std::vector<vigra_ext::ROIImage<ImageType, vigra::BImage> *>  &remapped,
                         std::vector<CoordType> & imgXCoord,
                         std::vector<CoordType> & imgYCoord,
                         const std::vector<vigra::Size2D> & imgSize,
                         const std::vector<FDiff2D> & imgCenter,
                         unsigned nPointsPerOverlap,
                         std::vector<PointPair> & points,
                         unsigned & nBadPoints)
{
    assert(remapped.size() == imgXCoord.size());
    assert(remapped.size() == imgYCoord.size());

    // init random number generator
    boost::mt19937 rng;
    unsigned nImg = remapped.size();
    nBadPoints = 0;
    // extract points from all overlaps.
    for (unsigned i=0; i < nImg; i++) {
        for (unsigned j=i+1; j < nImg; j++) {
            // get unifiying region.
            vigra::Rect2D roi1 = remapped[i]->boundingBox();
            vigra::Rect2D roi2 = remapped[j]->boundingBox();
            vigra::Rect2D roi = roi1 & roi2;
            if (roi.isEmpty()) {
                continue;
            }

            double maxr1 = sqrt(((double)imgSize[i].x)*imgSize[i].x + ((double)imgSize[i].y)*imgSize[i].y) / 2.0;
            double maxr2 = sqrt(((double)imgSize[i].x)*imgSize[i].x + ((double)imgSize[i].y)*imgSize[i].y) / 2.0;
            // randomly sample points.
            boost::uniform_int<> distribx(roi.left(), roi.right()-1);
            boost::uniform_int<> distriby(roi.top(), roi.bottom()-1);

            boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
                    randX(rng, distribx);             // glues randomness with mapping
            boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
                    randY(rng, distriby);             // glues randomness with mapping

            for (unsigned maxTry = nPointsPerOverlap*2; nPointsPerOverlap > 0 && maxTry > 0; maxTry--) {
                unsigned x = randX();
                unsigned y = randY();
                if (remapped[i]->getMask(x,y) && remapped[j]->getMask(x,y)) {
                    // extract gray value pair..
                    FDiff2D p1(imgXCoord[i](x-roi1.left(), y-roi1.top()),
                               imgYCoord[i](x-roi1.left(), y-roi1.top()));
                    FDiff2D p1c = p1 - imgCenter[i];
                    p1 = p1/maxr1;
                    p1c = p1c/maxr1;
                    FDiff2D p2(imgXCoord[j](x-roi2.left(), y-roi2.top()),
                               imgYCoord[j](x-roi2.left(), y-roi2.top()));
                    FDiff2D p2c = p2 - imgCenter[j];
                    p2 = p2/maxr2;
                    p2c = p2c/maxr2;
                    double r1 = sqrt(p1c.x*p1c.x + p1c.y*p1c.y);
                    double r2 = sqrt(p2c.x*p2c.x + p2c.y*p2c.y);
                    double i1 = remapped[i]->operator()(x,y);
                    double i2 = remapped[j]->operator()(x,y);
                    if (i1 >= i2 && r1 <= r2) {
                        // ok, point is good. i1 is closer to centre, swap point
                        // so that i1 < i2
                        points.push_back(PointPair(i2, p2, r2, i1, p1, r1) );
                    } else if(i1 < i2 && r1 > r2) {
                        points.push_back(PointPair(i1, p1, r1, i2, p2, r2) );
                    } else {
                        nBadPoints++;
                    }

                    nPointsPerOverlap--;
                }
            }
        }
    }
    DEBUG_DEBUG("random point extraction: consistent: " << points.size()
                << " inconsistent: " << nBadPoints << " points");
}

VigQuotientEstimateResult
optimizeVignettingQuotient(const std::vector<PointPair> & points,
                           double ransacDelta,
                           std::vector<double> & vigCoeff)
{
    VigQuotientEstimateResult res;

    // do optimisation..
    // the quotient error should be < 0.1
    VigQuotientEstimator vqEst(ransacDelta);

#ifdef HUGIN_VIG_USE_UBLAS
    boost::numeric::ublas::vector<double> vigCoeff2(3);
    for (int i=0; i < 3; i++) vigCoeff2[i] = vigCoeff[i];

    res.nUsedPoints = Ransac::compute(vigCoeff2, vqEst, points, 0.99, 0.3);

    for (int i=0; i < 3; i++) vigCoeff[i] = vigCoeff2[i];
#else
    res.nUsedPoints = Ransac::compute(vigCoeff, vqEst, points, 0.99, 0.3);
#endif

    double sqerror=0;
    // calculate brightness RMSE
    // not a real residual, since the brightness error was not minimized. instead
    // the quotient error was minimized. However, a brightness difference is observeable
    // in the final panorama and can be intuitively understood
    // (well, actually its the RMSE, but thats a detail ;)
    for (std::vector<PointPair>::const_iterator pnt=points.begin(); pnt != points.end();
         pnt++)
    {
#ifdef HUGIN_VIG_USE_UBLAS
        sqerror += utils::sqr(calcVigCorrPoly(vigCoeff2, pnt->r1)*pnt->i1 - calcVigCorrPoly(vigCoeff, pnt->r2)*pnt->i2);
#else
        sqerror += utils::sqr(calcVigCorrPoly(vigCoeff, pnt->r1)*pnt->i1 - calcVigCorrPoly(vigCoeff, pnt->r2)*pnt->i2);
#endif
    }
    res.brightnessRMSE = sqrt(sqerror/points.size());
    return res;
}


}

#endif //_VIG_QUOTIENT_PARAM_ESTIMATOR_H_
