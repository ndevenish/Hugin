// -*- c-basic-offset: 4 -*-
/** @file ImageVariable.h
 *
 *  @author James Legg
 * 
 *  @brief Define & Implement the ImageVariable class
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

#ifndef _PANODATA_IMAGEVARIABLE_H
#define _PANODATA_IMAGEVARIABLE_H


namespace HuginBase
{

/** An ImageVariable stores a value that can be linked to other ImageVariables
 * of the same type.
 *
 * When you link ImageVariables, setting one using setData sets all the other
 * linked ImageVariables. Then when reading the ImageVariables data with getData
 * you get the last value set on any of the linked ImageVariables.
 * 
 * @tparam Type The type of the data value to store with this variable. It is
 * the return type for getData and the parameter type for setData.
 * 
 * @todo These will probably be copied when creating the Undo/redo data, but
 * their pointers will remain the same. Therefore when copying a SrcPanoImg, we
 * should offset all the pointers by the difference between the old and new
 * SrcPanoImg, so the links are relatively the same.
 */
template <class Type>
class ImageVariable
{
public:
    /// constructor
    ImageVariable();
    
    /// construct with a given initial data value
    ImageVariable(Type data);
    
    /** Construct linked with something else.
     * 
     * Sets the data from the linked item. Behaves like linkWith.
     * 
     * @see linkWith
     * @param link ImageVariable which to link with. It provides the data for
     * the newly constructed variable.
     */
    ImageVariable(ImageVariable<Type> * link);
    
    /** Copy constructor
     * 
     * Genrally copied for a getSrcImage call, so make the copy independant from
     * the other image variables.
     */
    ImageVariable(const ImageVariable &source);
    
    /// destructor
    ~ImageVariable();
    
    /// get the variable's value
    Type getData() const;
    
    /** Set the variable groups' value.
     * 
     * Calling this sets the value returned by getData for this ImageVariable
     * and any ImageVariables linked to this one.
     * 
     * @param data the data to be set for this group.
     */
    void setData(const Type data);
    
    /** Create a link.
     * 
     * After making a link to another variable, this variable, and any variables
     * we link link to, will now have the passed in variable's data. Calling
     * setData(data) on either variable, or any variable previously linked with
     * either variable, sets the return value for both variable's getData()
     * function, and the getData function of variables previously linked with
     * either variable.
     * (i.e. Links are bidirectional and accumlative)
     * 
     * It takes longer to link when this element is already linked to many
     * other variables. The fastest way to link an array of variables is:
     * @code
     * for (int index = 1; index < size; index++)
     * {
     *     variables_array[index].linkWith(&(variables_array[index - 1]));
     * }
     * @endcode
     * When called like this, each call to linkWith is constant time.
     * 
     * worst case time is O(n+m) where n is the number of linked variables to
     * the object you are calling from, and m is the number of linked variables
     * of the object you pass as a pointer.
     * 
     * @param link a pointer to the variable to link with.
     */
    void linkWith(ImageVariable<Type> * link);
    
    /** remove all links
     * 
     * After calling this, setData will only change the value of this
     * ImageVariable.
     */
    void removeLinks();
    
    /** Find out if there are other linked variables.
     * 
     * @return true if there are any other variables linked with this one,
     * false otherwise.
     */
    bool isLinked() const;
    
    /** Find out if this variable is linked to a given variable
     * 
     * Find if this variable has been linked (either directly or indirectly)
     * with the variable pointed to by otherVariable.
     * 
     * The execution time depends on how the variables were linked. If linked
     * like in the example in linkWith, then it is fastest to use the variable
     * with the largest array index to look for the other one.
     * 
     * In the worst case, the variables are not linked, and the time taken is
     * O(n) where n is the number of linked variables to this object.
     * 
     * @see ImageVariable::linkWith
     * 
     * @param otherVariable the variable to check linkage with.
     * 
     * @return true if this variable is linked with otherVariable,
     * false otherwise.
     */
    bool isLinkedWith(const ImageVariable<Type> * otherVariable) const;
    
protected:
    /** @note
     * To keep the set of images linked, we store two pointers to other items
     * in the set, like a doubly linked list.
     * When copying variable values, we send it down the list in either
     * direction from the variable where setData(data) was called.
     * 
     * @par
     * When unlinking, we point the items linked to to each other, replacing
     * their links to us.
     * 
     * @par
     * When creating links, we need to check for circular lists. We check for
     * the presence of the item we are linking to in either direction before
     * linking to it. Since we are merging two sets that are either disjoint or
     * identical, this is enough.
     * The actual linking is done by linking the end of our list with the
     * beginning of the one containg the one we were passed in linkWith
     * 
     * @par
     * To find the links, we search in both directions.
     * 
     * @par
     * We search the previous links first, then the proceeding ones.
     */
    
    /** Find if we are linked to another ImageVariable earlier in the list.
     * 
     * @param otherVariable the variable to look for
     * @return true if the variable is linked earilier in the list.
     */
    bool searchBackwards(const ImageVariable<Type> * otherVariable) const;
    
    /** Find if we are linked to another ImageVariable later in the list.
     * 
     * @param otherVariable the variable to look for
     * @return true if the variable is linked later in the list.
     */
    bool searchForwards(const ImageVariable<Type> * otherVariable) const;
    
    /** Find the first item in the list of links with this ImageVariable.
     */
    ImageVariable<Type> * findStart();
    
    /** Find the last item in the list of links with this ImageVariable.
     */
    ImageVariable<Type> * findEnd();
    
