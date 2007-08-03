// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
#pragma code_seg( "CXIMAGE" )
#pragma data_seg( "CXIMAGE_RW" )
#pragma bss_seg( "CXIMAGE_RW" )
#pragma const_seg( "CXIMAGE_RD" )
#pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
#pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
/*
 *  Probability estimation tables for the arithmetic encoder/decoder
 *  given by ITU T.82 Table 24.
 *
 *  $Id: jbig_tab.c,v 1.6 1998-04-05 18:36:19+01 mgk25 Rel $
 */

short jbg_lsz[113] = {
  0x5a1d, 0x2586, 0x1114, 0x080b, 0x03d8, 0x01da, 0x00e5, 0x006f,
  0x0036, 0x001a, 0x000d, 0x0006, 0x0003, 0x0001, 0x5a7f, 0x3f25,
  0x2cf2, 0x207c, 0x17b9, 0x1182, 0x0cef, 0x09a1, 0x072f, 0x055c,
  0x0406, 0x0303, 0x0240, 0x01b1, 0x0144, 0x00f5, 0x00b7, 0x008a,
  0x0068, 0x004e, 0x003b, 0x002c, 0x5ae1, 0x484c, 0x3a0d, 0x2ef1,
  0x261f, 0x1f33, 0x19a8, 0x1518, 0x1177, 0x0e74, 0x0bfb, 0x09f8,
  0x0861, 0x0706, 0x05cd, 0x04de, 0x040f, 0x0363, 0x02d4, 0x025c,
  0x01f8, 0x01a4, 0x0160, 0x0125, 0x00f6, 0x00cb, 0x00ab, 0x008f,
  0x5b12, 0x4d04, 0x412c, 0x37d8, 0x2fe8, 0x293c, 0x2379, 0x1edf,
  0x1aa9, 0x174e, 0x1424, 0x119c, 0x0f6b, 0x0d51, 0x0bb6, 0x0a40,
  0x5832, 0x4d1c, 0x438e, 0x3bdd, 0x34ee, 0x2eae, 0x299a, 0x2516,
  0x5570, 0x4ca9, 0x44d9, 0x3e22, 0x3824, 0x32b4, 0x2e17, 0x56a8,
  0x4f46, 0x47e5, 0x41cf, 0x3c3d, 0x375e, 0x5231, 0x4c0f, 0x4639,
  0x415e, 0x5627, 0x50e7, 0x4b85, 0x5597, 0x504f, 0x5a10, 0x5522,
  0x59eb
};

unsigned char jbg_nmps[113] = {
    1,   2,   3,   4,   5,   6,   7,   8,
    9,  10,  11,  12,  13,  13,  15,  16,
   17,  18,  19,  20,  21,  22,  23,  24,
   25,  26,  27,  28,  29,  30,  31,  32,
   33,  34,  35,   9,  37,  38,  39,  40,
   41,  42,  43,  44,  45,  46,  47,  48,
   49,  50,  51,  52,  53,  54,  55,  56,
   57,  58,  59,  60,  61,  62,  63,  32,
   65,  66,  67,  68,  69,  70,  71,  72,
   73,  74,  75,  76,  77,  78,  79,  48,
   81,  82,  83,  84,  85,  86,  87,  71,
   89,  90,  91,  92,  93,  94,  86,  96,
   97,  98,  99, 100,  93, 102, 103, 104,
   99, 106, 107, 103, 109, 107, 111, 109,
  111
};

/*
 * least significant 7 bits (mask 0x7f) of jbg_nlps[] contain NLPS value,
 * most significant bit (mask 0x80) contains SWTCH bit
 */
unsigned char jbg_nlps[113] = {
  129,  14,  16,  18,  20,  23,  25,  28,
   30,  33,  35,   9,  10,  12, 143,  36,
   38,  39,  40,  42,  43,  45,  46,  48,
   49,  51,  52,  54,  56,  57,  59,  60,
   62,  63,  32,  33, 165,  64,  65,  67,
   68,  69,  70,  72,  73,  74,  75,  77,
   78,  79,  48,  50,  50,  51,  52,  53,
   54,  55,  56,  57,  58,  59,  61,  61,
  193,  80,  81,  82,  83,  84,  86,  87,
   87,  72,  72,  74,  74,  75,  77,  77,
  208,  88,  89,  90,  91,  92,  93,  86,
  216,  95,  96,  97,  99,  99,  93, 223,
  101, 102, 103, 104,  99, 105, 106, 107,
  103, 233, 108, 109, 110, 111, 238, 112,
  240
};

/*
 * Resolution reduction table given by ITU-T T.82 Table 17
 */

char jbg_resred[4096] = {
  0,0,0,1,0,0,0,1,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
  0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,1,0,1,0,0,1,1,1,0,1,1,
  0,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,1,1,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,1,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,
  1,0,0,1,0,0,1,1,0,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,0,0,1,0,0,0,1,0,0,1,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,0,1,1,0,1,1,1,1,1,1,0,1,1,1,0,
  0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,1,0,0,1,0,0,1,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,1,0,0,0,1,0,1,1,0,1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,
  1,1,1,0,1,0,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,
  1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,1,1,1,1,1,1,
  0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,1,1,0,1,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,0,1,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,1,1,1,1,1,1,0,1,1,1,1,0,1,1,
  1,0,0,1,0,0,1,0,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,
  0,0,1,0,1,1,1,1,0,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,
  0,0,0,0,1,0,0,1,0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,1,1,0,1,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,1,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,
  0,0,1,0,0,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,
  0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,1,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,
  0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,1,0,1,0,1,1,0,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0,0,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,
  0,0,1,0,0,1,1,1,0,0,0,0,1,0,0,1,0,0,0,1,1,1,1,0,1,0,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,1,0,
  0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,1,0,1,1,1,
  0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,1,1,0,1,0,0,0,1,1,0,1,0,0,0,0,1,1,1,1,0,0,1,1,1,0,1,1,0,0,1,1,
  0,0,0,0,0,0,0,0,1,1,0,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,0,0,1,1,0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,1,0,0,0,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,1,0,0,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,0,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,
  0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
  0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,1,1,0,1,1,
  0,0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,1,0,1,1,0,1,1,1,0,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,0,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,1,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,1,0,1,1,
  1,0,1,0,1,0,0,1,1,0,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,0,1,1,0,1,1,1,
  0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1,1,0,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,1,0,1,0,1,0,1,1,0,1,0,1,0,0,0,1,1,1,1,1,1,1,1,1,
  1,1,1,0,1,0,0,0,1,1,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,
  1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,1,1,0,1,1,
  0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,1,0,0,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,1,0,1,0,0,0,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,
  1,0,0,0,1,0,0,0,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,1,1,1,0,1,1,0,
  0,0,1,1,1,1,1,1,0,0,0,0,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,1,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,
  0,0,1,0,1,0,1,1,0,0,1,0,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,
  0,0,1,0,1,0,1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,
  0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,
  0,0,0,0,0,0,0,1,0,1,1,1,0,1,0,1,0,0,1,0,0,0,0,1,0,1,0,1,0,1,0,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,
  0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,0,0,0,0,0,0,0,1,
  0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,
  1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,
  0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,
  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,
  0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,1,1
};

/*
 * Deterministic prediction tables given by ITU-T T.82 tables
 * 19 to 22. The table below is organized differently, the
 * index bits are permutated for higher efficiency.
 */

char jbg_dptable[256 + 512 + 2048 + 4096] = {
  /* phase 0: offset=0 */
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,0,2,2,2,2,2,2,2,
  0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,0,2,0,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  /* phase 1: offset=256 */
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,2,2,2,0,2,0,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,1,2,1,2,2,2,2,1,1,1,1,2,0,2,0,2,2,2,2,0,2,0,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,2,2,2,2,0,2,2,2,2,2,2,2,
  0,2,0,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,0,0,2,2,2,2,2,0,0,2,2,2,2,2,
  0,2,2,2,2,1,2,1,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,
  1,2,1,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,1,1,2,2,2,2,2,0,2,2,2,2,2,2,
  2,2,2,2,2,0,2,0,2,2,2,2,0,0,0,0,0,2,0,2,2,2,2,2,0,2,2,2,2,2,2,2,
  0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,2,0,2,0,2,2,2,2,2,
  2,2,2,2,2,1,1,1,2,2,2,2,1,1,1,1,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,0,2,0,2,2,2,2,2,0,2,0,2,2,2,2,1,
  0,2,0,2,2,1,2,1,2,2,2,2,1,1,1,1,0,0,0,0,2,2,2,2,0,2,0,2,2,2,2,1,
  2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,0,0,0,2,2,2,2,2,
  2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,1,2,1,2,2,2,2,1,
  2,2,2,2,2,2,2,2,0,2,0,2,2,1,2,2,2,2,2,2,2,2,2,2,0,0,0,2,2,2,2,2,
  /* phase 2: offset=768 */
  2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,
  0,2,2,2,2,1,2,1,2,2,2,2,1,2,1,2,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,1,2,1,2,2,2,2,2,1,1,1,
  2,0,2,2,2,1,2,1,0,2,2,2,1,2,1,2,2,2,2,0,2,2,2,2,0,2,0,2,2,2,2,2,
  0,2,0,0,1,1,1,1,2,2,2,2,1,1,1,1,0,2,0,2,1,1,1,1,2,2,2,2,1,1,1,1,
  2,2,0,2,2,2,1,2,2,2,2,2,1,2,1,2,2,2,0,2,2,1,2,1,0,2,0,2,1,1,1,1,
  2,0,0,2,2,2,2,2,0,2,0,2,2,0,2,0,2,0,2,0,2,2,2,1,2,2,0,2,1,1,2,1,
  2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,
  0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,0,2,2,2,2,1,0,2,2,2,1,1,1,1,2,0,2,2,2,2,2,2,0,2,0,2,2,1,2,1,
  2,0,2,0,2,2,2,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,
  0,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,
  2,2,0,2,2,2,2,2,2,2,2,2,2,2,0,2,2,0,0,2,2,1,2,1,0,2,2,2,1,1,1,1,
  2,2,2,0,2,2,2,2,2,2,0,2,2,0,2,0,2,1,2,2,2,2,2,2,1,2,1,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,1,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,1,1,1,2,2,2,2,1,1,1,1,
  2,2,2,1,2,2,2,2,2,2,1,2,0,0,0,0,2,2,0,2,2,1,2,2,2,2,2,2,1,1,1,1,
  2,0,0,0,2,2,2,2,0,2,2,2,2,2,2,0,2,2,2,0,2,2,2,2,2,0,0,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,0,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,1,
  0,2,0,2,2,1,1,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,
  2,0,2,0,2,1,2,1,0,2,0,2,2,2,1,2,2,0,2,0,2,2,2,2,0,2,0,2,2,2,1,2,
  2,2,2,0,2,2,2,2,2,2,0,2,2,2,2,2,2,2,1,2,2,2,2,2,2,0,1,2,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  0,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,
  2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,2,1,2,1,0,2,2,2,1,1,1,1,
  2,0,2,0,2,1,2,2,0,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,1,2,2,
  2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,0,2,2,2,2,0,0,0,0,2,1,2,1,
  2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,0,0,2,2,2,1,2,2,2,
  0,0,2,0,2,2,2,2,0,2,0,2,2,0,2,0,1,1,1,2,2,2,2,2,2,2,2,2,2,1,1,1,
  2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,1,
  2,2,0,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2,2,2,2,1,1,1,1,
  0,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,
  2,0,0,2,2,2,2,2,0,2,0,2,2,2,2,2,1,0,1,2,2,2,2,1,0,2,2,2,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,0,2,2,0,2,0,2,1,2,2,2,2,2,2,2,2,0,2,2,1,2,2,
  0,2,0,0,1,1,1,1,0,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,0,2,2,1,2,1,1,
  2,2,0,2,2,1,2,2,2,2,2,2,1,2,2,2,2,0,2,2,2,2,2,2,0,2,0,2,1,2,1,1,
  2,0,2,0,2,2,2,2,0,2,0,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,1,
  2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,2,0,2,0,2,2,2,2,0,0,0,0,2,2,2,2,2,1,1,2,2,2,2,2,1,2,2,2,
  2,0,2,2,2,1,2,1,0,2,2,2,2,2,1,2,2,0,2,0,2,2,2,2,0,2,0,2,2,1,2,2,
  0,2,0,0,2,2,2,2,1,2,2,2,2,2,2,0,2,1,2,2,2,2,2,2,1,2,2,2,2,2,2,2,
  0,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,1,0,2,2,
  0,0,0,2,2,1,1,1,2,2,2,2,1,2,2,2,2,0,2,0,2,2,2,1,2,2,2,2,1,2,1,2,
  0,0,0,0,2,2,2,2,2,2,0,2,2,1,2,2,2,1,2,1,2,2,2,2,1,2,1,2,0,2,2,2,
  2,0,2,0,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  0,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,1,2,2,2,2,2,0,2,2,1,2,2,0,0,0,2,2,2,2,2,1,2,2,0,2,2,2,1,2,1,2,
  2,0,2,0,2,2,2,2,0,2,0,2,2,1,2,2,0,2,0,0,2,2,2,2,2,2,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,0,2,0,2,2,2,1,
  1,2,0,2,2,1,2,1,2,2,2,2,1,2,2,2,2,0,2,0,2,2,2,2,2,0,2,2,1,1,1,1,
  0,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,2,1,2,1,
  2,2,0,0,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,
  2,2,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,2,
  2,0,2,0,2,2,2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,2,0,2,2,2,1,2,
  2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,0,2,0,2,2,2,2,2,0,2,0,2,2,2,2,2,0,2,0,2,2,2,2,0,0,0,0,2,1,2,1,
  2,2,2,2,2,1,2,1,0,2,0,2,2,2,2,2,2,0,2,0,2,2,2,2,0,2,0,2,2,2,2,1,
  2,0,2,0,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,0,
  2,0,2,0,2,2,2,1,2,2,2,0,2,2,2,1,2,0,2,0,2,2,2,2,0,0,0,2,2,2,2,1,
  2,0,2,0,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,2,
  /* phase 3: offset=2816 */
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,1,2,1,2,0,2,0,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,0,2,2,2,1,2,0,2,2,2,1,2,2,2,2,0,2,0,2,1,2,1,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,
  2,2,2,1,2,2,2,0,1,1,1,1,0,0,0,0,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,2,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,2,0,2,0,2,1,2,1,
  2,0,0,0,2,1,1,1,0,0,0,0,1,1,1,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,
  2,0,2,2,2,1,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,0,0,2,0,1,1,2,1,
  2,2,2,0,2,2,2,1,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
  0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,2,0,2,0,2,1,2,1,0,0,0,0,1,1,1,1,
  2,0,0,2,2,1,1,2,2,2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,2,1,1,1,2,0,0,0,
  2,1,2,1,2,0,2,0,1,2,1,2,0,2,0,2,2,2,2,0,2,2,2,1,2,0,2,0,2,1,2,1,
  2,0,2,0,2,1,2,1,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,
  2,2,2,2,2,2,2,2,2,0,0,0,2,1,1,1,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,
  2,0,0,0,2,1,1,1,0,0,0,0,1,1,1,1,2,0,2,0,2,1,2,1,0,0,2,0,1,1,2,1,
  2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,0,0,0,2,1,1,1,
  2,2,2,1,2,2,2,0,2,1,1,1,2,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,1,2,1,2,0,2,0,2,
  2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,2,1,1,1,0,0,0,0,1,1,1,1,
  2,0,2,2,2,1,2,2,0,0,2,0,1,1,2,1,2,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,0,0,0,0,1,1,1,1,
  2,0,0,0,2,1,1,1,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,1,0,2,2,0,1,2,
  2,2,2,1,2,2,2,0,2,1,1,1,2,0,0,0,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,
  2,1,2,1,2,0,2,0,1,2,1,1,0,2,0,0,0,0,2,1,1,1,2,0,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,0,2,1,2,1,2,0,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,0,2,2,2,1,2,2,2,0,0,2,2,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,
  2,0,2,0,2,1,2,1,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,0,0,0,2,1,1,1,
  2,2,2,0,2,2,2,1,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,0,2,2,2,1,2,2,2,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,1,2,1,2,0,2,0,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,1,2,1,2,0,2,0,1,2,1,1,0,2,0,0,2,0,2,2,2,1,2,2,0,2,1,2,1,2,0,2,
  2,2,2,1,2,2,2,0,2,2,1,2,2,2,0,2,2,1,2,2,2,0,2,2,2,2,0,2,2,2,1,2,
  0,0,2,0,1,1,2,1,0,0,1,0,1,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,0,2,2,2,1,1,2,2,2,0,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,0,0,2,2,1,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,
  2,0,0,0,2,1,1,1,0,0,0,0,1,1,1,1,2,2,2,1,2,2,2,0,2,1,2,1,2,0,2,0,
  2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,0,2,0,0,1,2,1,1,2,0,0,0,2,1,1,1,
  2,2,2,2,2,2,2,2,2,1,1,1,2,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,1,2,1,2,0,2,0,2,0,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,1,1,1,2,0,0,0,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,0,0,1,2,1,1,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,2,
  2,1,2,1,2,0,2,0,2,1,2,2,2,0,2,2,2,2,2,0,2,2,2,1,2,0,2,0,2,1,2,1,
  2,0,2,0,2,1,2,1,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,0,1,0,0,1,0,1,1,
  2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,2,2,1,2,2,2,0,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,1,2,2,1,0,2,0,2,2,2,1,2,2,2,
  2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,
  2,0,2,0,2,1,2,1,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  0,2,0,0,1,2,1,1,2,0,0,0,2,1,1,1,2,2,2,2,2,2,2,2,1,0,1,2,0,1,0,2,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,1,2,2,2,0,2,2,1,1,2,2,0,0,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,1,2,1,2,0,2,0,2,1,2,2,2,0,2,2,2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,1,2,2,2,0,2,2,2,
  2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,2,
  0,0,0,0,1,1,1,1,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,2,2,0,2,2,2,1,2,
  2,0,2,0,2,1,2,1,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,
  0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,1,2,2,2,0,1,1,2,1,0,0,2,0,2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,2,2,0,2,2,2,1,2,
  2,0,2,0,2,1,2,1,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,2,1,2,2,2,0,2,2,
  0,2,0,0,1,2,1,1,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,0,0,0,2,1,1,1,2,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,0,0,2,1,1,1,2,0,0,2,2,2,1,2,2,2,
  2,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,2,0,0,1,2,1,1,
  0,0,2,2,1,1,2,2,0,2,1,2,1,2,0,2,2,1,2,1,2,0,2,0,1,2,1,2,0,2,0,2,
  2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,
  2,2,0,0,2,2,1,1,2,2,0,0,2,2,1,1,2,2,2,2,2,2,2,2,2,2,0,0,2,2,1,1,
  2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,0,2,0,0,1,2,1,1,
  2,2,2,0,2,2,2,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,1,1,1,2,0,0,0,2,
  2,2,2,2,2,2,2,2,1,1,1,2,0,0,0,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,
  2,0,2,0,2,1,2,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,0,0,0,2,1,1,1,
  2,0,2,2,2,1,2,2,0,2,2,2,1,2,2,2,2,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,
  2,0,2,0,2,1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,0,2,0,2,1,2,1,2,1,2,0,2,0,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,0,2,0,2,1,2,1,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,1,2,1,2,0,2,0,2,2,1,2,1,2,0,2,0,2,2,2,2,2,2,2,2,
  2,0,2,1,2,1,2,0,0,2,1,2,1,2,0,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,0,2,0,2,1,2,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,0,2,0,2,1,2,1,
  2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,1,2,1,2,0,2,0,1,1,1,2,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,
  2,0,2,0,2,1,2,1,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  2,2,2,2,2,2,2,2,2,2,2,0,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
};
