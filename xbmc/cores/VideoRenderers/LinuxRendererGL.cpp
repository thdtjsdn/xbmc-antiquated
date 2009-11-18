/*
* XBMC Media Center
* Linux OpenGL Renderer
* Copyright (c) 2007 Frodo/jcmarshall/vulkanr/d4rk
*
* Based on XBoxRenderer by Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#ifdef HAS_GL
#include <locale.h>
#include "LinuxRendererGL.h"
#include "Application.h"
#include "MathUtils.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "FrameBufferObject.h"
#include "VideoShaders/YUV2RGBShader.h"
#include "VideoShaders/VideoFilterShader.h"
#include "WindowingFactory.h"
#include "Texture.h"

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

#ifdef HAVE_LIBCRYSTALHD
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#endif

#ifdef HAS_GLX
#include <GL/glx.h>
#endif

using namespace Shaders;

static const GLubyte stipple_weave[] = {
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF,
};

CLinuxRendererGL::CLinuxRendererGL()
{
  m_textureTarget = GL_TEXTURE_2D;
  for (int i = 0; i < NUM_BUFFERS; i++)
    m_eventTexturesDone[i] = CreateEvent(NULL,FALSE,TRUE,NULL);

  m_renderMethod = RENDER_GLSL;
  m_renderQuality = RQ_SINGLEPASS;
  m_iFlags = 0;

  m_iYV12RenderBuffer = 0;
  m_flipindex = 0;
  m_currentField = FIELD_FULL;
  m_reloadShaders = 0;
  m_pYUVShader = NULL;
  m_pVideoFilterShader = NULL;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;

  m_upscalingWidth = 0;
  m_upscalingHeight = 0;
  memset(&m_imScaled, 0, sizeof(m_imScaled));
  m_isSoftwareUpscaling = false;

  memset(m_buffers, 0, sizeof(m_buffers));

  // NULL texture handlers, they get setup later
  LoadTexturesFuncPtr  = NULL;
  CreateTextureFuncPtr = NULL;
  DeleteTextureFuncPtr = NULL;

  m_rgbBuffer = NULL;
  m_rgbBufferSize = 0;

#ifdef HAVE_LIBVDPAU
  m_StrictBinding = g_guiSettings.GetBool("videoplayer.strictbinding");
#endif
}

CLinuxRendererGL::~CLinuxRendererGL()
{
  UnInit();
  for (int i = 0; i < NUM_BUFFERS; i++)
    CloseHandle(m_eventTexturesDone[i]);

  if (m_rgbBuffer != NULL) {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }
  for (int i=0; i<3; i++)
  {
    if (m_imScaled.plane[i])
    {
      delete [] m_imScaled.plane[i];
      m_imScaled.plane[i] = 0;
    }
  }

  if (m_pYUVShader)
  {
    m_pYUVShader->Free();
    delete m_pYUVShader;
    m_pYUVShader = NULL;
  }
}

void CLinuxRendererGL::ManageTextures()
{
  m_NumYV12Buffers = 2;
  //m_iYV12RenderBuffer = 0;
  return;
}

bool CLinuxRendererGL::ValidateRenderTarget()
{
  if (!m_bValidated)
  {
    if (!glewIsSupported("GL_ARB_texture_non_power_of_two") && glewIsSupported("GL_ARB_texture_rectangle"))
    {
      CLog::Log(LOGNOTICE,"Using GL_TEXTURE_RECTANGLE_ARB");
      m_textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    }
    else
      CLog::Log(LOGNOTICE,"Using GL_TEXTURE_2D");

     // create the yuv textures    
    LoadShaders();
    if (CreateTextureFuncPtr)
    {
      for (int i = 0 ; i < m_NumYV12Buffers ; i++)
      {
        (this->*CreateTextureFuncPtr)(i);
      }
    }
    m_bValidated = true;
    return true;
  }
  return false;  
}

bool CLinuxRendererGL::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  m_sourceWidth = width;
  m_sourceHeight = height;

  // Save the flags.
  m_iFlags = flags;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  ChooseUpscalingMethod();

  m_bConfigured = true;
  m_bImageReady = false;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;

  // Ensure that textures are recreated and rendering starts only after the 1st 
  // frame is loaded after every call to Configure().
  m_bValidated = false;

  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
    m_buffers[i].image.flags = 0;

  m_iLastRenderBuffer = -1;
  return true;
}

void CLinuxRendererGL::ChooseUpscalingMethod()
{
  m_upscalingWidth  = m_destRect.Width();
  m_upscalingHeight = m_destRect.Height();

  int upscale = g_guiSettings.GetInt("videoplayer.highqualityupscaling");
  
  // See if we're a candiate for upscaling.
  bool candidateForUpscaling = false;
  if (upscale != SOFTWARE_UPSCALING_DISABLED && (int)m_sourceWidth < m_upscalingWidth && (int)m_sourceHeight < m_upscalingHeight)
  {
    CLog::Log(LOGWARNING, "Upscale: possible given resolution increase.");
    candidateForUpscaling = true;
  }

  // Turn if off if we're told to upscale HD content and we're not always on.
  if (upscale == SOFTWARE_UPSCALING_SD_CONTENT && (m_sourceHeight >= 720 || m_sourceWidth >= 1280))
  {
    CLog::Log(LOGWARNING, "Upscale: Disabled due to HD source.");
    candidateForUpscaling = false;
  }

  if (candidateForUpscaling)
  {
    ESCALINGMETHOD ret = (ESCALINGMETHOD)g_guiSettings.GetInt("videoplayer.upscalingalgorithm");

    // Make sure to override the default setting for the video
    g_stSettings.m_currentVideoSettings.m_ScalingMethod = ret;

    // Initialize software upscaling.
    if (g_guiSettings.GetInt("videoplayer.upscalingalgorithm") < 10) //non-hardware
    {
      InitializeSoftwareUpscaling();
      CLog::Log(LOGWARNING, "Upscale: selected algorithm %d", ret);
    }
  }
}

void CLinuxRendererGL::InitializeSoftwareUpscaling()
{
  // Allocate a new destination image.
  m_imScaled.cshift_x = m_imScaled.cshift_y = 1;

  // Free the old planes if they exist.
  for (int i=0; i<3; i++)
  {
    if (m_imScaled.plane[i])
    {
      delete [] m_imScaled.plane[i];
      m_imScaled.plane[i] = 0;
    }
  }

  m_imScaled.stride[0] = ALIGN((m_upscalingWidth)   , 16);
  m_imScaled.stride[1] = ALIGN((m_upscalingWidth>>1), 16);
  m_imScaled.stride[2] = ALIGN((m_upscalingWidth>>1), 16);
  m_imScaled.plane[0] = new BYTE[m_imScaled.stride[0] * ALIGN((m_upscalingHeight)   , 16)];
  m_imScaled.plane[1] = new BYTE[m_imScaled.stride[1] * ALIGN((m_upscalingHeight>>1), 16)];
  m_imScaled.plane[2] = new BYTE[m_imScaled.stride[2] * ALIGN((m_upscalingHeight>>1), 16)];
  m_imScaled.width = m_upscalingWidth;
  m_imScaled.height = m_upscalingHeight;
  m_imScaled.flags = 0;
}

bool CLinuxRendererGL::IsSoftwareUpscaling()
{
  // See if we should be performing software upscaling on this frame.
  if (m_scalingMethod < VS_SCALINGMETHOD_BICUBIC_SOFTWARE ||
       (m_currentField != FIELD_FULL && 
        g_stSettings.m_currentVideoSettings.m_InterlaceMethod!=VS_INTERLACEMETHOD_NONE && 
        g_stSettings.m_currentVideoSettings.m_InterlaceMethod!=VS_INTERLACEMETHOD_DEINTERLACE))
  {
    return false;
  }

  return true;
}

int CLinuxRendererGL::NextYV12Texture()
{
  return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
}

int CLinuxRendererGL::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;
  if (!m_bValidated) return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  YV12Image &im = m_buffers[source].image;
  
  if (!(m_renderMethod & RENDER_VDPAU) && !(m_renderMethod & RENDER_CRYSTALHD))
  {
    if (!im.plane[0])
    {
       CLog::Log(LOGDEBUG, "CLinuxRendererGL::GetImage - image planes not allocated");
       return -1;
    }
  }
  if ((im.flags&(~IMAGE_FLAG_READY)) != 0)
  {
     CLog::Log(LOGDEBUG, "CLinuxRenderer::GetImage - request image but none to give");
     return -1;
  }

  if( readonly )
    im.flags |= IMAGE_FLAG_READING;
  else
  {
    if( WaitForSingleObject(m_eventTexturesDone[source], 500) == WAIT_TIMEOUT )
      CLog::Log(LOGWARNING, "%s - Timeout waiting for texture %d", __FUNCTION__, source);

    im.flags |= IMAGE_FLAG_WRITING;
  }

  // copy the image - should be operator of YV12Image
  for (int p=0;p<MAX_PLANES;p++)
  {
    image->plane[p]  = im.plane[p];
    image->stride[p] = im.stride[p];
  }
  image->width    = im.width;
  image->height   = im.height;
  image->flags    = im.flags;
  image->cshift_x = im.cshift_x;
  image->cshift_y = im.cshift_y;

  return source;

  return -1;
}

void CLinuxRendererGL::ReleaseImage(int source, bool preserve)
{
  YV12Image &im = m_buffers[source].image;

  if( im.flags & IMAGE_FLAG_WRITING )
    SetEvent(m_eventTexturesDone[source]);

  im.flags &= ~IMAGE_FLAG_INUSE;
  im.flags |= IMAGE_FLAG_READY;
  /* if image should be preserved reserve it so it's not auto seleceted */

  if( preserve )
    im.flags |= IMAGE_FLAG_RESERVED;

  m_bImageReady = true;
}

void CLinuxRendererGL::LoadPlane( YUVPLANE& plane, int type, unsigned flipindex
                                , unsigned width, unsigned height
                                , int stride, void* data )
{
  if(plane.flipindex == flipindex)
    return;

  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
  glBindTexture(m_textureTarget, plane.id);
  glTexSubImage2D(m_textureTarget, 0, 0, 0, width, height, type, GL_UNSIGNED_BYTE, data);

  /* check if we need to load any border pixels */
  if(height < plane.texheight)
    glTexSubImage2D( m_textureTarget, 0
                   , 0, height, width, 1
                   , type, GL_UNSIGNED_BYTE
                   , (unsigned char*)data + stride * (height-1));

  if(width  < plane.texwidth)
    glTexSubImage2D( m_textureTarget, 0
                   , width, 0, 1, height
                   , type, GL_UNSIGNED_BYTE
                   , (unsigned char*)data + stride - 1);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glBindTexture(m_textureTarget, 0);

  plane.flipindex = flipindex;
}

bool CLinuxRendererGL::LoadYV12Textures(int source)
{
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags&IMAGE_FLAG_READY))
  {
    SetEvent(m_eventTexturesDone[source]);
    return(true);
  }

  // See if we need to recreate textures.
  if (m_isSoftwareUpscaling != IsSoftwareUpscaling())
  {
    if (CreateTextureFuncPtr)
    {
      for (int i = 0 ; i < m_NumYV12Buffers ; i++)
        (this->*CreateTextureFuncPtr)(i);
    }
    im->flags = IMAGE_FLAG_READY;
  }

  // if we don't have a shader, fallback to SW YUV2RGB for now
  if (m_renderMethod & RENDER_SW)
  {
    if(m_rgbBufferSize < m_sourceWidth * m_sourceHeight * 4)
    {
      delete [] m_rgbBuffer;
      m_rgbBufferSize = m_sourceWidth*m_sourceHeight*4;
      m_rgbBuffer = new BYTE[m_rgbBufferSize];
    }

    struct SwsContext *context = m_dllSwScale.sws_getContext(im->width, im->height, PIX_FMT_YUV420P,
                                                             im->width, im->height, PIX_FMT_BGRA,
                                                             SWS_FAST_BILINEAR, NULL, NULL, NULL);
    uint8_t *src[] = { im->plane[0], im->plane[1], im->plane[2], 0 };
    int     srcStride[] = { im->stride[0], im->stride[1], im->stride[2], 0 };
    uint8_t *dst[] = { m_rgbBuffer, 0, 0, 0 };
    int     dstStride[] = { m_sourceWidth*4, 0, 0, 0 };
    m_dllSwScale.sws_scale(context, src, srcStride, 0, im->height, dst, dstStride);
    m_dllSwScale.sws_freeContext(context);
    SetEvent(m_eventTexturesDone[source]);
  }
  else if (IsSoftwareUpscaling()) // FIXME: s/w upscaling + RENDER_SW => broken
  {
    // Perform the scaling.
    uint8_t* src[] =       { im->plane[0],  im->plane[1],  im->plane[2], 0 };
    int      srcStride[] = { im->stride[0], im->stride[1], im->stride[2], 0 };
    uint8_t* dst[] =       { m_imScaled.plane[0],  m_imScaled.plane[1],  m_imScaled.plane[2], 0 };
    int      dstStride[] = { m_imScaled.stride[0], m_imScaled.stride[1], m_imScaled.stride[2], 0 };
    int      algorithm   = 0;

    switch (m_scalingMethod)
    {
    case VS_SCALINGMETHOD_BICUBIC_SOFTWARE: algorithm = SWS_BICUBIC; break;
    case VS_SCALINGMETHOD_LANCZOS_SOFTWARE: algorithm = SWS_LANCZOS; break;
    case VS_SCALINGMETHOD_SINC_SOFTWARE:    algorithm = SWS_SINC;    break;
    default: break;
    }

    struct SwsContext *ctx = m_dllSwScale.sws_getContext(im->width, im->height, PIX_FMT_YUV420P,
                                                         m_upscalingWidth, m_upscalingHeight, PIX_FMT_YUV420P,
                                                         algorithm, NULL, NULL, NULL);
    m_dllSwScale.sws_scale(ctx, src, srcStride, 0, im->height, dst, dstStride);
    m_dllSwScale.sws_freeContext(ctx);

    im = &m_imScaled;
    im->flags = IMAGE_FLAG_READY;
  }

  static int imaging = -1;
  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  if (imaging==-1)
  {
    imaging = 0;
    if (glewIsSupported("GL_ARB_imaging"))
    {
      CLog::Log(LOGINFO, "GL: ARB Imaging extension supported");
      imaging = 1;
    }
    else
    {
      unsigned int maj=0, min=0;
      g_Windowing.GetRenderVersion(maj, min);
      if (maj>=2)
      {
        imaging = 1;
      }
      else if (min>=2)
      {
        imaging = 1;
      }
    }
  }

  glEnable(m_textureTarget);
  VerifyGLState();

  if (m_renderMethod & RENDER_SW)
  {
    if (imaging==1 &&
        ((g_stSettings.m_currentVideoSettings.m_Brightness!=50) ||
         (g_stSettings.m_currentVideoSettings.m_Contrast!=50)))
    {
      GLfloat brightness = ((GLfloat)g_stSettings.m_currentVideoSettings.m_Brightness - 50.0f)/100.0f;;
      GLfloat contrast   = ((GLfloat)g_stSettings.m_currentVideoSettings.m_Contrast)/50.0f;

      glPixelTransferf(GL_RED_SCALE  , contrast);
      glPixelTransferf(GL_GREEN_SCALE, contrast);
      glPixelTransferf(GL_BLUE_SCALE , contrast);
      glPixelTransferf(GL_RED_BIAS   , brightness);
      glPixelTransferf(GL_GREEN_BIAS , brightness);
      glPixelTransferf(GL_BLUE_BIAS  , brightness);
      VerifyGLState();
      imaging++;
    }

    // Load RGB image
    if (deinterlacing)
    {
      LoadPlane( fields[FIELD_ODD][0] , GL_BGRA, buf.flipindex
               , im->width, im->height >> 1
               , m_sourceWidth*2, m_rgbBuffer );

      LoadPlane( fields[FIELD_EVEN][0], GL_BGRA, buf.flipindex
               , im->width, im->height >> 1
               , m_sourceWidth*2, m_rgbBuffer + m_sourceWidth*4);      
    }
    else
    {
      LoadPlane( fields[FIELD_FULL][0], GL_BGRA, buf.flipindex
               , im->width, im->height
               , m_sourceWidth, m_rgbBuffer );
    }

    if (imaging==2)
    {
      imaging--;
      glPixelTransferf(GL_RED_SCALE, 1.0);
      glPixelTransferf(GL_GREEN_SCALE, 1.0);
      glPixelTransferf(GL_BLUE_SCALE, 1.0);
      glPixelTransferf(GL_RED_BIAS, 0.0);
      glPixelTransferf(GL_GREEN_BIAS, 0.0);
      glPixelTransferf(GL_BLUE_BIAS, 0.0);
      VerifyGLState();
    }
  }
  else
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    if (deinterlacing)
    {
      // Load Y fields
      LoadPlane( fields[FIELD_ODD][0] , GL_LUMINANCE, buf.flipindex
               , im->width, im->height >> 1
               , im->stride[0]*2, im->plane[0] );

      LoadPlane( fields[FIELD_EVEN][0], GL_LUMINANCE, buf.flipindex
               , im->width, im->height >> 1
               , im->stride[0]*2, im->plane[0] + im->stride[0]) ;     
    }
    else
    {
      // Load Y plane
      LoadPlane( fields[FIELD_FULL][0], GL_LUMINANCE, buf.flipindex
               , im->width, im->height
               , im->stride[0], im->plane[0] );
    }
  }

  VerifyGLState();

  if (!(m_renderMethod & RENDER_SW))
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    if (deinterlacing)
    {
      // Load Even U & V Fields
      LoadPlane( fields[FIELD_ODD][1], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
               , im->stride[1]*2, im->plane[1] );

      LoadPlane( fields[FIELD_ODD][2], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
               , im->stride[2]*2, im->plane[2] );
      
      // Load Odd U & V Fields
      LoadPlane( fields[FIELD_EVEN][1], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
               , im->stride[1]*2, im->plane[1] + im->stride[1] );

      LoadPlane( fields[FIELD_EVEN][2], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
               , im->stride[2]*2, im->plane[2] + im->stride[2] );
      
    }
    else
    {
      LoadPlane( fields[FIELD_FULL][1], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> im->cshift_y
               , im->stride[1], im->plane[1] );

      LoadPlane( fields[FIELD_FULL][2], GL_LUMINANCE, buf.flipindex
               , im->width >> im->cshift_x, im->height >> im->cshift_y
               , im->stride[2], im->plane[2] );
    }
  }
  SetEvent(m_eventTexturesDone[source]);

  // calculate the source rectangle
  for(int field = 0; field < 3; field++)
  {
    for(int plane = 0; plane < 3; plane++)
    {
      YUVPLANE& p = fields[field][plane];

      /* software upscaling is precropped */
      if(IsSoftwareUpscaling())
        p.rect.SetRect(0, 0, im->width, im->height);
      else      
        p.rect = m_sourceRect;

      p.width  = im->width;
      p.height = im->height;

      if(field != FIELD_FULL)
      {
        /* correct for field offsets and chroma offsets */
        float offset_y = 0.5;
        if(plane != 0)
          offset_y += 0.5;
        if(field == FIELD_EVEN)
          offset_y *= -1;

        p.rect.y1 += offset_y;
        p.rect.y2 += offset_y;

        /* half the height if this is a field */
        p.height  *= 0.5f;
        p.rect.y1 *= 0.5f; 
        p.rect.y2 *= 0.5f;
      }

      if(plane != 0)
      {
        p.width   /= 1 << im->cshift_x;
        p.height  /= 1 << im->cshift_y;

        p.rect.x1 /= 1 << im->cshift_x;
        p.rect.x2 /= 1 << im->cshift_x;
        p.rect.y1 /= 1 << im->cshift_y;
        p.rect.y2 /= 1 << im->cshift_y;
      }

      if (m_textureTarget == GL_TEXTURE_2D)
      {
        p.height  /= p.texheight;
        p.rect.y1 /= p.texheight;
        p.rect.y2 /= p.texheight;
        p.width   /= p.texwidth;
        p.rect.x1 /= p.texwidth;
        p.rect.x2 /= p.texwidth;
      }
    }
  }

  glDisable(m_textureTarget);

  return(true);
}

void CLinuxRendererGL::Reset()
{
  for(int i=0; i<m_NumYV12Buffers; i++)
  {
    /* reset all image flags, this will cleanup textures later */
    m_buffers[i].image.flags = 0;
    /* reset texture locks, a bit ugly, could result in tearing */
    SetEvent(m_eventTexturesDone[i]);
  }
}

void CLinuxRendererGL::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
  ManageTextures();
}

void CLinuxRendererGL::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) return;

  // if its first pass, just init textures and return
  if (ValidateRenderTarget())
    return;

  // this needs to be checked after texture validation
  if (!m_bImageReady) return;

  int index = m_iYV12RenderBuffer;
  YUVBUFFER& buf =  m_buffers[index];

  if (!buf.fields[FIELD_FULL][0].id) return ;

  if (buf.image.flags==0)
    return;

  ManageDisplay();
  ManageTextures();

  g_graphicsContext.BeginPaint();

  if( WaitForSingleObject(m_eventTexturesDone[index], 500) == WAIT_TIMEOUT )
  {
    CLog::Log(LOGWARNING, "%s - Timeout waiting for texture %d", __FUNCTION__, index);

    // render the previous frame if this one isn't ready yet
    if (m_iLastRenderBuffer > -1)
    {
      m_iYV12RenderBuffer = m_iLastRenderBuffer;
      index = m_iYV12RenderBuffer;
    }
  }
  else
    m_iLastRenderBuffer = index;

  if (clear)
  {
    glClearColor(m_clearColour, m_clearColour, m_clearColour, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0,0,0,0);
  }

  if (alpha<255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, alpha / 255.0f);
  }
  else
  {
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  }

  if( (flags & RENDER_FLAG_ODD)
   && (flags & RENDER_FLAG_EVEN) )
  {
    glEnable(GL_POLYGON_STIPPLE);

    glPolygonStipple(stipple_weave);
    Render(flags & ~RENDER_FLAG_EVEN, index);
    glPolygonStipple(stipple_weave+4);
    Render(flags & ~RENDER_FLAG_ODD , index);

    glDisable(GL_POLYGON_STIPPLE);
  }
  else
    Render(flags, index);

  VerifyGLState();
  glEnable(GL_BLEND);
  glFlush();

  g_graphicsContext.EndPaint();
}

void CLinuxRendererGL::FlipPage(int source)
{
#ifdef HAVE_LIBVDPAU
  if (g_VDPAU)
    g_VDPAU->Present();
#endif

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();

  m_buffers[m_iYV12RenderBuffer].flipindex = ++m_flipindex;

  return;
}


unsigned int CLinuxRendererGL::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  BYTE *s;
  BYTE *d;
  int i, p;

  int index = NextYV12Texture();
  if( index < 0 )
    return -1;

  YV12Image &im = m_buffers[index].image;
  // copy Y
  p = 0;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  w >>= im.cshift_x; h >>= im.cshift_y;
  x >>= im.cshift_x; y >>= im.cshift_y;

  // copy U
  p = 1;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  // copy V
  p = 2;
  d = (BYTE*)im.plane[p] + im.stride[p] * y + x;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += im.stride[p];
  }

  SetEvent(m_eventTexturesDone[index]);
  return 0;
}

unsigned int CLinuxRendererGL::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  m_bValidated = false;
  UnInit();
  m_resolution = RES_PAL_4x3;
  m_crop.x1 = m_crop.x2 = 0.0f;
  m_crop.y1 = m_crop.y2 = 0.0f;

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 2;

  // setup the background colour
  m_clearColour = (float)(g_advancedSettings.m_videoBlackBarColour & 0xff) / 0xff;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllSwScale.Load())
    CLog::Log(LOGERROR,"CLinuxRendererGL::PreInit - failed to load rescale libraries!");

  #if (! defined USE_EXTERNAL_FFMPEG)
    m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #elif (defined HAVE_LIBSWSCALE_RGB2RGB_H) || (defined HAVE_FFMPEG_RGB2RGB_H)
    m_dllSwScale.sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);
  #endif

  return true;
}

