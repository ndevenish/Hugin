
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

// Includes ====================================================================
#include "Image.h"
#ifdef SURF_ENABLED
#include "SurfKeyPointDescriptor.h"
#endif
#include "CircularKeyPointDescriptor.h"
#include "KeyPointDetector.h"

// Using =======================================================================
using namespace boost::python;


// Some wrapper classes ========================================================
struct KeyPointInsertorWrap : lfeat::KeyPointInsertor, wrapper<lfeat::KeyPointInsertor>
{
  
    virtual void operator()(const lfeat::KeyPoint &k) 
    {
        printf("found Keypoint at %f, %f\n", k._x, k._y);
        insert(k);
    };
    int insert(const lfeat::KeyPoint &k)
    {
        return this->get_override("insert")();
    }
};

// Module ======================================================================
BOOST_PYTHON_MODULE(pylf)
{

    class_<std::vector<int> >("IntVec")
        .def(vector_indexing_suite<std::vector<int> >())
    ;
    class_<std::vector<double> >("DoubleVec")
        .def(vector_indexing_suite<std::vector<double> >())
    ;

    class_< lfeat::KeyPoint >("KeyPoint", init< >())
        .def_readwrite("_x", &lfeat::KeyPoint::_x)
        .def_readwrite("_y", &lfeat::KeyPoint::_y)
        .def_readwrite("_scale", &lfeat::KeyPoint::_scale)
        .def_readwrite("_score", &lfeat::KeyPoint::_score)
        .def_readwrite("_trace", &lfeat::KeyPoint::_trace)
        .def_readwrite("_ori", &lfeat::KeyPoint::_ori)
        .def_readwrite("_vec", &lfeat::KeyPoint::_vec)
    ;

    class_<KeyPointInsertorWrap, boost::noncopyable>("KeyPointInsertor")
        .def("insert", pure_virtual(&KeyPointInsertorWrap::insert))
    ;

    class_< lfeat::Image >("Image", init<  >())
        .def(init< const lfeat::Image& >())
      // HACK: use char* instead of double * for simple interfacing
      //.def(init< char*, unsigned int, unsigned int >())
        .def("init", &lfeat::Image::init)
        .def("clean", &lfeat::Image::clean)
        .def("getWidth", &lfeat::Image::getWidth)
        .def("getHeight", &lfeat::Image::getHeight)
      /*
        .def("getPixels", &lfeat::Image::getPixels,return_value_policy<reference_existing_object>())
        .def("getIntegralImage", &lfeat::Image::getIntegralImage)
        .def("AllocateImage", &lfeat::Image::AllocateImage)
        .def("DeallocateImage", &lfeat::Image::DeallocateImage)
        .staticmethod("DeallocateImage")
        .staticmethod("AllocateImage")
      */
    ;

    class_< lfeat::KeyPointDetector >("KeyPointDetector", init<  >())
        .def(init< const lfeat::KeyPointDetector& >())
        .def("setMaxScales", &lfeat::KeyPointDetector::setMaxScales)
        .def("setMaxOctaves", &lfeat::KeyPointDetector::setMaxOctaves)
        .def("setScoreThreshold", &lfeat::KeyPointDetector::setScoreThreshold)
        .def("detectKeypoints", &lfeat::KeyPointDetector::detectKeypoints)
    ;

#ifdef SURF_ENABLED
    class_< lfeat::SurfKeyPointDescriptor, boost::noncopyable >("SurfKeyPointDescriptor", init< lfeat::Image&, optional< bool > >())
        .def("makeDescriptor", &lfeat::SurfKeyPointDescriptor::makeDescriptor)
        .def("getDescriptorLength", &lfeat::SurfKeyPointDescriptor::getDescriptorLength)
        .def("assignOrientation", &lfeat::SurfKeyPointDescriptor::assignOrientation)
    ;
#endif
    class_< lfeat::CircularKeyPointDescriptor, boost::noncopyable >("CircularKeyPointDescriptor", init< lfeat::Image&, std::vector<int>, std::vector<double>, std::vector<double> >())
        .def("makeDescriptor", &lfeat::CircularKeyPointDescriptor::makeDescriptor)
        .def("getDescriptorLength", &lfeat::CircularKeyPointDescriptor::getDescriptorLength)
        .def("assignOrientation", &lfeat::CircularKeyPointDescriptor::assignOrientation)
    ;
}

