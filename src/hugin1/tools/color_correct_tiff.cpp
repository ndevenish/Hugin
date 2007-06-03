/**
 * @file one.cc
 * @author Philippe Thomin <philippe.thomin@univ-valenciennes.fr>
 * @date 200511
 * @version very first
 *
 * Color correction for panoramic stitching.
 * usage: one referenceimage imagetocorrect correctedimage [-c | -l]
 * options
 * -l do overall correction (same correction on each RGB channel)
 * -c do per-channel correction (different correction on each RGB channel)
 * Images must be tiff, have the same size, have been aligned prior to correction
 * and have some opaque overlapping parts (ie use nona with multiple tiff output).
 */

#include <iostream>
#include <sstream>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <cctype>
#include <cmath>
#include <tiffio.h>

/**
 * Makes a string from an int.
 * @param x value to convert
 * @return a string representing value
 */
std::string tostring(const int x)
{
    std::ostringstream s;
    s << x;
    return s.str();
}

/**
 * color dependant data
 */
struct rgb
{
    double r, g, b;

    /** creates (0,0,0) color data */
    rgb()
            :r(0.), g(0.), b(0.)
    {}
    /** creates (x,x,x) color data */
    explicit rgb(const double _x)
            :r(_x), g(_x), b(_x)
    {}
    /** creates (r,g,b) color data */
    rgb(const double _r, const double _g, const double _b)
            :r(_r), g(_g), b(_b)
    {}
    /**
     * adds color data
     * @param x color data to add
     * @return self reference
     */
    rgb& operator+=(const rgb& x)
    {
        r+=x.r;
        g+=x.g;
        b+=x.b;
        return *this;
    }
    /**
     * substracts color data
     * @param x color data to add
     * @return self reference
     */
    rgb& operator-=(const rgb& x)
    {
        r-=x.r;
        g-=x.g;
        b-=x.b;
        return *this;
    }
    /**
     * scale color data
     * @param x scaling factor
     * @return self reference
     */
    rgb& operator*=(const double x)
    {
        r*=x;
        g*=x;
        b*=x;
        return *this;
    }
    /**
     * computes sum of 2 color data
     * @param x color data to add
     * @return sum
     */
    rgb operator+(const rgb& x) const
    {
        rgb y(*this);
        return y+=x;
    }
    /**
     * computes difference of 2 color data
     * @param x color data to substract
     * @return difference
     */
    rgb operator-(const rgb& x) const
    {
        rgb y(*this);
        return y-=x;
    }
    /**
     * computes scaled color data
     * @param x scale factor
     * @return scaled data
     */
    rgb operator*(const double x) const
    {
        rgb y(*this);
        return y*=x;
    }
    /**
     * computes length of color data
     * @return length
     */
    double length() const
    {
        return sqrt(r*r + g*g + b*b);
    }
    /**
     * output color data
     * @param s output stream
     * @return modified output stream
     */
     std::ostream& print(std::ostream& s) const
     {
         return s << r << " " << g << " " << b;
     }
};

/**
 * output color data
 * @param s output stream
 * @param x color data
 * @return modified output stream
 */
std::ostream& operator<<(std::ostream& s, const rgb& x)
{
    return x.print(s);
}

/**
 * computes absolute value of color data
 * @param x color data
 * @return new color data
 */
rgb abs(const rgb& x)
{
    rgb y(x);
    if (y.r < 0)
        y.r = -y.r;
    if (y.g < 0)
        y.g = -y.g;
    if (y.b < 0)
        y.b = -y.b;
    return y;
}

/**
 * value adjusting function
 */
unsigned char vadjust(unsigned char v, double a, double b)
{
    int j = (int)(a*(255-v) + b*v);
    if (j < 0)
        j = 0;
    else if (j > 255)
        j = 255;
    return j;
}

/**
 * pixel adjusting object
 */
class adjuster
{
    rgb a, b;
public:
    adjuster(const rgb& _a, const rgb& _b) : a(_a), b(_b)
    {std::cerr << "adjuster(" << a << "," << b << ")" << std::endl;}
    void operator()(uint32* pp)
    {
        uint32 alpha = (*pp >> 24) & 255;
        uint32 blue = (*pp >> 16) & 255;
        uint32 green = (*pp >> 8) & 255;
        uint32 red = (*pp >> 0) & 255;
        blue = vadjust(blue, a.b, b.b);
        green = vadjust(green, a.g, b.g);
        red = vadjust(red, a.r, b.r);
        if ( alpha != 0 )
            *pp = alpha<<24 | blue<<16 | green<<8 | red<<0;
    }
};

