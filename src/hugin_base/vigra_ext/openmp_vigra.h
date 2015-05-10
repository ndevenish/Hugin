/*
 * Copyright (C) 2013, 2014 Christoph L. Spiel
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * <http://www.gnu.org/licenses/>.
 */
#ifndef OPENMP_VIGRA_H_INCLUDED_
#define OPENMP_VIGRA_H_INCLUDED_

#include <vigra/diff2d.hxx>
#include <vigra/initimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/combineimages.hxx>
#include <vigra/convolution.hxx>

#ifdef _OPENMP
#define OPENMP
#endif

namespace vigra
{
    namespace omp
    {
#ifdef OPENMP
        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImages(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                         SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                         DestImageIterator dest_upperleft, DestAccessor dest_acc,
                         const Functor& functor)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src1_lowerright - src1_upperleft);
                Functor f(functor);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::combineTwoImages(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                            src2_upperleft + begin, src2_acc,
                                            dest_upperleft + begin, dest_acc,
                                            f);
                }
            } // omp parallel
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImagesIf(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                           SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                           MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                           DestImageIterator dest_upperleft, DestAccessor dest_acc,
                           const Functor& functor)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src1_lowerright - src1_upperleft);
                Functor f(functor);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::combineTwoImagesIf(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                              src2_upperleft + begin, src2_acc,
                                              mask_upperleft + begin, mask_acc,
                                              dest_upperleft + begin, dest_acc,
                                              f);
                }
            } // omp parallel
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class SrcImageIterator3, class SrcAccessor3,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineThreeImages(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                           SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                           SrcImageIterator3 src3_upperleft, SrcAccessor3 src3_acc,
                           DestImageIterator dest_upperleft, DestAccessor dest_acc,
                           const Functor& functor)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src1_lowerright - src1_upperleft);
                Functor f(functor);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::combineThreeImages(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                              src2_upperleft + begin, src2_acc,
                                              src3_upperleft + begin, src3_acc,
                                              dest_upperleft + begin, dest_acc,
                                              f);
                }
            } // omp parallel
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImage(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                  DestImageIterator dest_upperleft, DestAccessor dest_acc)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src_lowerright - src_upperleft);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::copyImage(src_upperleft + begin, src_upperleft + end, src_acc,
                                     dest_upperleft + begin, dest_acc);
                }
            } // omp parallel
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImageIf(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                    MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                    DestImageIterator dest_upperleft, DestAccessor dest_acc)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src_lowerright - src_upperleft);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::copyImageIf(src_upperleft + begin, src_upperleft + end, src_acc,
                                       mask_upperleft + begin, mask_acc,
                                       dest_upperleft + begin, dest_acc);
                }
            } // omp parallel
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImage(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                       DestImageIterator dest_upperleft, DestAccessor dest_acc,
                       const Functor& functor)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src_lowerright - src_upperleft);
                Functor f(functor);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::transformImage(src_upperleft + begin, src_upperleft + end, src_acc,
                                          dest_upperleft + begin, dest_acc,
                                          f);
                }
            } // omp parallel
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImageIf(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                         MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                         DestImageIterator dest_upperleft, DestAccessor dest_acc,
                         const Functor& functor)
        {
#pragma omp parallel
            {
                const vigra::Size2D size(src_lowerright - src_upperleft);
                Functor f(functor);

#pragma omp for schedule(guided) nowait
                for (int y = 0; y < size.y; ++y)
                {
                    const vigra::Diff2D begin(0, y);
                    const vigra::Diff2D end(size.x, y + 1);

                    vigra::transformImageIf(src_upperleft + begin, src_upperleft + end, src_acc,
                                            mask_upperleft + begin, mask_acc,
                                            dest_upperleft + begin, dest_acc,
                                            f);
                }
            } // omp parallel
        }


