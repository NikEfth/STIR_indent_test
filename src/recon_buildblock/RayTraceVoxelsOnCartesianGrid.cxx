//
// $Id$
//
/*!
  \file
  \ingroup recon_buildblock

  \brief Implementation of RayTraceVoxelsOnCartesianGrid  

  \author Kris Thielemans
  \author Mustapha Sadki
  \author (loosely based on some C code by Matthias Egger)
  \author PARAPET project

  $Date$
  $Revision$
*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/
/* Modification history:
   KT 30/05/2002 
   start and stop point can now be arbitrarily located
   treatment of LORs parallel to planes is now scale independent (and checked with asserts)
*/

#include "stir/recon_buildblock/RayTraceVoxelsOnCartesianGrid.h"
#include "stir/recon_buildblock/ProjMatrixElemsForOneBin.h"
#include "stir/CartesianCoordinate3D.h"
#include "stir/round.h"
#include <math.h>
#include <algorithm>

#ifndef STIR_NO_NAMESPACE
using std::min;
using std::max;
#endif

START_NAMESPACE_STIR


void 
RayTraceVoxelsOnCartesianGrid
        (ProjMatrixElemsForOneBin& lor, 
         const CartesianCoordinate3D<float>& start_point, 
         const CartesianCoordinate3D<float>& stop_point, 
         const CartesianCoordinate3D<float>& voxel_size,
         const float normalisation_constant)
{

  const CartesianCoordinate3D<float> difference = stop_point-start_point;

  // Make sure there's enough space in the LOR to avoid reallocation.
  // This will make it faster, but also avoid over-allocation
  // (as most STL implementations double the allocated size at over-run).
  lor.reserve(lor.size() +
              static_cast<unsigned int>(ceil(fabs(difference.z())) +
                                        ceil(fabs(difference.y())) +
                                        ceil(fabs(difference.x()))) +
              3);

  // d12 is distance between the 2 points
  // it turns out we can multiply here with the normalisation_constant
  // (as that just scales the coordinate system)
  const float d12 = 
    static_cast<float>(norm(difference*voxel_size) * normalisation_constant);
  
  const int sign_x = difference.x()>=0 ? 1 : -1;
  const int sign_y = difference.y()>=0 ? 1 : -1;
  const int sign_z = difference.z()>=0 ? 1 : -1;

  /* parametrise line in grid units as
     {z,y,x} = start_point + a difference/d12
     So, a step in x towards stop_point will mean a corresponding step inc_x in a
       x+sign_x - x = inc_x difference.x()/d12
     or
       inc_x = d12*sign_x/difference.x()
    i.e. inc_x is always positive

    Special treatment is necessary when the line is parallel to one of the 
    coordinate planes. This is determined by comparing difference with the 
    constant small_difference below. (Note that difference is in grid-units, so
    it has a natural scale of 1.)
  */
  const float small_difference = 1.E-5F;
  const bool zero_diff_in_x = fabs(difference.x())<=small_difference;
  const bool zero_diff_in_y = fabs(difference.y())<=small_difference;
  const bool zero_diff_in_z = fabs(difference.z())<=small_difference;

  const float inc_x = zero_diff_in_x ? d12*1000000.F : d12 / fabs(difference.x());
  const float inc_y = zero_diff_in_y ? d12*1000000.F : d12 / fabs(difference.y());
  const float inc_z = zero_diff_in_z ? d12*1000000.F : d12 / fabs(difference.z());
  
  // intersection points with intra-voxel planes : 
  // find voxel which contains the start_point, and go to its 'left' edge
  const float xmin = round(start_point.x()) - sign_x*0.5F;
  const float ymin = round(start_point.y()) - sign_y*0.5F;
  const float zmin = round(start_point.z()) - sign_z*0.5F;
  // find voxel which contains the end_point, and go to its 'right' edge
  const float xmax = round(stop_point.x()) + sign_x*0.5F;
  const float ymax = round(stop_point.y()) + sign_y*0.5F;  
  const float zmax = round(stop_point.z()) + sign_z*0.5F;

  /* Find a?end for the last intersections with the coordinate planes. 
     amax will then be the smallest of all these a?end.

     If the LOR is parallel to a plane, take care that its a?end is larger than all the others.
     Note that axend <= d12 (difference.x()+1)/difference.x()

     In fact, we will take a?end slightly smaller than the actual last value (i.e. we multiply
     with a factor .9999). This is to avoid rounding errors in the loop below. In this loop,
     we try to detect the end of the LOR by comparing a (which is either ax,ay or az) with
     aend. With exact arithmetic, a? would have been incremented exactly to 
       a?_end_actual = a?start + (?max-?end)*inc_?*sign_?, 
     so we could loop until a==aend_actual. However, because of numerical precision,
     a? might turn out be a tiny bit smaller then a?_end_actual. So, we set aend a tiny bit 
     smaller than aend_actual.
  */
const float axend = zero_diff_in_x ? d12*1000000.F : (xmax - start_point.x()) * inc_x * sign_x *.9999F;
  const float ayend = zero_diff_in_y ? d12*1000000.F : (ymax - start_point.y()) * inc_y * sign_y *.9999F;
  const float azend = zero_diff_in_z ? d12*1000000.F : (zmax - start_point.z()) * inc_z * sign_z *.9999F;
  
  const float amax = min(axend, min(ayend, azend));
  
  // just to be sure, check that axend was set large enough when difference.x() was small.
  assert(fabs(difference.x())>small_difference || axend>amax);
  assert(fabs(difference.y())>small_difference || ayend>amax);
  assert(fabs(difference.z())>small_difference || azend>amax);

  // coordinates of the first Voxel: (same as round(start_point))
  CartesianCoordinate3D<int> current_voxel(round(zmin + sign_z*0.5F), 
					   round(ymin + sign_y*0.5F), 
					   round(xmin + sign_x*0.5F));
  
  /* Find the a? values of the intersection points of the LOR with the planes between voxels.

     Note on special handling of rays parallel to one of the planes:
     
     The corresponding a? value would be -infinity. We just set it to
     a value low enough such that the start value of 'a' is not compromised 
     further on.
     Normally
       a? = (?min-start_point.?) * inc_? * sign_?
     Because the start voxel includes the start_point, we have that
       a? <= -inc_?
     As inc_? is set to some large number when the ray is parallel, this is
     a good value for the ray.
  */
  // with the previous xy-plane
 float az = zero_diff_in_z ? -inc_z : (zmin - start_point.z()) * inc_z * sign_z;
  // with the previous yz-plane
  float ax = zero_diff_in_x ? -inc_x : (xmin - start_point.x()) * inc_x * sign_x;
  // with the previous xz-plane
  float ay = zero_diff_in_y ? -inc_y : (ymin - start_point.y()) * inc_y * sign_y;
  
  // The biggest a?  value gives the start of the a-row 
  float a = max(ax, max(ay,az));      

  // now go the intersections with next plane
  if (zero_diff_in_x) ax = axend; else ax += inc_x;
  if (zero_diff_in_y) ay = ayend; else ay += inc_y;
  if (zero_diff_in_z) az = azend; else az += inc_z;
  
  // just to be sure, check that ax was set large enough when difference.x() was small.
  assert(!zero_diff_in_x || ax>amax);
  assert(!zero_diff_in_y || ay>amax);
  assert(!zero_diff_in_z || az>amax);

  {	  
    // go along the LOR 
    while ( a  < amax) {
      if ( ax < ay )    
        if (  ax  < az ) 
        { // LOR leaves voxel through yz-plane               	
          lor.push_back(ProjMatrixElemsForOneBin::value_type(current_voxel,ax - a));
          a = ax;ax += inc_x;
          current_voxel.x()+=sign_x;
        }      	  
        else{ 	// LOR leaves voxel through xy-plane            	      
          lor.push_back(ProjMatrixElemsForOneBin::value_type(current_voxel,az - a));	    
          a = az ;  az +=  inc_z;
          current_voxel.z()+=sign_z;
        } 
        else  if ( ay < az) {	// LOR leaves voxel through xz-plane 		                           
          lor.push_back(ProjMatrixElemsForOneBin::value_type(current_voxel,ay - a));
          a = ay;   ay +=  inc_y;
          current_voxel.y()+=sign_y;
        }  
        else {// LOR leaves voxel through xy-plane 			                      
          lor.push_back(ProjMatrixElemsForOneBin::value_type(current_voxel,az - a ));
          a = az; az +=  inc_z;
          current_voxel.z()+=sign_z; 
        }
    }	// end of while (a<amax)           
  }
}
END_NAMESPACE_STIR