/**
 * Image in ABGR format
 */
struct image
{
    /**
     * error thrown by image methods
     */
    class error : public std::exception
    {
        std::string _message;
    public:
        explicit error(const std::string& message)
                : _message(message)
        {}
        virtual ~error() throw()
        {}
        virtual const char* what() const throw()
        {
            return _message.c_str();
        }
    };
    /** image width */
    size_t width;
    /** image height */
    size_t height;
    /** image data */
    uint32 *raster;

    /**
     * Reads image from TIFF file
     * @param _name filename
     */
    void read(const std::string _name)
    {
        TIFF *handle = TIFFOpen(_name.c_str(), "r");
        if (handle == 0)
            throw error(_name+": cannot open image for read");
        try
        {
            TIFFGetField(handle, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(handle, TIFFTAG_IMAGELENGTH, &height);
            uint32 npixels = width * height;
            raster = (uint32*)malloc(npixels * sizeof (uint32));
            if (raster == 0)
                error(_name+": cannot allocate memory for "
                      +tostring(width)+"x"+tostring(height)+" pixels");
            if (!TIFFReadRGBAImage(handle, width, height, raster, 0))
                throw(_name+": error while reading image");
        }
        catch(error& e)
        {
            std::cout << "intermediate catch:" << e.what() << std::endl;
            if (handle != 0)
                TIFFClose(handle);
            free(raster);
            raster=0;
            throw(e);
        }
        TIFFClose(handle);
    }
    /**
     * Writes image to TIFF file
     * @param _name file name
     */
    void write(const std::string _name)
    {
        TIFF *handle = TIFFOpen(_name.c_str(), "w");
        if (handle == 0)
            throw error(_name+": cannot open image for write");
        try
        {
            TIFFSetField(handle, TIFFTAG_IMAGEWIDTH, width);
            TIFFSetField(handle, TIFFTAG_IMAGELENGTH, height);
            TIFFSetField(handle, TIFFTAG_BITSPERSAMPLE, 8 );
            TIFFSetField(handle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
            TIFFSetField(handle, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(handle, TIFFTAG_SAMPLESPERPIXEL, 4);
            TIFFSetField(handle, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS );
            TIFFSetField(handle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
            TIFFSetField(handle, TIFFTAG_ROWSPERSTRIP, height );

            for (size_t y = 0; y < height; ++y)
                if (TIFFWriteScanline(handle, raster+width*(height-1-y), y) != 1)
                    throw(_name+": cannot write pixel data");
        }
        catch(error& e)
        {
            if (handle != 0)
                TIFFClose(handle);
            throw(e);
        }
        TIFFClose(handle);
    }
    /**
     * Creates an image from a tiff file
     * @param _name file name
     */
    explicit image(const std::string _name)
            : width(0), height(0), raster(0)
    {
        read(_name);
    }
    /**
     * Creates an empty image
     */
    image()
            : width(0), height(0), raster(0)
    {}
    /**
     * Destroy an image
     */
    virtual ~image()
    {
        free(raster);
    }
    /**
     * Quick test an image
     */
    operator bool() const
    {
        return raster != 0;
    }
    /**
     * change pixels
     * @param f pixel changer object
     */
    template <typename CHGR>
    void change(CHGR f)
    {
        if (raster == 0)
            return;
        uint32 *i = raster, *e = raster + width * height;
        while (i != e)
            f(i++);
    }
private:
    image(const image&); // no clone
    image& operator=(const image&); // no copy
};

/**
 * luminance correction application
 *
 * Reads two images and creates a third one such that histograms
 * of overlapping parts are as close as possible.
 */
class adjust
{
public:
    /**
     * error thrown by adjust methods
     */
class error : public std::exception
    {
        std::string _message;
    public:
        explicit error(const std::string& message)
                : _message(message)
        {}
        virtual ~error() throw()
        {}
        virtual const char* what() const throw()
        {
            return _message.c_str();
        }
    };
private:
    /**
     * type used for image names lists parameters
     */
    typedef std::vector<std::string> namelist_type;
    /**
     * type used for internal image collection
     */
    typedef std::vector<image*> imagelist_type;
    /**
     * image collection
     */
    imagelist_type images;
    /**
     * correction type
     * true: overall
     * false: separate coeficients on each channel
     */
    bool lum;
    /**
     *
     */
    void clear()
    {
        imagelist_type::const_iterator i = images.begin(), e = images.end();
        while (i != e)
            delete(*i++);
    }
    /**
     * read the 2 images to adjust
     */
    void readimages(std::vector<std::string> names)
    {
        size_t refwidth, refheight;
        namelist_type::const_iterator i = names.begin(), e = names.end();
        for (; i != e && images.size() < 2; ++i)
        {
            std::cout << "opening " << *i << "..." << std::endl;
            images.push_back(new image(*i));
            std::cout << images.back()->width << " x "
            << images.back()->height << std::endl;
            if (images.size() == 1)
            {
                refwidth = images.back()->width;
                refheight = images.back()->height;
            }
            else if (images.back()->width != refwidth && images.back()->height != refheight)
            {
                clear();
                throw error("sizes do not match");
            }
        }
    }
    /**
     * creates adjusted image
     */
    void do_job(std::string name)
    {
        // compute histograms for common parts
        size_t npix = images[0]->height * images[0]->width;
        size_t ncount = 0;
        rgb h0[256], h1[256];
        for (int i = 0; i < 256; ++i)
            h0[i]  = h1[i] = rgb(0,0,0);
        for (size_t i = 0; i < npix; ++i)
        {
            uint32& pixel0 = images[0]->raster[i];
            uint32& pixel1 = images[1]->raster[i];
            if ( (pixel0 >> 24) & 255 != 0 && (pixel1 >> 24) & 255 != 0)
            {
                ++ncount;
                h0[(pixel0 >> 16) & 255 ] += rgb(0,0,1);
                h1[(pixel1 >> 16) & 255 ] += rgb(0,0,1);
                h0[(pixel0 >> 8) & 255 ] += rgb(0,1,0);
                h1[(pixel1 >> 8) & 255 ] += rgb(0,1,0);
                h0[(pixel0 >> 0) & 255 ] += rgb(1,0,0);
                h1[(pixel1 >> 0) & 255 ] += rgb(1,0,0);
            }
        }

        if (ncount == 0)
            throw error("no common parts");
        std::cout << "common pixels:" << ncount << "/" << npix << " = "
        << (double)ncount/npix*100 << "%" << std::endl;

        // normalize histograms values
        for (int i = 0; i < 256; ++i)
        {
            h0[i] *= 1./ncount;
            h1[i] *= 1./ncount;
        }

        // search pair (a,b) such that distance(image0, a + image1*b) is minimum
        rgb h2[256];
        typedef std::multimap<double,std::pair<int,int> > score_type;
        score_type score, scorer, scoreg, scoreb;
        int res=100;
        for (int ia = -res; ia <= 2*res; ++ia)
        {
            if (ia%(res/10)==0)
                std::cout << "." << std::flush;
            double a = ia/(double)res;
            for (int ib = -res; ib <= 2*res; ++ib)
            {
                double b = ib/(double)res;
                for (int i = 0; i < 256; ++i)
                    h2[i] = rgb(0,0,0);
                for (int i = 0; i < 256; ++i)
                    h2[vadjust(i, a, b)] += h1[i];
                double d = 0;
                rgb drgb;
                for (int i = 0; i < 256; ++i)
                {
                    rgb dd = abs(h0[i]-h2[i]);
                    d += dd.length();
                    drgb += dd;
                }
                score.insert(std::make_pair(d, std::make_pair(ia, ib)));
                scorer.insert(std::make_pair(drgb.r, std::make_pair(ia, ib)));
                scoreg.insert(std::make_pair(drgb.g, std::make_pair(ia, ib)));
                scoreb.insert(std::make_pair(drgb.b, std::make_pair(ia, ib)));
            }
        }
        std::cout << std::endl << "correction coefficients (out = a*(1-in) + b*in):" << std::endl;
        const size_t show = 5;
        std::cout << "overall" << std::endl;
        score_type::const_iterator i = score.begin(), e = score.end();
        for (size_t n = 0; i != e; ++n, ++i)
        {
            const std::pair<int,int>& p = i->second;
            if (n< show /*|| n>score.size()- show*/ || (p.first == 0 && p.second == res))
                std::cout << i->first
                << ":  a=" << p.first/(double)res
                << ", b=" << p.second/(double)res
                << std::endl;
        }
        std::cout << "red" << std::endl;
        i = scorer.begin();
        e = scorer.end();
        for (size_t n = 0; i != e; ++n, ++i)
        {
            const std::pair<int,int>& p = i->second;
            if (n< show /*|| n>score.size()- show*/ || (p.first == 0 && p.second == res))
                std::cout << n << "  " << i->first
                << ":  a=" << p.first/(double)res
                << ", b=" << p.second/(double)res
                << std::endl;
        }
        std::cout << "green" << std::endl;
        i = scoreg.begin();
        e = scoreg.end();
        for (size_t n = 0; i != e; ++n, ++i)
        {
            const std::pair<int,int>& p = i->second;
            if (n< show /*|| n>score.size()- show*/ || (p.first == 0 && p.second == res))
                std::cout << n << "  " << i->first
                << ":  a=" << p.first/(double)res
                << ", b=" << p.second/(double)res
                << std::endl;
        }
        std::cout << "blue" << std::endl;
        i = scoreb.begin();
        e = scoreb.end();
        for (size_t n = 0; i != e; ++n, ++i)
        {
            const std::pair<int,int>& p = i->second;
            if (n< show /*|| n>score.size()- show*/ || (p.first == 0 && p.second == res))
                std::cout << n << "  " << i->first
                << ":  a=" << p.first/(double)res
                << ", b=" << p.second/(double)res
                << std::endl;
        }

        // create result image
        const std::pair<int,int>& abo = score.begin()->second;
        const double& ao = abo.first  / (double)res;
        const double& bo = abo.second / (double)res;
        const std::pair<int,int>& abr = scorer.begin()->second;
        const double& ar = abr.first  / (double)res;
        const double& br = abr.second / (double)res;
        const std::pair<int,int>& abg = scoreg.begin()->second;
        const double& ag = abg.first  / (double)res;
        const double& bg = abg.second / (double)res;
        const std::pair<int,int>& abb = scoreb.begin()->second;
        const double& ab = abb.first  / (double)res;
        const double& bb = abb.second / (double)res;
        if (lum)
        {
            std::cout << "overall change" << std::endl;
            images[1]->change(adjuster(rgb(ao), rgb(bo)));
        }
        else
        {
            std::cout << "per-channel change" << std::endl;
            images[1]->change(adjuster(rgb(ar,ag,ab), rgb(br,bg,bb)));
        }

        std::cout << "write image" << name << std::endl;
        images[1]->write(name);
    }
public:
    /**
     * main call
     */
    adjust(std::vector<std::string> names, const bool _lum=true) : lum(_lum)
    {
        if (names.size() != 3)
            throw error("bad name list size");
        readimages(names);
        do_job(names.back());
    }
    /**
     * clean data
     */
    virtual ~adjust()
    {
        clear();
    }
};

/**
 * main interface
 *
 * usage: programname inputfile1 inputfile2 outputfile
 *
 * Creates an image adjusted from second file to match histogram of first file
 */
int main(int argc, char **argv)
{
    // get image names
    std::vector<std::string> names;
    bool lum=true;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-c")==0)
            lum=false;
        else if (strcmp(argv[i], "-l")==0)
            lum=true;
        else
            names.push_back(argv[i]);
    }
    if (names.size() < 3)
    {
        std::cerr << "usage: " << argv[0]
        << "[-l(*)|-c] inputfile1 inputfile2 outputfile" << std::endl
        << "(*) default value" << std::endl
        << std::endl
        << "Color correction for panoramic stitching."<< std::endl
        << "options" << std::endl
        << "  -l do overall correction (same correction on each RGB channel)" << std::endl
        << "  -c do per-channel correction (different correction on each RGB channel)" << std::endl
        << std::endl
        << "Images must be tiff, have the same size, have been aligned prior to correction" << std::endl
        << " and have some opaque overlapping parts (ie use nona with multiple tiff output)." << std::endl;
        exit(1);
    }
    // do job
    try
    {
        adjust app(names, lum);
    }
    // catch errors
    catch(adjust::error& e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "internal error" << std::endl;
        exit(1);
    }
    // that's all folks !
    return 0;
}
