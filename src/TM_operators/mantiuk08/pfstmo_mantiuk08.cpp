/**
 * @brief Display Adaptive TMO
 *
 * From:
 * Rafal Mantiuk, Scott Daly, Louis Kerofsky.
 * Display Adaptive Tone Mapping.
 * To appear in: ACM Transactions on Graphics (Proc. of SIGGRAPH'08) 27 (3)
 * http://www.mpi-inf.mpg.de/resources/hdr/datmo/
 *
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 * 
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: pfstmo_mantiuk08.cpp,v 1.12 2009/02/23 18:46:36 rafm Exp $
 */

#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>

#include "../../Libpfs/pfs.h"
#include "../../Common/progressHelper.h"

#include "display_adaptive_tmo.h"

#define PROG_NAME "pfstmo_mantiuk08"

using namespace std;

void pfstmo_mantiuk08(pfs::Frame* frame, float saturation_factor, float contrast_enhance_factor, float white_y, bool setluminance, ProgressHelper *ph)
{
    //--- default tone mapping parameters;
    //float contrast_enhance_factor = 1.f;
    //float saturation_factor = 1.f;
    //float white_y = -2.f;
    //int temporal_filter = 0;
    int itmax = 200;
    float tol = 1e-3;

    if (!setluminance)
	white_y = -2.f;
    
    if( contrast_enhance_factor <= 0.0f )
      throw pfs::Exception("incorrect contrast enhancement factor, accepted value is any positive number");

    if( saturation_factor < 0.0f || saturation_factor > 2.0f )
      throw pfs::Exception("incorrect saturation factor, accepted range is (0..2)");

    std::cout << "pfstmo_mantiuk08" << std::endl;
    std::cout << "saturation factor: " << saturation_factor << std::endl;
    std::cout << "contrast enhancement factor: " << contrast_enhance_factor << std::endl;
    std::cout << "white_y: " << white_y << std::endl;
    std::cout << "setluminance: " << setluminance  << std::endl;

    DisplayFunction *df = NULL;
    DisplaySize *ds = NULL;

    if( df == NULL )
      df = new DisplayFunctionGGBA( "lcd" );
    
    if( ds == NULL )
      ds = new DisplaySize( 30.f, 0.5f ); 
    
    df->print( stderr );
    ds->print( stderr );

    pfs::DOMIO pfsio;

    pfs::Channel *inX, *inY, *inZ;
    frame->getXYZChannels(inX, inY, inZ);
    int cols = frame->getWidth();
    int rows = frame->getHeight();

    pfs::Array2DImpl R( cols, rows ); 
    pfs::transformColorSpace( pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, inX, &R, inZ );

    if( white_y == -2.f ) {      
      const char *white_y_str = frame->getTags()->getString( "WHITE_Y" );
      if( white_y_str != NULL ) {
        white_y = strtof( white_y_str, NULL );
	if( white_y == 0 ) {
          white_y = -1;
          fprintf( stderr, PROG_NAME ": warning - wrong WHITE_Y in the input image" );        
        }
      }
    }

    fprintf( stderr, "Luminance factor of the reference white: " );
    if( white_y < 0 )
      fprintf( stderr, "not specified\n" );
    else
      fprintf( stderr, "%g\n", white_y );

    const char *lum_data = frame->getTags()->getString("LUMINANCE");
    if( lum_data != NULL && !strcmp( lum_data, "DISPLAY" )) {
      fprintf( stderr, PROG_NAME " warning: input image should be in linear (not gamma corrected) luminance factor units. Use '--linear' option with pfsin* commands.\n" );
    }

    datmoToneCurve tc;
 
    std::auto_ptr<datmoConditionalDensity> C = datmo_compute_conditional_density( cols, rows, inY->getRawData(), ph);
    if( C.get() == NULL )
      throw pfs::Exception("failed to analyse the image");

    int res;
    res = datmo_compute_tone_curve( &tc, C.get(), df, ds, contrast_enhance_factor, white_y, ph);
    if( res != PFSTMO_OK )
      throw pfs::Exception( "failed to compute the tone-curve" );    

    res = datmo_apply_tone_curve_cc( inX->getRawData(), R.getRawData(), inZ->getRawData(), cols, rows, inX->getRawData(), R.getRawData(), inZ->getRawData(), inY->getRawData(), &tc, df, saturation_factor );
    if( res != PFSTMO_OK )
      throw pfs::Exception( "failed to tone-map the image" );

    //res = datmo_tonemap( inX->getRawData(), R.getRawData(), inZ->getRawData(), cols, rows,
    //   inX->getRawData(), R.getRawData(), inZ->getRawData(), inY->getRawData(),
    //df, ds, contrast_enhance_factor, saturation_factor, white_y, progress_cb );
    ph->newValue( 100 );

    pfs::transformColorSpace( pfs::CS_RGB, inX, &R, inZ, pfs::CS_XYZ, inX, inY, inZ );
    frame->getTags()->setString("LUMINANCE", "DISPLAY");

    delete df;
    delete ds;
}