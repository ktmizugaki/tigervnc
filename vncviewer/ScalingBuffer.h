/* Copyright (C) 2025 Kawashima Teruaki
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __SCALINGBUFFER_H__
#define __SCALINGBUFFER_H__

#include <rfb/PixelBuffer.h>

class PixelBufferScaler {
public:
  PixelBufferScaler();
  virtual ~PixelBufferScaler();

  virtual void prepare(const rfb::ModifiablePixelBuffer* dst, const rfb::PixelBuffer* src) = 0;
  virtual core::Rect calculateAffectedRect(const core::Rect &r) = 0;
  virtual void scaleRect(const core::Rect &r, rfb::ModifiablePixelBuffer* dstpb, rfb::PixelBuffer* srcpb) = 0;
};

class ScalingBuffer: public rfb::ManagedPixelBuffer {
public:
  ScalingBuffer(int width, int height, rfb::ModifiablePixelBuffer* fb);
  ~ScalingBuffer();

  void setScaler(PixelBufferScaler *scaler);
  void setFramebuffer(rfb::ModifiablePixelBuffer* fb);
  void commitBufferRW(const core::Rect& r) override;

protected:
  rfb::ModifiablePixelBuffer* framebuffer;
  PixelBufferScaler *scaler;
};

#endif