void CLinuxRendererGL::UpdateVideoFilter()
{
  if (m_scalingMethod == g_stSettings.m_currentVideoSettings.m_ScalingMethod)
    return;
  m_scalingMethod = g_stSettings.m_currentVideoSettings.m_ScalingMethod;

  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.Cleanup();

  VerifyGLState();

  switch (g_stSettings.m_currentVideoSettings.m_ScalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
    SetTextureFilter(GL_NEAREST);
    m_renderQuality = RQ_SINGLEPASS;
    return;

  case VS_SCALINGMETHOD_LINEAR:
    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_SINGLEPASS;
    return;

  case VS_SCALINGMETHOD_CUBIC:
    if(!glewIsSupported("GL_ARB_texture_float"))
    {
      CLog::Log(LOGERROR, "GL: hardware doesn't support GL_ARB_texture_float");
      break;
    }

    if (!m_fbo.Initialize())
    {
      CLog::Log(LOGERROR, "GL: Error initializing FBO");
      break;
    }

    if (!m_fbo.CreateAndBindToTexture(GL_TEXTURE_2D, m_sourceWidth, m_sourceHeight, GL_RGBA))
    {
      CLog::Log(LOGERROR, "GL: Error creating texture and binding to FBO");
      break;
    }

    m_pVideoFilterShader = new BicubicFilterShader(0.3f, 0.3f);
    if (!m_pVideoFilterShader->CompileAndLink())
    {
      CLog::Log(LOGERROR, "GL: Error compiling and linking video filter shader");
      break;
    }

    SetTextureFilter(GL_LINEAR);
    m_renderQuality = RQ_MULTIPASS;
    return;

  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3:
  case VS_SCALINGMETHOD_SINC8:
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "GL: TODO: This scaler has not yet been implemented");
    break;

  case VS_SCALINGMETHOD_BICUBIC_SOFTWARE:
  case VS_SCALINGMETHOD_LANCZOS_SOFTWARE:
  case VS_SCALINGMETHOD_SINC_SOFTWARE:
    InitializeSoftwareUpscaling();
    m_renderQuality = RQ_SINGLEPASS;
    return;

  default:
    break;
  }

  g_application.m_guiDialogKaiToast.QueueNotification("Video Renderering", "Failed to init video filters/scalers, falling back to bilinear scaling");
  CLog::Log(LOGERROR, "GL: Falling back to bilinear due to failure to init scaler");
  if (m_pVideoFilterShader)
  {
    m_pVideoFilterShader->Free();
    delete m_pVideoFilterShader;
    m_pVideoFilterShader = NULL;
  }
  m_fbo.Cleanup();

  SetTextureFilter(GL_LINEAR);
  m_renderQuality = RQ_SINGLEPASS;
}

void CLinuxRendererGL::LoadShaders(int field)
{
  int requestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");
  CLog::Log(LOGDEBUG, "GL: Requested render method: %d", requestedMethod);
  bool err = false;

#ifdef HAVE_LIBVDPAU
  if (g_VDPAU)
  {
    CLog::Log(LOGNOTICE, "GL: Using VDPAU render method");
    m_renderMethod = RENDER_VDPAU;
  }
  else 
#endif //HAVE_LIBVDPAU
#ifdef HAVE_LIBCRYSTALHD
  /*
    Default to VDPAU if supported.
   */
  if (g_CrystalHD)
  {
    err = false;
    CLog::Log(LOGNOTICE, "GL: Using Crystal HD render method");
    m_renderMethod = RENDER_CRYSTALHD;
    if (m_pYUVShader)
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }

    // create regular progressive scan shader
    m_pYUVShader = new NV12ToRGBProgressiveShaderARB(m_textureTarget==GL_TEXTURE_RECTANGLE_ARB, m_iFlags);
    CLog::Log(LOGNOTICE, "GL: Selecting Single Pass ARB NV12ToRGB shader");

    if (m_pYUVShader && m_pYUVShader->CompileAndLink())
    {
      UpdateVideoFilter();
    }
    else
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
      err = true;
      CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB ARB shader");
    }
  }
  else 
#endif //HAVE_LIBCRYSTALHD
  /*
    Try GLSL shaders if they're supported and if the user has
    requested for it. (settings -> video -> player -> rendermethod)
   */
  if (glCreateProgram // TODO: proper check
      && (requestedMethod==RENDER_METHOD_AUTO || requestedMethod==RENDER_METHOD_GLSL
            || requestedMethod==RENDER_METHOD_VDPAU))
  {
    if (m_pYUVShader)
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }

    // create regular progressive scan shader
    m_pYUVShader = new YUV2RGBProgressiveShader(m_textureTarget==GL_TEXTURE_RECTANGLE_ARB, m_iFlags);
    CLog::Log(LOGNOTICE, "GL: Selecting Single Pass YUV 2 RGB shader");

    if (m_pYUVShader && m_pYUVShader->CompileAndLink())
    {
      m_renderMethod = RENDER_GLSL;
      UpdateVideoFilter();
    }
    else
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
      err = true;
      CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB GLSL shader");
    }
  }

  /*
    Try ARB shaders if the extension is supported AND either:
      1) user requested it
      2) or GLSL shaders failed and user selected AUTO
   */
  else if (glewIsSupported("GL_ARB_fragment_program")  
           && ((requestedMethod==RENDER_METHOD_AUTO || requestedMethod==RENDER_METHOD_ARB)
               || err))
  {
    err = false;
    CLog::Log(LOGNOTICE, "GL: ARB shaders support detected");
    m_renderMethod = RENDER_ARB ;
    if (m_pYUVShader)
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
    }

    // create regular progressive scan shader
    m_pYUVShader = new YUV2RGBProgressiveShaderARB(m_textureTarget==GL_TEXTURE_RECTANGLE_ARB, m_iFlags);
    CLog::Log(LOGNOTICE, "GL: Selecting Single Pass ARB YUV2RGB shader");

    if (m_pYUVShader && m_pYUVShader->CompileAndLink())
    {
      m_renderMethod = RENDER_ARB;
      UpdateVideoFilter();
    }
    else
    {
      m_pYUVShader->Free();
      delete m_pYUVShader;
      m_pYUVShader = NULL;
      err = true;
      CLog::Log(LOGERROR, "GL: Error enabling YUV2RGB ARB shader");
    }
  }

  /*
    Fall back to software YUV 2 RGB conversion if
      1) user requested it
      2) or GLSL and/or ARB shaders failed
   */
  else
  {
    m_renderMethod = RENDER_SW ;
    CLog::Log(LOGNOTICE, "GL: Shaders support not present, falling back to SW mode");
  }

  if (err==true)
  {
    CLog::Log(LOGERROR, "GL: Falling back to Software YUV2RGB");
    m_renderMethod = RENDER_SW;
  }

  // determine whether GPU supports NPOT textures
  if (!glewIsSupported("GL_ARB_texture_non_power_of_two"))
  {
    if (!glewIsSupported("GL_ARB_texture_rectangle"))
    {
      CLog::Log(LOGNOTICE, "GL: GL_ARB_texture_rectangle not supported and OpenGL version is not 2.x");
      CLog::Log(LOGNOTICE, "GL: Reverting to POT textures");
      m_renderMethod |= RENDER_POT;
    }
    else
      CLog::Log(LOGNOTICE, "GL: NPOT textures are supported through GL_ARB_texture_rectangle extension");
  }
  else
    CLog::Log(LOGNOTICE, "GL: NPOT texture support detected");

  // Now that we now the render method, setup texture function handlers
#ifdef HAVE_LIBVDPAU
  if (m_renderMethod & RENDER_VDPAU)
  {
    LoadTexturesFuncPtr  = &CLinuxRendererGL::LoadVDPAUTextures;
    CreateTextureFuncPtr = &CLinuxRendererGL::CreateVDPAUTexture;
    DeleteTextureFuncPtr = &CLinuxRendererGL::DeleteVDPAUTexture;
  }
else 
#endif
#ifdef HAVE_LIBCRYSTALHD
  // FIXME: add m_renderMethod for RENDER_CRYSTALHD
  if (m_renderMethod & RENDER_CRYSTALHD)
  {
    LoadTexturesFuncPtr  = &CLinuxRendererGL::LoadCrystalHDTextures;
    CreateTextureFuncPtr = &CLinuxRendererGL::CreateCrystalHDTexture;
    DeleteTextureFuncPtr = &CLinuxRendererGL::DeleteCrystalHDTexture;
  }
  else
#endif
  {
    // setup default YV12 texture handlers
    LoadTexturesFuncPtr  = &CLinuxRendererGL::LoadYV12Textures;
    CreateTextureFuncPtr = &CLinuxRendererGL::CreateYV12Texture;
    DeleteTextureFuncPtr = &CLinuxRendererGL::DeleteYV12Texture;
  }
}

void CLinuxRendererGL::UnInit()
{
  CLog::Log(LOGDEBUG, "LinuxRendererGL: Cleaning up GL resources");
  CSingleLock lock(g_graphicsContext);

  if (m_rgbBuffer != NULL)
  {
    delete [] m_rgbBuffer;
    m_rgbBuffer = NULL;
  }
  m_rgbBufferSize = 0;

#ifdef HAVE_LIBVDPAU
  if (g_VDPAU)
    g_VDPAU->ReleasePixmap();
#endif
  // YV12 textures
  if (DeleteTextureFuncPtr)
  {
    for (int i = 0; i < NUM_BUFFERS; ++i)
      (this->*DeleteTextureFuncPtr)(i);
  }
  // cleanup framebuffer object if it was in use
  m_fbo.Cleanup();
  m_bValidated = false;
  m_bImageReady = false;
  m_bConfigured = false;
}

void CLinuxRendererGL::Render(DWORD flags, int renderBuffer)
{
  // obtain current field, if interlaced
  if( flags & RENDER_FLAG_ODD)
    m_currentField = FIELD_ODD;

  else if (flags & RENDER_FLAG_EVEN)
    m_currentField = FIELD_EVEN;

  else if (flags & RENDER_FLAG_LAST)
  {
    switch(m_currentField)
    {
    case FIELD_ODD:
      flags = RENDER_FLAG_ODD;
      break;

    case FIELD_EVEN:
      flags = RENDER_FLAG_EVEN;
      break;
    }
  }
  else
    m_currentField = FIELD_FULL;

  if (LoadTexturesFuncPtr)
    (this->*LoadTexturesFuncPtr)(renderBuffer);

  if (m_renderMethod & RENDER_GLSL)
  {
    UpdateVideoFilter();
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      RenderSinglePass(renderBuffer, m_currentField);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      RenderMultiPass(renderBuffer, m_currentField);
      VerifyGLState();
      break;

    case RQ_SOFTWARE:
      RenderSoftware(renderBuffer, m_currentField);
      VerifyGLState();
      break;
    }
  }
  else if (m_renderMethod & RENDER_ARB)
  {
    RenderSinglePass(renderBuffer, m_currentField);
  }
#ifdef HAVE_LIBVDPAU
  else if (m_renderMethod & RENDER_VDPAU)
  {
    RenderVDPAU(renderBuffer, m_currentField);
  }
#endif
#ifdef HAVE_LIBCRYSTALHD
  else if (m_renderMethod & RENDER_CRYSTALHD)
  {
    RenderCrystalHD(renderBuffer, m_currentField);
  }
#endif
  else
  {
    RenderSoftware(renderBuffer, m_currentField);
    VerifyGLState();
  }
}

void CLinuxRendererGL::AutoCrop(bool bCrop)
{
  RECT crop;

  if(!m_bValidated) return;

  if (bCrop)
    CBaseRenderer::AutoCrop(m_buffers[m_iYV12RenderBuffer].image, crop);
  else
  { // reset to defaults
    crop.left   = 0;
    crop.right  = 0;
    crop.top    = 0;
    crop.bottom = 0;
  }

  m_crop.x1 += ((float)crop.left   - m_crop.x1) * 0.1;
  m_crop.x2 += ((float)crop.right  - m_crop.x2) * 0.1;
  m_crop.y1 += ((float)crop.top    - m_crop.y1) * 0.1;
  m_crop.y2 += ((float)crop.bottom - m_crop.y2) * 0.1;

  crop.left   = MathUtils::round_int(m_crop.x1);
  crop.right  = MathUtils::round_int(m_crop.x2);
  crop.top    = MathUtils::round_int(m_crop.y1);
  crop.bottom = MathUtils::round_int(m_crop.y2);

  //compare with hysteresis
# define HYST(n, o) ((n) > (o) || (n) + 1 < (o))
  if(HYST(g_stSettings.m_currentVideoSettings.m_CropLeft  , crop.left)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropRight , crop.right)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropTop   , crop.top)
  || HYST(g_stSettings.m_currentVideoSettings.m_CropBottom, crop.bottom))
  {
    g_stSettings.m_currentVideoSettings.m_CropLeft   = crop.left;
    g_stSettings.m_currentVideoSettings.m_CropRight  = crop.right;
    g_stSettings.m_currentVideoSettings.m_CropTop    = crop.top;
    g_stSettings.m_currentVideoSettings.m_CropBottom = crop.bottom;
    SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
  }
# undef HYST
}

void CLinuxRendererGL::RenderSinglePass(int index, int field)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[field];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(field);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glActiveTextureARB(GL_TEXTURE0);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[0].id);

  // U
  glActiveTextureARB(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);

  // V
  glActiveTextureARB(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);

  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  m_pYUVShader->SetBlack(g_stSettings.m_currentVideoSettings.m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(g_stSettings.m_currentVideoSettings.m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(im.width);
  m_pYUVShader->SetHeight(im.height);
  if     (field == FIELD_ODD)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_EVEN)
    m_pYUVShader->SetField(0);

  m_pYUVShader->Enable();

  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y1);
  glVertex4f(m_destRect.x1, m_destRect.y1, 0, 1.0f );

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y1);
  glVertex4f(m_destRect.x2, m_destRect.y1, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y2);
  glVertex4f(m_destRect.x2, m_destRect.y2, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y2);
  glVertex4f(m_destRect.x1, m_destRect.y2, 0, 1.0f);

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();
  VerifyGLState();

  glActiveTextureARB(GL_TEXTURE1);
  glDisable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE2);
  glDisable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE0);
  glDisable(m_textureTarget);

  glMatrixMode(GL_MODELVIEW);

  VerifyGLState();
}

void CLinuxRendererGL::RenderMultiPass(int index, int field)
{
  YV12Image &im     = m_buffers[index].image;
  YUVPLANES &planes = m_buffers[index].fields[field];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(m_currentField);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  VerifyGLState();

  // U
  glActiveTextureARB(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);
  VerifyGLState();

  // V
  glActiveTextureARB(GL_TEXTURE2);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[2].id);
  VerifyGLState();

  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  // make sure the yuv shader is loaded and ready to go
  if (!m_pYUVShader || (!m_pYUVShader->OK()))
  {
    CLog::Log(LOGERROR, "GL: YUV shader not active, cannot do multipass render");
    return;
  }

  m_fbo.BeginRender();
  VerifyGLState();

  m_pYUVShader->SetBlack(g_stSettings.m_currentVideoSettings.m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(g_stSettings.m_currentVideoSettings.m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(im.width);
  m_pYUVShader->SetHeight(im.height);
  if     (field == FIELD_ODD)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_EVEN)
    m_pYUVShader->SetField(0);

  VerifyGLState();

  glPushAttrib(GL_VIEWPORT_BIT);
  glPushAttrib(GL_SCISSOR_BIT);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  VerifyGLState();
  gluOrtho2D(0, m_sourceWidth, 0, m_sourceHeight);
  glViewport(0, 0, m_sourceWidth, m_sourceHeight);
  glScissor (0, 0, m_sourceWidth, m_sourceHeight);
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();


  if (!m_pYUVShader->Enable())
  {
    CLog::Log(LOGERROR, "GL: Error enabling YUV shader");
  }

  float imgwidth  = planes[0].rect.x2 - planes[0].rect.x1;
  float imgheight = planes[0].rect.y2 - planes[0].rect.y1;
  if (m_textureTarget == GL_TEXTURE_2D)
  {
    imgwidth  *= planes[0].texwidth;
    imgheight *= planes[0].texheight;
  }

  // 1st Pass to video frame size
  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y1);
  glVertex2f(0.0f    , 0.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y1);
  glVertex2f(imgwidth, 0.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x2, planes[2].rect.y2);
  glVertex2f(imgwidth, imgheight);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE2, planes[2].rect.x1, planes[2].rect.y2);
  glVertex2f(0.0f    , imgheight);

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix(); // pop modelview
  glMatrixMode(GL_PROJECTION);
  glPopMatrix(); // pop projection
  glPopAttrib(); // pop scissor
  glPopAttrib(); // pop viewport
  glMatrixMode(GL_MODELVIEW);
  VerifyGLState();

  m_fbo.EndRender();

  glActiveTextureARB(GL_TEXTURE1);
  glDisable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE2);
  glDisable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glDisable(m_textureTarget);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_fbo.Texture());
  VerifyGLState();

  // Use regular normalized texture coordinates

  // 2nd Pass to screen size with optional video filter

  if (m_pVideoFilterShader)
  {
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_NEAREST);
    m_pVideoFilterShader->SetSourceTexture(0);
    m_pVideoFilterShader->SetWidth(m_sourceWidth);
    m_pVideoFilterShader->SetHeight(m_sourceHeight);
    m_pVideoFilterShader->Enable();
  }
  else
    m_fbo.SetFiltering(GL_TEXTURE_2D, GL_LINEAR);

  VerifyGLState();

  imgwidth  /= m_sourceWidth;
  imgheight /= m_sourceHeight;

  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f    , 0.0f);
  glVertex4f(m_destRect.x1, m_destRect.y1, 0, 1.0f );

  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, 0.0f);
  glVertex4f(m_destRect.x2, m_destRect.y1, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, imgwidth, imgheight);
  glVertex4f(m_destRect.x2, m_destRect.y2, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f    , imgheight);
  glVertex4f(m_destRect.x1, m_destRect.y2, 0, 1.0f);

  glEnd();

  VerifyGLState();

  if (m_pVideoFilterShader)
    m_pVideoFilterShader->Disable();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
}

void CLinuxRendererGL::RenderVDPAU(int index, int field)
{
#ifdef HAVE_LIBVDPAU
  if (!g_VDPAU)
  {
    CLog::Log(LOGERROR,"(VDPAU) m_Surface is NULL");
    return;
  }

  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  glEnable(m_textureTarget);
  
  if (!g_VDPAU->m_glPixmapTexture)
  {
    glGenTextures (1, &(g_VDPAU->m_glPixmapTexture));
    CLog::Log(LOGNOTICE,"Created m_glPixmapTexture (%i)",(int)g_VDPAU->m_glPixmapTexture);
  }
  
  glBindTexture(m_textureTarget, g_VDPAU->m_glPixmapTexture);
  g_VDPAU->BindPixmap();

  glActiveTextureARB(GL_TEXTURE0);

  // Try some clamping or wrapping
  glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  VerifyGLState();

  glBegin(GL_QUADS);
  if (m_textureTarget==GL_TEXTURE_2D)
  {
    glTexCoord2f(0.0, 0.0);  glVertex2f(m_destRect.x1, m_destRect.y1);
    glTexCoord2f(1.0, 0.0);  glVertex2f(m_destRect.x2, m_destRect.y1);
    glTexCoord2f(1.0, 1.0);  glVertex2f(m_destRect.x2, m_destRect.y2);
    glTexCoord2f(0.0, 1.0);  glVertex2f(m_destRect.x1, m_destRect.y2);
  }
  else
  {
    glTexCoord2f(m_destRect.x1, m_destRect.y1); glVertex4f(m_destRect.x1, m_destRect.y1, 0.0f, 0.0f);
    glTexCoord2f(m_destRect.x2, m_destRect.y1); glVertex4f(m_destRect.x2, m_destRect.y1, 1.0f, 0.0f);
    glTexCoord2f(m_destRect.x2, m_destRect.y2); glVertex4f(m_destRect.x2, m_destRect.y2, 1.0f, 1.0f);
    glTexCoord2f(m_destRect.x1, m_destRect.y2); glVertex4f(m_destRect.x1, m_destRect.y2, 0.0f, 1.0f);
  }
  glEnd();
  VerifyGLState();
  if (m_StrictBinding)
  {
    glBindTexture(m_textureTarget, g_VDPAU->m_glPixmapTexture);
    g_VDPAU->ReleasePixmap();
  }

  glBindTexture (m_textureTarget, 0);
  glDisable(m_textureTarget);
#endif
}

void CLinuxRendererGL::RenderCrystalHD(int index, int field)
{
#ifdef HAVE_LIBCRYSTALHD
  if (!g_CrystalHD)
  {
    CLog::Log(LOGERROR,"(CrystalHD) m_Surface is NULL");
    return;
  }

  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANES &planes = fields[field];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  if (m_reloadShaders)
  {
    m_reloadShaders = 0;
    LoadShaders(field);
  }

  glDisable(GL_DEPTH_TEST);

  // Y
  glActiveTextureARB(GL_TEXTURE0);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[0].id);

  // UV
  glActiveTextureARB(GL_TEXTURE1);
  glEnable(m_textureTarget);
  glBindTexture(m_textureTarget, planes[1].id);

  glActiveTextureARB(GL_TEXTURE0);
  VerifyGLState();

  m_pYUVShader->SetBlack(g_stSettings.m_currentVideoSettings.m_Brightness * 0.01f - 0.5f);
  m_pYUVShader->SetContrast(g_stSettings.m_currentVideoSettings.m_Contrast * 0.02f);
  m_pYUVShader->SetWidth(im.width);
  m_pYUVShader->SetHeight(im.height);
  if     (field == FIELD_ODD)
    m_pYUVShader->SetField(1);
  else if(field == FIELD_EVEN)
    m_pYUVShader->SetField(0);

  m_pYUVShader->Enable();

  glBegin(GL_QUADS);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y1);
  glVertex4f(m_destRect.x1, m_destRect.y1, 0, 1.0f );

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y1);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y1);
  glVertex4f(m_destRect.x2, m_destRect.y1, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x2, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x2, planes[1].rect.y2);
  glVertex4f(m_destRect.x2, m_destRect.y2, 0, 1.0f);

  glMultiTexCoord2fARB(GL_TEXTURE0, planes[0].rect.x1, planes[0].rect.y2);
  glMultiTexCoord2fARB(GL_TEXTURE1, planes[1].rect.x1, planes[1].rect.y2);
  glVertex4f(m_destRect.x1, m_destRect.y2, 0, 1.0f);

  glEnd();
  VerifyGLState();

  m_pYUVShader->Disable();
  VerifyGLState();

  glActiveTextureARB(GL_TEXTURE1);
  glDisable(m_textureTarget);

  glActiveTextureARB(GL_TEXTURE0);
  glDisable(m_textureTarget);

  glMatrixMode(GL_MODELVIEW);

  VerifyGLState();
#endif
}

void CLinuxRendererGL::RenderSoftware(int index, int field)
{
  YUVPLANES &planes = m_buffers[index].fields[field];

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTextureARB(GL_TEXTURE0);
  glBindTexture(m_textureTarget, planes[0].id);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);
  glTexCoord2f(planes[0].rect.x1, planes[0].rect.y1);
  glVertex4f(m_destRect.x1, m_destRect.y1, 0, 1.0f );

  glTexCoord2f(planes[0].rect.x2, planes[0].rect.y1);
  glVertex4f(m_destRect.x2, m_destRect.y1, 0, 1.0f);

  glTexCoord2f(planes[0].rect.x2, planes[0].rect.y2);
  glVertex4f(m_destRect.x2, m_destRect.y2, 0, 1.0f);

  glTexCoord2f(planes[0].rect.x1, planes[0].rect.y2);
  glVertex4f(m_destRect.x1, m_destRect.y2, 0, 1.0f);

  glEnd();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();
}

void CLinuxRendererGL::CreateThumbnail(CBaseTexture* texture, unsigned int width, unsigned int height)
{
  // get our screen rect
  const CRect& rv = g_graphicsContext.GetViewWindow();

  // save current video rect
  CRect saveSize = m_destRect;

  // new video rect is thumbnail size
  m_destRect.SetRect(0, 0, (float)width, (float)height);

  // clear framebuffer and invert Y axis to get non-inverted image
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0, 0, 0, 0);
  glDisable(GL_BLEND);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(0, height, 0);
  glScalef(1.0, -1.0f, 1.0f);
  Render(RENDER_FLAG_NOOSD, m_iYV12RenderBuffer);

  // read pixels
  glReadPixels(0, rv.y2-height, width, height, GL_BGRA, GL_UNSIGNED_BYTE, texture->GetPixels());

  // revert model view matrix
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // restore original video rect
  m_destRect = saveSize;
}

//********************************************************************************************************
// YV12 Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGL::DeleteYV12Texture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  if( fields[FIELD_FULL][0].id == 0 ) return(true);

  CLog::Log(LOGDEBUG, "Deleted YV12 texture %i", index);
  /* finish up all textures, and delete them */
  g_graphicsContext.BeginPaint();  //FIXME
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<MAX_PLANES;p++)
    {
      if( fields[f][p].id )
      {
        if (glIsTexture(fields[f][p].id))
        {
          glDeleteTextures(1, &fields[f][p].id);
          CLog::Log(LOGDEBUG, "GL: Deleting texture field %d plane %d", f+1, p+1);
        }
        fields[f][p].id = 0;
      }
    }
  }
  g_graphicsContext.EndPaint();

  for(int p = 0;p<MAX_PLANES;p++)
  {
    if (im.plane[p])
    {
      delete[] im.plane[p];
      im.plane[p] = NULL;
    }
  }
  
  return(true);
}

