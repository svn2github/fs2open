#include "ddsutils/ddsutils.h"
#include "cfile/cfile.h"



int Texture_compression_enabled=0;


int dds_read_header(char *filename, CFILE *img_cfp, int *width, int *height, int *bpp, int *compression_type, int *levels, int *size)
{
	DDSURFACEDESC2 dds_header;
	int code = 0;
	CFILE *ddsfile;
	char real_name[MAX_FILENAME_LEN];
	int retval = DDS_ERROR_NONE;
	int ct = DDS_UNCOMPRESSED;
	int bits = 0;
	int trash;

	if (img_cfp == NULL) {
		// this better not happen.. ever
		Assert(filename != NULL);

		// make sure there is an extension
		strcpy(real_name, filename);
		char *p = strchr(real_name, '.');
		if (p) { *p=0; }
		strcat(real_name, ".dds");

		// try to open the file
		ddsfile = cfopen(real_name, "rb");

		// file not found
		if (ddsfile == NULL)
			return DDS_ERROR_INVALID_FILENAME;
	} else {
		ddsfile = img_cfp;
	}

	// read the code
	code = cfread_int(ddsfile);

	// check it
	if (code != DDS_FILECODE)
		return DDS_ERROR_BAD_HEADER;

	// read header variables
	dds_header.dwSize				= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwSize );
	dds_header.dwFlags				= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwFlags );
	dds_header.dwHeight				= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwHeight );
	dds_header.dwWidth				= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwWidth );
	dds_header.dwPitchOrLinearSize	= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwPitchOrLinearSize );
	dds_header.dwDepth				= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwDepth);
	dds_header.dwMipMapCount		= cfread_uint(ddsfile); //INTEL_INT( dds_header.dwMipMapCount );

	// skip over the crap we don't care about
	for (int i = 0; i < 11; i++)
		trash = cfread_uint(ddsfile);

	dds_header.ddpfPixelFormat.dwSize				= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwSize );
	dds_header.ddpfPixelFormat.dwFlags				= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwFlags );
	dds_header.ddpfPixelFormat.dwFourCC				= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwFourCC );
	dds_header.ddpfPixelFormat.dwRGBBitCount		= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwRGBBitCount );
	dds_header.ddpfPixelFormat.dwRBitMask			= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwRBitMask );
	dds_header.ddpfPixelFormat.dwGBitMask			= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwGBitMask );
	dds_header.ddpfPixelFormat.dwBBitMask			= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwBBitMask );
	dds_header.ddpfPixelFormat.dwRGBAlphaBitMask	= cfread_uint(ddsfile); //INTEL_INT( dds_header.ddpfPixelFormat.dwRGBAlphaBitMask );

	// calculate the type and size of the data
	if (dds_header.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
		// did I mention lately that I hate Microsoft?
		// this is here to fix the situation where Microsoft doesn't follow their own docs
		if ( dds_header.dwPitchOrLinearSize <= 0 ) {
			if (dds_header.dwDepth == 0)
				dds_header.dwDepth = 1;

			// calculate the block size, mult by 8 for DXT1, 16 for DXT3 & 5
			*size = ((dds_header.dwWidth + 3)/4) * ((dds_header.dwHeight + 3)/4) * ((dds_header.dwDepth + 3)/4) * ((dds_header.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 8 : 16);
		} else {
			*size = dds_header.dwMipMapCount > 1 ? dds_header.dwPitchOrLinearSize * 2 : dds_header.dwPitchOrLinearSize;
		}

		switch (dds_header.ddpfPixelFormat.dwFourCC) {
			case FOURCC_DXT1:
				bits=24;
				ct=DDS_DXT1;
				break;

			case FOURCC_DXT3:
				bits=32;
				ct=DDS_DXT3;
				break;

			case FOURCC_DXT5:
				bits=32;
				ct=DDS_DXT5;
				break;

			// dxt2 and dxt4 aren't supported
			case FOURCC_DXT2:
			case FOURCC_DXT4:
				retval = DDS_ERROR_INVALID_FORMAT;
				ct = DDS_DXT_INVALID;
				break;

			// none of the above
			default:
				retval = DDS_ERROR_UNSUPPORTED;
				ct = DDS_DXT_INVALID;
				break;
		}
	} else if (dds_header.ddpfPixelFormat.dwFlags & DDPF_RGB) {
		if (dds_header.dwDepth == 0)
			dds_header.dwDepth = 1;

		*size = dds_header.dwWidth * dds_header.dwHeight * dds_header.dwDepth * (dds_header.ddpfPixelFormat.dwRGBBitCount / 8);

		bits = dds_header.ddpfPixelFormat.dwRGBBitCount;
		ct = DDS_UNCOMPRESSED;
	} else {
		// it's not a readable format
		retval = DDS_ERROR_UNSUPPORTED;
	}

	// stuff important info
	if (bpp)
		*bpp = bits;

	if (compression_type)
		*compression_type = ct;

	if (width)
		*width = dds_header.dwWidth;

	if (height)
		*height = dds_header.dwHeight;

	if (levels)
		*levels = dds_header.dwMipMapCount;

	if (img_cfp == NULL) {
		// close file and return
		cfclose(ddsfile);
		ddsfile = NULL;
	}

	return retval;
}

//reads pixel info from a dds file
int dds_read_bitmap(char *filename, ubyte *data, ubyte *bpp)
{
	int retval;
	int w,h,ct,lvl;
	int size = 0, bits = 0;
	CFILE *cfp;
	char real_name[MAX_FILENAME_LEN];

	// this better not happen.. ever
	Assert(filename != NULL);

	// make sure there is an extension
	strcpy(real_name, filename);
	char *p = strchr(real_name, '.');
	if (p) { *p = 0; }
	strcat(real_name, ".dds");

	// open it up and go to the data section
	cfp = cfopen(real_name, "rb");

	// just in case
	if (cfp == NULL)
		return DDS_ERROR_INVALID_FILENAME;

	// read the header -- if its at this stage, it should be legal.
	retval = dds_read_header(real_name, cfp, &w, &h, &bits, &ct, &lvl, &size);
	Assert(retval == DDS_ERROR_NONE);

	// this really shouldn't be needed but better safe than sorry
	if (retval != DDS_ERROR_NONE) {
		return retval;
	}

	cfseek(cfp, DDS_OFFSET, CF_SEEK_SET);

	// read in the data
	cfread(data, 1, size, cfp);

	if (bpp)
		*bpp = (ubyte)bits;

	// we look done here
	cfclose(cfp);

	return DDS_ERROR_NONE;
}

//returns string representation of DDS_** error code
const char *dds_error_string(int code)
{
	switch (code)
	{
		case DDS_ERROR_NONE:
			return "No error";

		case DDS_ERROR_NO_MEM:
			return "Insufficient memory";

		case DDS_ERROR_INVALID_FILENAME:
			return "File not found";

		case DDS_ERROR_BAD_HEADER:
			return "Filecode did not equal \"DDS \"";

		case DDS_ERROR_INVALID_FORMAT:
			return "File was compressed, but it was DXT2 or DXT4";
			
		case DDS_ERROR_UNSUPPORTED:
			return "*.DDS files must be uncompressed or use DXT1, DXT3, or DXT5 compression";

		default:
			return "Abort, retry, fail?";
	}

	//get a warning otherwise
	return "Abort, retry, fail?";
}
