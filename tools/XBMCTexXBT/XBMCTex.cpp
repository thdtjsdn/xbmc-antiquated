/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <sys/types.h>
#include <dirent.h>
#include <squish.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "XBTF.h"
#include "XBTFWriter.h"
#include "SDL_anigif.h"
#include "cmdlineargs.h"
#ifdef _WIN32
#define strncasecmp strnicmp
#endif

#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'

#define FLAGS_USE_LZO     1
#define FLAGS_ALLOW_YCOCG 2

#undef main

const char *GetFormatString(unsigned int format)
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return "DXT1 ";
  case XB_FMT_DXT3:
    return "DXT3 ";
  case XB_FMT_DXT5:
    return "DXT5 ";
  case XB_FMT_DXT5_YCoCg:
    return "YCoCg";
  case XB_FMT_A8R8G8B8:
    return "ARGB ";
  case XB_FMT_A8:
    return "A8   ";
  default:
    return "?????";
  }
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGraphicsFile(char *strFileName)
{
  size_t n = strlen(strFileName);
  if (n < 4)
    return false;

  if (strncasecmp(&strFileName[n-4], ".png", 4) &&
      strncasecmp(&strFileName[n-4], ".bmp", 4) &&
      strncasecmp(&strFileName[n-4], ".tga", 4) &&
      strncasecmp(&strFileName[n-4], ".gif", 4) &&
      strncasecmp(&strFileName[n-4], ".tbn", 4) &&
      strncasecmp(&strFileName[n-4], ".jpg", 4))
    return false;

  return true;
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGIF(const char *strFileName)
{
  size_t n = strlen(strFileName);
  if (n < 4)
    return false;

  if (strncasecmp(&strFileName[n-4], ".gif", 4))
    return false;

  return true;
}

void CreateSkeletonHeaderImpl(CXBTF& xbtf, std::string fullPath, std::string relativePath)
{
  struct dirent* dp;
  DIR *dirp = opendir(fullPath.c_str());

  while ((dp = readdir(dirp)) != NULL)
  {
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) 
    {
      continue;
    }

    if (dp->d_type == DT_DIR)
    {
      std::string tmpPath = relativePath;
      if (tmpPath.size() > 0)
      {
        tmpPath += "/";
      }

      CreateSkeletonHeaderImpl(xbtf, fullPath + DIR_SEPARATOR + dp->d_name, tmpPath + dp->d_name);
    }
    else if (IsGraphicsFile(dp->d_name))
    {
      std::string fileName = "";
      if (relativePath.size() > 0)
      {
        fileName += relativePath;
        fileName += "/";
      }

      fileName += dp->d_name;

      CXBTFFile file;
      file.SetPath(fileName);
      xbtf.GetFiles().push_back(file);
    }
  }

  closedir(dirp);
}

void CreateSkeletonHeader(CXBTF& xbtf, std::string fullPath)
{
  std::string temp;
  CreateSkeletonHeaderImpl(xbtf, fullPath, temp);
}

CXBTFFrame appendContent(CXBTFWriter &writer, int width, int height, unsigned char const *data, unsigned int size, unsigned int format, unsigned int flags)
{
  CXBTFFrame frame;
  unsigned int compressedSize = size;
  if ((flags & FLAGS_USE_LZO) == FLAGS_USE_LZO)
  { // TODO: add lzo'ing
  }
  else
  {
    writer.AppendContent(data, size);
  }
  frame.SetPackedSize(compressedSize);
  frame.SetUnpackedSize(size);
  frame.SetWidth(width);
  frame.SetHeight(height);
  frame.SetFormat(format);
  frame.SetDuration(0);
  return frame;
}

void CompressImage(const squish::u8 *brga, int width, int height, squish::u8 *compressed, unsigned int flags, double &colorMSE, double &alphaMSE)
{
  squish::CompressImage(brga, width, height, compressed, flags | squish::kSourceBGRA);
  squish::ComputeMSE(brga, width, height, compressed, flags | squish::kSourceBGRA, colorMSE, alphaMSE);
}

CXBTFFrame createXBTFFrame(SDL_Surface* image, CXBTFWriter& writer, double maxMSE, unsigned int flags)
{
  // Convert to ARGB
  SDL_PixelFormat argbFormat;
  memset(&argbFormat, 0, sizeof(SDL_PixelFormat));
  argbFormat.BitsPerPixel = 32;
  argbFormat.BytesPerPixel = 4;

  // For DXT5 we need RGBA
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  argbFormat.Amask = 0xff000000;
  argbFormat.Ashift = 24;
  argbFormat.Rmask = 0x00ff0000;
  argbFormat.Rshift = 16;
  argbFormat.Gmask = 0x0000ff00;
  argbFormat.Gshift = 8;
  argbFormat.Bmask = 0x000000ff;
  argbFormat.Bshift = 0;
#else
  argbFormat.Amask = 0x000000ff;
  argbFormat.Ashift = 0;
  argbFormat.Rmask = 0x0000ff00;
  argbFormat.Rshift = 8;
  argbFormat.Gmask = 0x00ff0000;
  argbFormat.Gshift = 16;
  argbFormat.Bmask = 0xff000000;
  argbFormat.Bshift = 24;
#endif

  SDL_Surface *argbImage = SDL_ConvertSurface(image, &argbFormat, 0);

  unsigned int format = 0;
  double colorMSE, alphaMSE;
  squish::u8* argb = (squish::u8 *)argbImage->pixels;
  unsigned int compressedSize = squish::GetStorageRequirements(image->w, image->h, squish::kDxt5);
  squish::u8* compressed = new squish::u8[compressedSize];
  // first try DXT1, which is only 4bits/pixel
  CompressImage(argb, image->w, image->h, compressed, squish::kDxt1, colorMSE, alphaMSE);
  if (colorMSE < maxMSE && alphaMSE < maxMSE)
  { // success - use it
    compressedSize = squish::GetStorageRequirements(image->w, image->h, squish::kDxt1);
    format = XB_FMT_DXT1;
  }
  if (!format && alphaMSE == 0 && (flags & FLAGS_ALLOW_YCOCG) == FLAGS_ALLOW_YCOCG)
  { // no alpha channel, so DXT5YCoCg is going to be the best DXT5 format
/*    CompressImage(argb, image->w, image->h, compressed, squish::kDxt5 | squish::kUseYCoCg, colorMSE, alphaMSE);
    if (colorMSE < maxMSE && alphaMSE < maxMSE)
    { // success - use it
      compressedSize = squish::GetStorageRequirements(image->w, image->h, squish::kDxt5);
      format = XB_FMT_DXT5_YCoCg;
    }
    */
  }
  if (!format)
  { // try DXT3 and DXT5 - use whichever is better (color is the same, but alpha will be different)
    CompressImage(argb, image->w, image->h, compressed, squish::kDxt3, colorMSE, alphaMSE);
    if (colorMSE < maxMSE)
    { // color is fine, test DXT5 as well
      double dxt5MSE;
      squish::u8* compressed2 = new squish::u8[squish::GetStorageRequirements(image->w, image->h, squish::kDxt5)];
      CompressImage(argb, image->w, image->h, compressed2, squish::kDxt5, colorMSE, dxt5MSE);
      if (alphaMSE < maxMSE && alphaMSE < dxt5MSE)
      { // DXT3 passes and is best
        compressedSize = squish::GetStorageRequirements(image->w, image->h, squish::kDxt3);
        format = XB_FMT_DXT3;
      }
      else if (dxt5MSE < maxMSE)
      { // DXT5 passes
        compressedSize = squish::GetStorageRequirements(image->w, image->h, squish::kDxt5);
        memcpy(compressed, compressed2, compressedSize);
        format = XB_FMT_DXT5;
      }
      delete[] compressed2;
    }
  }
  CXBTFFrame frame; 
  if (!format)
  { // none of the compressed stuff works for us, so we use 32bit texture
    format = XB_FMT_A8R8G8B8;
    frame = appendContent(writer, image->w, image->h, argb, image->w * image->h * 4, format, flags);
  }
  else
  {
    frame = appendContent(writer, image->w, image->h, compressed, compressedSize, format, flags);
  }
  delete[] compressed;
  SDL_FreeSurface(argbImage);
  return frame;
}

void Usage()
{
  puts("Usage:");
  puts("  -help            Show this screen.");
  puts("  -input <dir>     Input directory. Default: current dir");
  puts("  -output <dir>    Output directory/filename. Default: Textures.xpr");
}

int createBundle(const std::string& InputDir, const std::string& OutputFile, double maxMSE, unsigned int flags)
{
  CXBTF xbtf;
  CreateSkeletonHeader(xbtf, InputDir);

  CXBTFWriter writer(xbtf, OutputFile);
  if (!writer.Create())
  {
    printf("Error creating file\n");
    return 1;
  }

  std::vector<CXBTFFile>& files = xbtf.GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    CXBTFFile& file = files[i];

    std::string fullPath = InputDir;
    fullPath += file.GetPath();

    std::string output = file.GetPath();
    output = output.substr(0, 40);
    while (output.size() < 46)
      output += ' ';
    if (!IsGIF(fullPath.c_str()))
    {
      // Load the image
      SDL_Surface* image = IMG_Load(fullPath.c_str());
      if (!image)
      {
        printf("...unable to load image %s\n", file.GetPath());
        continue;
      }

      printf(output.c_str());

      CXBTFFrame frame = createXBTFFrame(image, writer, maxMSE, flags);

      printf("%s (%d,%d @ %d bytes)\n", GetFormatString(frame.GetFormat()), frame.GetWidth(), frame.GetHeight(), frame.GetUnpackedSize());

      file.SetLoop(0);
      file.GetFrames().push_back(frame);

      SDL_FreeSurface(image);
    }
    else
    {
      int gnAG = AG_LoadGIF(fullPath.c_str(), NULL, 0);
      AG_Frame* gpAG = new AG_Frame[gnAG];
      AG_LoadGIF(fullPath.c_str(), gpAG, gnAG);

      printf("%s\n", output.c_str());

      for (int j = 0; j < gnAG; j++)
      {
        printf("    frame %4i                                ", j);
        CXBTFFrame frame = createXBTFFrame(gpAG[j].surface, writer, maxMSE, flags);
        frame.SetDuration(gpAG[j].delay);
        file.GetFrames().push_back(frame);
        printf("%s (%d,%d @ %d bytes)\n", GetFormatString(frame.GetFormat()), frame.GetWidth(), frame.GetHeight(), frame.GetUnpackedSize());
      }

      AG_FreeSurfaces(gpAG, gnAG);
      delete [] gpAG;

      file.SetLoop(0);
    }
  }

  if (!writer.UpdateHeader())
  {
    printf("Error writing header to file\n");
    return 1;
  }

  if (!writer.Close())
  {
    printf("Error closing file\n");
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[])
{
  bool valid = false;
  CmdLineArgs args(argc, (const char**)argv);

  if (args.size() == 1)
  {
    Usage();
    return 1;
  }

  std::string InputDir;
  std::string OutputFilename = "Textures.xbt";

  for (unsigned int i = 1; i < args.size(); ++i)
  {
    if (!stricmp(args[i], "-help") || !stricmp(args[i], "-h") || !stricmp(args[i], "-?"))
    {
      Usage();
      return 1;
    }
    else if (!stricmp(args[i], "-input") || !stricmp(args[i], "-i"))
    {
      InputDir = args[++i];
      valid = true;
    }
    else if (!stricmp(args[i], "-output") || !stricmp(args[i], "-o"))
    {
      OutputFilename = args[++i];
      valid = true;
#ifdef _LINUX
      char *c = NULL;
      while ((c = (char *)strchr(OutputFilename.c_str(), '\\')) != NULL) *c = '/';
#endif
    }
    else
    {
      printf("Unrecognized command line flag: %s\n", args[i]);
    }
  }

  if (!valid)
  {
    Usage();
    return 1;
  }

  size_t pos = InputDir.find_last_of(DIR_SEPARATOR);
  if (pos != InputDir.length() - 1)
    InputDir += DIR_SEPARATOR;

  double maxMSE = 1.5;    // HQ only please
  unsigned int flags = 0; // TODO: currently no YCoCg and no LZO (commandline option?)
  createBundle(InputDir, OutputFilename, maxMSE, flags);
}
