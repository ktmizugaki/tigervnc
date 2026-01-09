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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <core/LogWriter.h>

#include "ScalingBuffer.h"

static core::LogWriter vlog("ScalingBuffer");

PixelBufferScaler::PixelBufferScaler() {}
PixelBufferScaler::~PixelBufferScaler() {}

class NNScaler: public PixelBufferScaler {
  void prepare(const rfb::ModifiablePixelBuffer* dst, const rfb::PixelBuffer* src) override
  {
    init_param(param_x, src->width(), dst->width());
    init_param(param_y, src->height(), dst->height());
  }
  core::Rect calculateAffectedRect(const core::Rect &r) override
  {
    const int srcw = param_x.src, dstw = param_x.dst;
    const int srch = param_y.src, dsth = param_y.dst;
    return core::Rect(
      ((r.tl.x - param_x.shift)*dstw + srcw-1)/srcw,
      ((r.tl.y - param_y.shift)*dsth + srch-1)/srch,
      ((r.br.x - param_x.shift)*dstw + srcw-1)/srcw,
      ((r.br.y - param_y.shift)*dsth + srch-1)/srch);
  }

  void scaleRect(const core::Rect &r, rfb::ModifiablePixelBuffer* dstpb, rfb::PixelBuffer* srcpb) override
  {
    const int dstw = param_x.dst, ix = param_x.incr, rx = param_x.rem;
    const int dsth = param_y.dst, iy = param_y.incr, ry = param_y.rem;
    const int errx1 = dstw - ((rx*r.tl.x) % dstw);
    int srcstride, dststride;
    const uint32_t *src_row;
    const uint32_t *src_data;
    uint32_t *dst_row;
    uint32_t *dst_data;
    int x, errx, y, erry;
    // only handle 32 bpp case since PlatformPixelBuffer is fixed to 32 bpp
    assert(dstpb->getPF().bpp == 32);
    src_row = (const uint32_t*)srcpb->getBuffer(sourceRect(r), &srcstride);
    dst_row = (uint32_t*)dstpb->getBufferRW(r, &dststride);
    erry = dsth - ((ry*r.tl.y) % dsth);
    for (y = r.tl.y; y < r.br.y; y++) {
      dst_data = dst_row;
      src_data = src_row;
      errx = errx1;
      for (x = r.tl.x; x < r.br.x; x++) {
        *dst_data++ = *src_data;
        src_data += ix;
        errx -= rx;
        if (errx <= 0) {
          src_data ++;
          errx += dstw;
        }
      }
      dst_row += dststride;
      src_row += iy*srcstride;
      erry -= ry;
      if (erry <= 0) {
        src_row += srcstride;
        erry += dsth;
      }
    }
    dstpb->commitBufferRW(r);
  }

private:
  struct param {
    int src, dst, incr, rem, shift;
  } param_x, param_y;
  void init_param(struct param &p, int src, int dst)
  {
    p.src = src;
    p.dst = dst;
    p.incr = src / dst;
    p.rem = src % dst;
    p.shift = p.incr>0 ? (p.incr-1)/2 : 0;
  }
  core::Rect sourceRect(const core::Rect &r)
  {
    const int srcw = param_x.src, dstw = param_x.dst;
    const int srch = param_y.src, dsth = param_y.dst;
    return core::Rect(
      param_x.shift + r.tl.x*srcw/dstw,
      param_y.shift + r.tl.y*srch/dsth,
      param_x.shift + (r.br.x-1)*srcw/dstw+1,
      param_y.shift + (r.br.y-1)*srch/dsth+1);
  }
};

ScalingBuffer::ScalingBuffer(int width, int height, rfb::ModifiablePixelBuffer* fb) :
  ManagedPixelBuffer(fb->getPF(), width, height),
  framebuffer(fb)
{
  scaler = new NNScaler();
  scaler->prepare(fb, this);
}

ScalingBuffer::~ScalingBuffer()
{
  delete scaler;
  delete framebuffer;
}

void ScalingBuffer::setScaler(PixelBufferScaler *scaler_)
{
  assert(scaler != nullptr);
  delete scaler;
  scaler = scaler_;
  scaler->prepare(framebuffer, this);
  commitBufferRW(getRect());
}

void ScalingBuffer::setFramebuffer(rfb::ModifiablePixelBuffer* fb)
{
  assert(fb != nullptr);
  assert(fb->getPF() == getPF());
  delete framebuffer;
  framebuffer = fb;
  scaler->prepare(fb, this);
  commitBufferRW(getRect());
}

void ScalingBuffer::commitBufferRW(const core::Rect& r)
{
  FullFramePixelBuffer::commitBufferRW(r);
  core::Rect r2 = scaler->calculateAffectedRect(r);
  if (!r2.is_empty()) {
    scaler->scaleRect(r2, framebuffer, this);
  }
}