#else


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImages(SrcImageIterator1 src1_upperleft,
                         SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                         SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                         DestImageIterator dest_upperleft, DestAccessor dest_acc,
                         const Functor& func)
        {
            vigra::combineTwoImages(src1_upperleft, src1_lowerright, src1_acc,
                                    src2_upperleft, src2_acc,
                                    dest_upperleft, dest_acc,
                                    func);
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImagesIf(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                           SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                           MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                           DestImageIterator dest_upperleft, DestAccessor dest_acc,
                           const Functor& func)
        {
            vigra::combineTwoImagesIf(src1_upperleft, src1_lowerright, src1_acc,
                                      src2_upperleft, src2_acc,
                                      mask_upperleft, mask_acc,
                                      dest_upperleft, dest_acc,
                                      func);
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class SrcImageIterator3, class SrcAccessor3,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineThreeImages(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                           SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                           SrcImageIterator3 src3_upperleft, SrcAccessor3 src3_acc,
                           DestImageIterator dest_upperleft, DestAccessor dest_acc,
                           const Functor& func)
        {
            vigra::combineThreeImages(src1_upperleft, src1_lowerright, src1_acc,
                                      src2_upperleft, src2_acc,
                                      src3_upperleft, src3_acc,
                                      dest_upperleft, dest_acc,
                                      func);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImage(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                  DestImageIterator dest_upperleft, DestAccessor dest_acc)
        {
            vigra::copyImage(src_upperleft, src_lowerright, src_acc, dest_upperleft, dest_acc);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImageIf(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                    MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                    DestImageIterator dest_upperleft, DestAccessor dest_acc)
        {
            vigra::copyImageIf(src_upperleft, src_lowerright, src_acc,
                               mask_upperleft, mask_acc,
                               dest_upperleft, dest_acc);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImage(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                       DestImageIterator dest_upperleft, DestAccessor dest_acc,
                       const Functor& func)
        {
            vigra::transformImage(src_upperleft, src_lowerright, src_acc,
                                  dest_upperleft, dest_acc,
                                  func);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImageIf(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                         MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                         DestImageIterator dest_upperleft, DestAccessor dest_acc,
                         const Functor& func)
        {
            vigra::transformImageIf(src_upperleft, src_lowerright, src_acc,
                                    mask_upperleft, mask_acc,
                                    dest_upperleft, dest_acc,
                                    func);
        }

#endif // OPENMP


        //
        // Argument Object Factory versions
        //

        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImages(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                         vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                         vigra::pair<DestImageIterator, DestAccessor> dest,
                         const Functor& functor)
        {
            vigra::omp::combineTwoImages(src1.first, src1.second, src1.third,
                                         src2.first, src2.second,
                                         dest.first, dest.second,
                                         functor);
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineTwoImagesIf(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                           vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                           vigra::pair<MaskImageIterator, MaskAccessor> mask,
                           vigra::pair<DestImageIterator, DestAccessor> dest,
                           const Functor& functor)
        {
            vigra::omp::combineTwoImagesIf(src1.first, src1.second, src1.third,
                                           src2.first, src2.second,
                                           mask.first, mask.second,
                                           dest.first, dest.second,
                                           functor);
        }


        template <class SrcImageIterator1, class SrcAccessor1,
                  class SrcImageIterator2, class SrcAccessor2,
                  class SrcImageIterator3, class SrcAccessor3,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        combineThreeImages(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                           vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                           vigra::pair<SrcImageIterator3, SrcAccessor3> src3,
                           vigra::pair<DestImageIterator, DestAccessor> dest,
                           const Functor& functor)
        {
            vigra::omp::combineThreeImages(src1.first, src1.second, src1.third,
                                           src2.first, src2.second,
                                           src3.first, src3.second,
                                           dest.first, dest.second,
                                           functor);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                       vigra::pair<DestImageIterator, DestAccessor> dest,
                       const Functor& functor)
        {
            vigra::omp::transformImage(src.first, src.second, src.third,
                                       dest.first, dest.second,
                                       functor);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                  vigra::pair<DestImageIterator, DestAccessor> dest)
        {
            vigra::omp::copyImage(src.first, src.second, src.third,
                                  dest.first, dest.second);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor>
        inline void
        copyImageIf(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::pair<MaskImageIterator, MaskAccessor> mask,
                    vigra::pair<DestImageIterator, DestAccessor> dest)
        {
            vigra::omp::copyImageIf(src.first, src.second, src.third,
                                    mask.first, mask.second,
                                    dest.first, dest.second);
        }


        template <class SrcImageIterator, class SrcAccessor,
                  class MaskImageIterator, class MaskAccessor,
                  class DestImageIterator, class DestAccessor,
                  class Functor>
        inline void
        transformImageIf(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         vigra::pair<MaskImageIterator, MaskAccessor> mask,
                         vigra::pair<DestImageIterator, DestAccessor> dest,
                         const Functor& functor)
        {
            vigra::omp::transformImageIf(src.first, src.second, src.third,
                                         mask.first, mask.second,
                                         dest.first, dest.second,
                                         functor);
        }

    } // namespace omp
} // namespace vigra


#endif // OPENMP_VIGRA_H_INCLUDED_

// Local Variables:
// mode: c++
// End:
