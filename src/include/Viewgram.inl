//
// $Id$: $Date$
//
/*!

  \file
  \ingroup buildblock

  \brief Inline implementations of class Viewgram

  \author Sanida Mustafovic
  \author Kris Thielemans
  \author PARAPET project

  \date $Date$

  \version $Revision$
*/

#include "IndexRange2D.h"

START_NAMESPACE_TOMO

template <typename elemT>
int
Viewgram<elemT>::get_segment_num() const
{ return segment_num; }

template <typename elemT>
int
Viewgram<elemT>::get_view_num() const
{ return view_num; }

template <typename elemT>
int
Viewgram<elemT>::get_min_axial_pos_num() const
  {return get_min_index();}

template <typename elemT>
int
Viewgram<elemT>::get_max_axial_pos_num() const
  { return get_max_index(); }

template <typename elemT>
int
Viewgram<elemT>::get_num_axial_poss() const
  { return get_length();}


template <typename elemT>
int
Viewgram<elemT>::get_num_tangential_poss() const
  { return get_length()==0 ? 0 : (*this)[get_min_axial_pos_num()].get_length();}


template <typename elemT>
int
Viewgram<elemT>::get_min_tangential_pos_num() const
  { return get_length()==0 ? 0 :(*this)[get_min_axial_pos_num()].get_min_index();}

template <typename elemT>
int
Viewgram<elemT>::get_max_tangential_pos_num() const
{ return get_length()==0 ? 0 :(*this)[get_min_axial_pos_num()].get_max_index(); }


template <typename elemT>
Viewgram<elemT>
Viewgram<elemT>::get_empty_copy(void) const
  {
    Viewgram<elemT> copy(proj_data_info_ptr, get_view_num(), get_segment_num());
    return copy;
}

template <typename elemT>
const ProjDataInfo*
Viewgram<elemT>:: get_proj_data_info_ptr()const
{
  return proj_data_info_ptr.get();
}


template <typename elemT>
Viewgram<elemT>::
Viewgram(const Array<2,elemT>& p, 
	 const shared_ptr<ProjDataInfo>& pdi_ptr, 
	 const int v_num, const int s_num) 
  :
  Array<2,elemT>(p), proj_data_info_ptr(pdi_ptr),
  view_num(v_num), segment_num(s_num)
{
  assert(view_num <= proj_data_info_ptr->get_max_view_num());
  assert(view_num >= proj_data_info_ptr->get_min_view_num());
  // segment_num is already checked by doing get_max_axial_pos_num(s_num)

  assert( get_min_axial_pos_num() == pdi_ptr->get_min_axial_pos_num(s_num));
  assert( get_max_axial_pos_num() == pdi_ptr->get_max_axial_pos_num(s_num));
  assert( get_min_tangential_pos_num() == pdi_ptr->get_min_tangential_pos_num());
  assert( get_max_tangential_pos_num() == pdi_ptr->get_max_tangential_pos_num());
}

template <typename elemT>
Viewgram<elemT>::
Viewgram(const shared_ptr<ProjDataInfo>& pdi_ptr, 
	 const int v_num, const int s_num) 
  : 
  Array<2,elemT>(IndexRange2D (pdi_ptr->get_min_axial_pos_num(s_num),
			       pdi_ptr->get_max_axial_pos_num(s_num),
			       pdi_ptr->get_min_tangential_pos_num(),
			       pdi_ptr->get_max_tangential_pos_num())), 
  proj_data_info_ptr(pdi_ptr),
  view_num(v_num),
  segment_num(s_num)
{
  assert(view_num <= proj_data_info_ptr->get_max_view_num());
  assert(view_num >= proj_data_info_ptr->get_min_view_num());
  // segment_num is already checked by doing get_max_axial_pos_num(s_num)
}


END_NAMESPACE_TOMO
