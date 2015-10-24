
/**
 * Simple denoising algorithm
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vigra/stdimage.hxx>
#include <vigra/pixelneighborhood.hxx>

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void simpleDenoise(SrcIterator sy, SrcIterator send, SrcAccessor sa,
                   DestIterator dy, DestAccessor da) {
    int width = send.x - sy.x;
    int height = send.y - sy.y;
    
    sy.y++, dy.y++;
    
    for (int y=1; y<height-1; ++y, ++dy.y, ++sy.y) {
        SrcIterator sx = sy;
        DestIterator dx = dy;
        sx.x++, dx.x++;
        
        for (int x=1; x<width-1; ++x, ++dx.x, ++sx.x) {
            vigra::NeighborhoodCirculator<SrcIterator, vigra::EightNeighborCode>
                           circulator(sx),
                           end(circulator);
            int whitePixels = 0;
            int blackPixels = 0;
            do
            {
                if (*circulator > 127)
                    whitePixels++;
                else
                    blackPixels++;
            }
            while(++circulator != end);
            
            if (whitePixels > 6) {
                *dx = 255;
            } else if (blackPixels > 6) {
                *dx = 0;
            } else {
                *dx = *sx;
            }
        }
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void simpleDenoise(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> src,
                    std::pair<DestIterator, DestAccessor> dest) {
    simpleDenoise(src.first, src.second, src.third, dest.first, dest.second);
}
