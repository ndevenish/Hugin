// this section contains the macros for the property accessors in class SrcPanoImage
// this excerpt is separately precompiled to generate explicit
// definitions, because SWIG can't handle the 'lazy metaprogramming'.
// This is only needed for the python interface.
// The precompiled portion is then inserted into a modified version
// of SrcPanoImage.h called hsi_SrcPanoImage.h and replaces the
// macro definitions there. Otherwise the two headers are the same.
// to precompile this header with gcc, run
// gcc -E hsi_SrcPanoImage_accessor_macros.h

    // property accessors
public:
    // get[variable name] functions. Return the value stored in the ImageVariable.
#define image_variable( name, type, default_value ) \
    type get##name() const { return m_##name.getData(); }
#include "image_variables.h"
#undef image_variable

    // get[variable name]IV functions. Return a const reference to the ImageVariable.
#define image_variable( name, type, default_value ) \
    const ImageVariable<type > & get##name##IV() const { return m_##name; }
#include "image_variables.h"
#undef image_variable

    // set[variable name] functions
#define image_variable( name, type, default_value ) \
    void set##name(type data) { m_##name.setData(data); }
#include "image_variables.h"
#undef image_variable

    /* The link[variable name] functions
     * Pass a pointer to another SrcPanoImg and the respective image variable
     * will be shared between the images. Afterwards, changing the variable with
     * set[variable name] on either image also sets the other image.
     */
#define image_variable( name, type, default_value ) \
    void link##name (BaseSrcPanoImage * target) \
    { m_##name.linkWith(&(target->m_##name)); }
#include "image_variables.h"
#undef image_variable

    /* The unlink[variable name] functions
     * Unlinking a variable makes it unique to this image. Then changing it will
     * not affect the other images.
     */
#define image_variable( name, type, default_value ) \
    void unlink##name () \
    { m_##name.removeLinks(); }
#include "image_variables.h"
#undef image_variable

    /* The [variable name]isLinked functions
     * Returns true if the variable has links, or false if it is independant.
     */
#define image_variable( name, type, default_value ) \
    bool name##isLinked () const \
    { return m_##name.isLinked(); }
#include "image_variables.h"
#undef image_variable

    /* The [variable name]isLinkedWith functions
     * Returns true if the variable is linked with the equivalent variable in
     * the specified image, false otherwise.
     */
#define image_variable( name, type, default_value ) \
    bool name##isLinkedWith (const BaseSrcPanoImage & image) const \
    { return m_##name.isLinkedWith(&(image.m_##name)); }
#include "image_variables.h"
#undef image_variable

protected:
 
    // the image variables m_[variable name]
#define image_variable( name, type, default_value ) \
    ImageVariable<type > m_##name;
#include "image_variables.h"
#undef image_variable    