    /** Set all linked variables earlier in the list to some given data.
     */
    void setBackwards(const Type data);
    
    /** Set all linked variables later in the list to some given data.
     */
    void setForwards(const Type data);
    
    ///////////////////
    // Member variables
    
    /// the data that will be returned when getData() is called.
    Type m_data;
    
    /// The item preceding us in the list of links
    ImageVariable<Type> * m_linkPrevious;
    /// The item following us in the list of links
    ImageVariable<Type> * m_linkNext;
}; // ImageVariable class

/////////////////////////////
// Public member functions //
/////////////////////////////

// Constructors

template <class Type>
ImageVariable<Type>::ImageVariable()
{
    // Start not linked to anything, so the variable is independant.
    m_linkPrevious = 0;
    m_linkNext = 0;
}

template <class Type>
ImageVariable<Type>::ImageVariable(Type data)
{
    // Use the data given
    m_data = data;
    // ...and start not linked to anything.
    m_linkPrevious = 0;
    m_linkNext = 0;
}

template <class Type>
ImageVariable<Type>::ImageVariable(ImageVariable<Type> * link)
{
    // So we don't break linkWith:
    m_linkPrevious = 0;
    m_linkNext = 0;
    // make the link. Note this sets our data from linked variables.
    linkWith(link);
}

template <class Type>
ImageVariable<Type>::ImageVariable(const ImageVariable<Type> & source)
{
    // When creating a copy, make independant.
    m_data = source.m_data;
    m_linkNext = 0;
    m_linkPrevious = 0;
}

// Destructor

template <class Type>
ImageVariable<Type>::~ImageVariable()
{
    // We will need to remove the links to this variable to keep the list sane.
    removeLinks();
    // it is safe to delete this object now.
}

// Other public member functions

template <class Type>
Type ImageVariable<Type>::getData() const
{
    return m_data;
}

template <class Type>
void ImageVariable<Type>::setData(const Type data)
{
    /* We keep multiple copies of the data.
     * Hopefully the data isn't too large, and setData is called far less often
     * then getData, so it should be fast enough this way.
     */    
    // set all the linked variables.
    // these both set this variables data.
    setBackwards(data);
    setForwards(data);
}

template <class Type>
void ImageVariable<Type>::linkWith(ImageVariable<Type> * link)
{
    // We need to first check that we aren't linked already.
    if (searchBackwards(link) || searchForwards(link))
    {
        DEBUG_INFO("Attempt to link already linked variables");
        return;
    }
    else
    {
        // not linked, it is safe to merge them.
        /* we need to merge the two sets together. We link the end of the list
         * with this object in it to the beginning of the list passed to us.
         */
        ImageVariable<Type> *end = findEnd();
        ImageVariable<Type> *beginning = link->findStart();
        end->m_linkNext = beginning;
        beginning->m_linkPrevious = end;
        // now we must set the data from what we were linked to.
        /* the link target has the same data, but the stuff previously linked
         * with us might have something different.
         */
        setBackwards(link->m_data);
    }
}

template <class Type>
void ImageVariable<Type>::removeLinks()
{
    if (m_linkPrevious)
    {
        // there is something before us, link it to the item after, or 0 if none
        m_linkPrevious->m_linkNext = m_linkNext;
    }
    if (m_linkNext)
    {
        // there is something after us, link it to the item before, or 0 if none
        m_linkNext->m_linkPrevious = m_linkPrevious;
        m_linkNext = 0;
    }
    m_linkPrevious = 0;
}

template <class Type>
bool ImageVariable<Type>::isLinked() const
{
    // return true if there are any links, false if this variable is independant
    return (m_linkPrevious || m_linkNext);
}

template <class Type>
bool ImageVariable<Type>::isLinkedWith(const ImageVariable<Type> * otherVariable) const
{
    // return true if we can find a link with the given item.
    return (searchBackwards(otherVariable) || searchForwards(otherVariable));
}

////////////////////////////////
// Protected member functions //
////////////////////////////////

template <class Type>
bool ImageVariable<Type>::searchBackwards(const ImageVariable<Type> * otherVariable) const
{
    // is this what we are looking for?
    if (this == otherVariable) return true;
    // are there any more items to check?
    if (!m_linkPrevious) return false;
    // try another one.
    return m_linkPrevious->searchBackwards(otherVariable);
}

template <class Type>
bool ImageVariable<Type>::searchForwards(const ImageVariable<Type> * otherVariable) const
{
    // is this what we are looking for?
    if (this == otherVariable) return true;
    // are there any more items to check?
    if (!m_linkNext) return false;
    // try another one.
    return m_linkNext->searchForwards(otherVariable);
}

template <class Type>
ImageVariable<Type> * ImageVariable<Type>::findStart()
{
    // Is this the start of the list?
    if (!m_linkPrevious) return this;
    // look father back
    return m_linkPrevious->findStart();
}

template <class Type>
ImageVariable<Type> * ImageVariable<Type>::findEnd()
{
    // Is this the end of the list?
    if (!m_linkNext) return this;
    // look father forwards
    return m_linkNext->findEnd();
}

template <class Type>
void ImageVariable<Type>::setBackwards(const Type data)
{
    // set this and all proceeding items.
    m_data = data;
    if (m_linkPrevious) m_linkPrevious->setBackwards(data);
}

template <class Type>
void ImageVariable<Type>::setForwards(const Type data)
{
    // set this and all proceeding items.
    m_data = data;
    if (m_linkNext) m_linkNext->setForwards(data);
}

} // HuginBase namespace

#endif // ndef _PANODATA_IMAGEVARIABLE_H