bool CLinuxRendererGL::CreateYV12Texture(int index)
{
/*
  // Remember if we're software upscaling.
  m_isSoftwareUpscaling = IsSoftwareUpscaling();

  /* since we also want the field textures, pitch must be texture aligned */
  unsigned p;

  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  // Delte any old texture
  if (DeleteTextureFuncPtr)
    (this->*DeleteTextureFuncPtr)(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  im.stride[0] = im.width;
  im.stride[1] = im.width >> im.cshift_x;
  im.stride[2] = im.width >> im.cshift_x;
  im.plane[0] = new BYTE[im.stride[0] * im.height];
  im.plane[1] = new BYTE[im.stride[1] * ( im.height >> im.cshift_y )];
  im.plane[2] = new BYTE[im.stride[2] * ( im.height >> im.cshift_y )];

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(p = 0;p<MAX_PLANES;p++)
    {
      if (!glIsTexture(fields[f][p].id))
      {
        glGenTextures(1, &fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_EVEN ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANES &planes = fields[f];

    if(m_isSoftwareUpscaling)
    {
      planes[0].texwidth  = m_upscalingWidth;
      planes[0].texheight = m_upscalingHeight >> fieldshift;
    }
    else
    {
      planes[0].texwidth  = im.width;
      planes[0].texheight = im.height >> fieldshift;
    }

    if (m_renderMethod & RENDER_SW)
    {
      planes[1].texwidth  = 0;
      planes[1].texheight = 0;
      planes[2].texwidth  = 0;
      planes[2].texheight = 0;
    }
    else
    {
      planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
      planes[1].texheight = planes[0].texheight >> im.cshift_y;
      planes[2].texwidth  = planes[0].texwidth  >> im.cshift_x;
      planes[2].texheight = planes[0].texheight >> im.cshift_y;
    }

    if(m_renderMethod & RENDER_POT)
    {
      for(int p = 0; p < 3; p++)
      {
        planes[p].texwidth  = NP2(planes[p].texwidth);
        planes[p].texheight = NP2(planes[p].texheight);
      }
    }

    for(int p = 0; p < 3; p++)
    {
      YUVPLANE &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
        continue;

      glBindTexture(m_textureTarget, plane.id);
      if (m_renderMethod & RENDER_SW)
      {
        if(m_renderMethod & RENDER_POT)
          CLog::Log(LOGNOTICE, "GL: Creating RGB POT texture of size %d x %d",  plane.texwidth, plane.texheight);
        else
          CLog::Log(LOGDEBUG,  "GL: Creating RGB NPOT texture of size %d x %d", plane.texwidth, plane.texheight);

        glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      } 
      else
      {
        if(m_renderMethod & RENDER_POT)
          CLog::Log(LOGNOTICE, "GL: Creating YUV POT texture of size %d x %d",  plane.texwidth, plane.texheight);
        else
          CLog::Log(LOGDEBUG,  "GL: Creating YUV NPOT texture of size %d x %d", plane.texwidth, plane.texheight);

        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
      }

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  glDisable(m_textureTarget);
  SetEvent(m_eventTexturesDone[index]);
  return true;
}

//********************************************************************************************************
// VDPAU Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGL::LoadVDPAUTextures(int source)
{
#ifdef HAVE_LIBVDPAU
  if (g_VDPAU)
    g_VDPAU->CheckRecover();
#endif
  SetEvent(m_eventTexturesDone[source]);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  return(true);
}
bool CLinuxRendererGL::CreateVDPAUTexture(int index)
{
  SetEvent(m_eventTexturesDone[index]);
  return(true);
}
bool CLinuxRendererGL::DeleteVDPAUTexture(int index)
{
  return(true);
}

//********************************************************************************************************
// CrystalHD Texture loading, creation and deletion
//********************************************************************************************************
bool CLinuxRendererGL::LoadCrystalHDTextures(int source)
{
#ifdef HAVE_LIBCRYSTALHD
  YUVBUFFER& buf    =  m_buffers[source];
  YV12Image* im     = &buf.image;
  YUVFIELDS& fields =  buf.fields;

  if (!(im->flags & IMAGE_FLAG_READY))
  {
    SetEvent(m_eventTexturesDone[source]);
    return(true);
  }

  if (g_CrystalHD)
  {
    g_CrystalHD->LoadNV12Pointers(im);
  }
  else
  {
    SetEvent(m_eventTexturesDone[source]);
    return(true);
  }

  if (!im->plane[0] || !im->plane[1])
  {
    SetEvent(m_eventTexturesDone[source]);
    return(true);
  }
  
  static int imaging = -1;
  bool deinterlacing;
  if (m_currentField == FIELD_FULL)
    deinterlacing = false;
  else
    deinterlacing = true;

  if (imaging==-1)
  {
    imaging = 0;
    if (glewIsSupported("GL_ARB_imaging"))
    {
      CLog::Log(LOGINFO, "GL: ARB Imaging extension supported");
      imaging = 1;
    }
    else
    {
      unsigned int maj=0, min=0;
      g_Windowing.GetRenderVersion(maj, min);
      if (maj>=2)
      {
        imaging = 1;
      }
      else if (min>=2)
      {
        imaging = 1;
      }
    }
  }

  glEnable(m_textureTarget);
  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load Y fields
    LoadPlane( fields[FIELD_ODD][0] , GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->plane[0] );

    LoadPlane( fields[FIELD_EVEN][0], GL_LUMINANCE, buf.flipindex
             , im->width, im->height >> 1
             , im->stride[0]*2, im->plane[0] + im->stride[0]) ;     
  }
  else
  {
    // Load Y plane
    LoadPlane( fields[FIELD_FULL][0], GL_LUMINANCE, buf.flipindex
             , im->width, im->height
             , im->stride[0], im->plane[0] );
  }

  VerifyGLState();

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (deinterlacing)
  {
    // Load Even U & V Fields
    LoadPlane( fields[FIELD_ODD][1], GL_LUMINANCE, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->plane[1] );

    // Load Odd U & V Fields
    LoadPlane( fields[FIELD_EVEN][1], GL_LUMINANCE, buf.flipindex
             , im->width >> im->cshift_x, im->height >> (im->cshift_y + 1)
             , im->stride[1]*2, im->plane[1] + im->stride[1] );
  }
  else
  {
    LoadPlane( fields[FIELD_FULL][1], GL_LUMINANCE_ALPHA, buf.flipindex
             , im->width >> im->cshift_x, im->height >> im->cshift_y
             , im->stride[1], im->plane[1] );
  }

  SetEvent(m_eventTexturesDone[source]);

  // calculate the source rectangle
  for(int field = 0; field < MAX_FIELDS; field++)
  {
    for(int plane = 0; plane < MAX_PLANES-1; plane++)
    {
      YUVPLANE& p = fields[field][plane];

      p.rect = m_sourceRect;

      p.width  = im->width;
      p.height = im->height;

      if(field != FIELD_FULL)
      {
        /* correct for field offsets and chroma offsets */
        float offset_y = 0.5;
        if(plane != 0)
          offset_y += 0.5;
        if(field == FIELD_EVEN)
          offset_y *= -1;

        p.rect.y1 += offset_y;
        p.rect.y2 += offset_y;

        /* half the height if this is a field */
        p.height  *= 0.5f;
        p.rect.y1 *= 0.5f; 
        p.rect.y2 *= 0.5f;
      }

      if(plane != 0)
      {
        p.width   /= 1 << im->cshift_x;
        p.height  /= 1 << im->cshift_y;

        p.rect.x1 /= 1 << im->cshift_x;
        p.rect.x2 /= 1 << im->cshift_x;
        p.rect.y1 /= 1 << im->cshift_y;
        p.rect.y2 /= 1 << im->cshift_y;
      }

      if (m_textureTarget == GL_TEXTURE_2D)
      {
        p.height  /= p.texheight;
        p.rect.y1 /= p.texheight;
        p.rect.y2 /= p.texheight;
        p.width   /= p.texwidth;
        p.rect.x1 /= p.texwidth;
        p.rect.x2 /= p.texwidth;
      }
    }
  }

  glDisable(m_textureTarget);
#endif

  return(true);
}
bool CLinuxRendererGL::CreateCrystalHDTexture(int index)
{
#ifdef HAVE_LIBCRYSTALHD
  // since we also want the field textures, pitch must be texture aligned
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  // Delte any old texture
  if (DeleteTextureFuncPtr)
    (this->*DeleteTextureFuncPtr)(index);

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  im.stride[0] = im.width;
  im.stride[1] = im.width >> im.cshift_x;

  im.plane[0] = NULL;
  im.plane[1] = NULL;

  glEnable(m_textureTarget);
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<MAX_PLANES-1;p++)
    {
      if (!glIsTexture(fields[f][p].id))
      {
        glGenTextures(1, &fields[f][p].id);
        VerifyGLState();
      }
    }
  }

  // YUV
  for (int f = FIELD_FULL; f<=FIELD_EVEN ; f++)
  {
    int fieldshift = (f==FIELD_FULL) ? 0 : 1;
    YUVPLANES &planes = fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> fieldshift;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;

    if(m_renderMethod & RENDER_POT)
    {
      for(int p = 0; p < MAX_PLANES-1; p++)
      {
        planes[p].texwidth  = NP2(planes[p].texwidth);
        planes[p].texheight = NP2(planes[p].texheight);
      }
    }

    for(int p = 0; p < MAX_PLANES-1; p++)
    {
      YUVPLANE &plane = planes[p];
      if (plane.texwidth * plane.texheight == 0)
        continue;

      glBindTexture(m_textureTarget, plane.id);

      if(m_renderMethod & RENDER_POT)
        CLog::Log(LOGNOTICE, "GL: Creating YUV POT texture of size %d x %d",  plane.texwidth, plane.texheight);
      else
        CLog::Log(LOGDEBUG,  "GL: Creating YUV NPOT texture of size %d x %d", plane.texwidth, plane.texheight);

      if (p == 1)
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE_ALPHA, plane.texwidth, plane.texheight, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
      else
        glTexImage2D(m_textureTarget, 0, GL_LUMINANCE, plane.texwidth, plane.texheight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      VerifyGLState();
    }
  }
  glDisable(m_textureTarget);
  SetEvent(m_eventTexturesDone[index]);
#endif
  return(true);
}
bool CLinuxRendererGL::DeleteCrystalHDTexture(int index)
{
#ifdef HAVE_LIBCRYSTALHD
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  if( fields[FIELD_FULL][0].id == 0 ) return(true);

  CLog::Log(LOGDEBUG, "Deleted YV12 texture %i", index);
  
  // finish up all textures, and delete them
  g_graphicsContext.BeginPaint();  //FIXME
  for(int f = 0;f<MAX_FIELDS;f++)
  {
    for(int p = 0;p<MAX_PLANES-1;p++)
    {
      if( fields[f][p].id )
      {
        if (glIsTexture(fields[f][p].id))
        {
          glDeleteTextures(1, &fields[f][p].id);
          //CLog::Log(LOGDEBUG, "GL: Deleting texture field %d plane %d", f+1, p+1);
        }
        fields[f][p].id = 0;
      }
    }
  }
  g_graphicsContext.EndPaint();

  for(int p = 0;p<MAX_PLANES-1;p++)
  {
    im.plane[p] = NULL;
  }
#endif

  return(true);
}

void CLinuxRendererGL::SetTextureFilter(GLenum method)
{
  for (int i = 0 ; i<m_NumYV12Buffers ; i++)
  {
    YUVFIELDS &fields = m_buffers[i].fields;

    for (int f = FIELD_FULL; f<=FIELD_EVEN ; f++)
    {
      glBindTexture(m_textureTarget, fields[f][0].id);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
      VerifyGLState();

      if (!(m_renderMethod & RENDER_SW))
      {
        glBindTexture(m_textureTarget, fields[f][1].id);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
        VerifyGLState();

        glBindTexture(m_textureTarget, fields[f][2].id);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, method);
        glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, method);
        VerifyGLState();
      }
    }
  }
}

bool CLinuxRendererGL::SupportsBrightness()
{
#ifdef HAVE_LIBVDPAU
  if(g_VDPAU && !g_guiSettings.GetBool("videoplayer.vdpaustudiolevel"))
    return true;
#endif
  return m_renderMethod == RENDER_GLSL
      || m_renderMethod == RENDER_ARB
      || (m_renderMethod == RENDER_SW && glewIsSupported("GL_ARB_imaging") == GL_TRUE);
}

bool CLinuxRendererGL::SupportsContrast()
{
#ifdef HAVE_LIBVDPAU
  if(g_VDPAU && !g_guiSettings.GetBool("videoplayer.vdpaustudiolevel"))
    return true;
#endif
  return m_renderMethod == RENDER_GLSL
      || m_renderMethod == RENDER_ARB
      || (m_renderMethod == RENDER_SW && glewIsSupported("GL_ARB_imaging") == GL_TRUE);
}

bool CLinuxRendererGL::SupportsGamma()
{
  return false;
}

bool CLinuxRendererGL::SupportsMultiPassRendering()
{
  return glewIsSupported("GL_EXT_framebuffer_object") && glCreateProgram;
}

bool CLinuxRendererGL::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if(m_renderMethod == RENDER_METHOD_VDPAU)
  {
    if(method == VS_INTERLACEMETHOD_VDPAU
    || method == VS_INTERLACEMETHOD_RENDER_BLEND
    || method == VS_INTERLACEMETHOD_INVERSE_TELECINE)
      return true;
    return false;
  }

  if(method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  if(method == VS_INTERLACEMETHOD_RENDER_BLEND
  || method == VS_INTERLACEMETHOD_RENDER_WEAVE_INVERTED
  || method == VS_INTERLACEMETHOD_RENDER_WEAVE
  || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED
  || method == VS_INTERLACEMETHOD_RENDER_BOB)
    return true;

  return false;
}

bool CLinuxRendererGL::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST
  || method == VS_SCALINGMETHOD_LINEAR)
    return true;


  if(method == VS_SCALINGMETHOD_CUBIC 
  && glewIsSupported("GL_ARB_texture_float")
  && glewIsSupported("GL_EXT_framebuffer_object")
  && m_renderMethod == RENDER_GLSL)
    return true;

#if 0
  if(method == VS_SCALINGMETHOD_BICUBIC_SOFTWARE
  || method == VS_SCALINGMETHOD_LANCZOS_SOFTWARE
  || method == VS_SCALINGMETHOD_SINC_SOFTWARE)
    return true;
#endif

  if(method == VS_SCALINGMETHOD_VDPAU_HARDWARE && m_renderMethod == RENDER_METHOD_VDPAU)
    return true;

  return false;
}

#endif
