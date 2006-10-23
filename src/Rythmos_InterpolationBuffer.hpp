//@HEADER
// ***********************************************************************
//
//                           Rythmos Package
//                 Copyright (2006) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Todd S. Coffey (tscoffe@sandia.gov)
//
// ***********************************************************************
//@HEADER

#ifndef Rythmos_INTERPOLATION_BUFFER_H
#define Rythmos_INTERPOLATION_BUFFER_H

#include "Rythmos_InterpolationBufferBase.hpp"
#include "Rythmos_Interpolator.hpp"
#include "Rythmos_LinearInterpolator.hpp"
#include "Rythmos_DataStore.hpp"

#include "Thyra_VectorBase.hpp"

namespace Rythmos {

/** \brief class for defining linear interpolation buffer functionality. */
template<class Scalar> 
class InterpolationBuffer : virtual public InterpolationBufferBase<Scalar>
{
  public:

    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;
    
    /** \brief. */
    InterpolationBuffer();
    InterpolationBuffer( const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_, int storage_ );

    /// Initialize the buffer:
    void initialize( const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_, int storage_ );

    /// Set the interpolator for this buffer
    void SetInterpolator(const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_);

    /// Set the maximum storage of this buffer
    void SetStorage( int storage );
        
    /// Destructor
    ~InterpolationBuffer() {};

    /// Add point to buffer
    bool SetPoints(
      const std::vector<Scalar>& time_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec
      ,const std::vector<ScalarMag> & accuracy_vec 
      );

    bool SetPoints(
      const std::vector<Scalar>& time_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec);

    /// Get value from buffer
    bool GetPoints(
      const std::vector<Scalar>& time_vec
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec
      ,std::vector<ScalarMag>* accuracy_vec) const;

    /// Fill data in from another interpolation buffer
    bool SetRange(
      const Scalar& time_lower
      ,const Scalar& time_upper
      ,const InterpolationBufferBase<Scalar>& IB);

    /// Get interpolation nodes
    bool GetNodes(std::vector<Scalar>* time_vec) const;

    /// Remove interpolation nodes
    bool RemoveNodes(std::vector<Scalar>& time_vec);

    /// Get order of interpolation
    int GetOrder() const;

    /// Redefined from Teuchos::Describable
    /** \brief . */
    std::string description() const;

    /** \brief . */
    void describe(
      Teuchos::FancyOStream       &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ) const;

    /// Redefined from Teuchos::ParameterListAcceptor
    /** \brief . */
    void setParameterList(Teuchos::RefCountPtr<Teuchos::ParameterList> const& paramList);

    /** \brief . */
    Teuchos::RefCountPtr<Teuchos::ParameterList> getParameterList();

    /** \brief . */
    Teuchos::RefCountPtr<Teuchos::ParameterList> unsetParameterList();
    
  private:

    Teuchos::RefCountPtr<Interpolator<Scalar> > interpolator;
    int storage_limit;
    typename DataStore<Scalar>::DataStoreVector_t data_vec;

#ifdef Rythmos_DEBUG
    int debugLevel;
    Teuchos::RefCountPtr<Teuchos::FancyOStream> debug_out;
#endif // Rythmos_DEBUG

};

// ////////////////////////////
// Defintions
template<class Scalar>
InterpolationBuffer<Scalar>::InterpolationBuffer()
{
  initialize(Teuchos::null,0);
}

template<class Scalar>
InterpolationBuffer<Scalar>::InterpolationBuffer( 
    const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_
    ,int storage_ 
    )
{
  initialize(interpolator_,storage_);
}

template<class Scalar>
void InterpolationBuffer<Scalar>::initialize( 
    const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_
    ,int storage_
    )
{
#ifdef Rythmos_DEBUG
  debugLevel = 2;
  debug_out = Teuchos::VerboseObjectBase::getDefaultOStream();
  debug_out->precision(15);
  debug_out->setMaxLenLinePrefix(30);
  debug_out->pushLinePrefix("Rythmos::InterpolationBuffer");
  debug_out->setShowLinePrefix(true);
  debug_out->setTabIndentStr("    ");
  *debug_out << "Initializing InterpolationBuffer" << std::endl;
  Teuchos::OSTab ostab(debug_out,1,"IB::initialize");
  if (debugLevel > 1)
    *debug_out << "Calling SetInterpolator..." << std::endl;
#endif // Rythmos_DEBUG
  SetInterpolator(interpolator_);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Calling SetStorage..." << std::endl;
#endif // Rythmos_DEBUG
  SetStorage(storage_);
}

template<class Scalar>
void InterpolationBuffer<Scalar>::SetStorage( int storage_ )
{
  storage_limit = max(2,storage_); // Minimum of two points so interpolation is possible
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::SetStorage");
  if (debugLevel > 1)
  {
    *debug_out << "storage_limit = " << storage_limit << std::endl;
  }
#endif // Rythmos_DEBUG
}

template<class Scalar>
void InterpolationBuffer<Scalar>::SetInterpolator(
    const Teuchos::RefCountPtr<Interpolator<Scalar> >& interpolator_
    )
{
  if (interpolator_ == Teuchos::null)
  {
    interpolator = Teuchos::rcp(new LinearInterpolator<Scalar>);
  }
  else
  {
    interpolator = interpolator_;
  }
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::SetInterpolator");
  if (debugLevel > 1)
  {
    *debug_out << "interpolator = " << interpolator->description() << std::endl;
  }
#endif // Rythmos_DEBUG
}

template<class Scalar>
bool InterpolationBuffer<Scalar>::SetPoints( 
    const std::vector<Scalar>& time_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec 
    ,const std::vector<ScalarMag> & accuracy_vec 
    )
{
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::SetPoints");
  if (debugLevel > 1)
  {
    *debug_out << "time_vec = " << std::endl;
    for (int i=0 ; i<time_vec.size() ; ++i)
      *debug_out << "time_vec[" << i << "] = " << time_vec[i] << std::endl;
    *debug_out << "x_vec = " << std::endl;
    for (int i=0 ; i<x_vec.size() ; ++i)
    {
      *debug_out << "x_vec[" << i << "] = " << std::endl;
      x_vec[i]->describe(*debug_out,Teuchos::VERB_EXTREME);
    }
    if (xdot_vec.size() == 0)
      *debug_out << "xdot_vec = empty vector" << std::endl;
    else
      *debug_out << "xdot_vec = " << std::endl;
    for (int i=0 ; i<xdot_vec.size() ; ++i)
    {
      if (xdot_vec[i] == Teuchos::null)
        *debug_out << "xdot_vec[" << i << "] = Teuchos::null" << std::endl;
      else
      {
        *debug_out << "xdot_vec[" << i << "] = " << std::endl;
        xdot_vec[i]->describe(*debug_out,Teuchos::VERB_EXTREME);
      }
    }
    if (accuracy_vec.size() == 0)
      *debug_out << "accuracy_vec = empty vector" << std::endl;
    else
      *debug_out << "accuracy_vec = " << std::endl;
    for (int i=0 ; i<accuracy_vec.size() ; ++i)
      *debug_out << "accuracy_vec[" << i << "] = " << accuracy_vec[i] << std::endl;
  }
#endif // Rythmos_DEBUG
  typename DataStore<Scalar>::DataStoreList_t input_data_list;
  VectorToDataStoreList<Scalar>(time_vec,x_vec,xdot_vec,accuracy_vec,&input_data_list);
  input_data_list.sort();
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    *debug_out << "input_data_list after sorting = " << std::endl;
    typename DataStore<Scalar>::DataStoreList_t::iterator
      data_it = input_data_list.begin();
    int i=0;
    for (; data_it != input_data_list.end() ; data_it++)
    {
      *debug_out << "item " << i << ":" << std::endl;
      data_it->describe(*debug_out,Teuchos::VERB_EXTREME);
      i++;
    }
  }
#endif // Rythmos_DEBUG
  // Determine if time is already in list and if so, replace existing data with new data.
  typename DataStore<Scalar>::DataStoreList_t::iterator 
    input_it = input_data_list.begin();
  while (input_it != input_data_list.end())
  {
#ifdef Rythmos_DEBUG
    if (debugLevel > 1)
      *debug_out << "Looking for time = " << (*input_it).time << " in data_vec to replace with new value" << std::endl;
#endif // Rythmos_DEBUG
    typename DataStore<Scalar>::DataStoreVector_t::iterator 
      node_it = std::find(data_vec.begin(),data_vec.end(),*input_it);
    if (node_it != data_vec.end())
    {
      int node_index = node_it - data_vec.begin(); // 10/17/06 tscoffe:  this
                                                   // is how you back out an
                                                   // element's index into a
                                                   // vector from its iterator.
#ifdef Rythmos_DEBUG
      if (debugLevel > 1)
      {
        *debug_out << "Replacing data_vec[" << node_index << "] = " << std::endl;
        node_it->describe(*debug_out,Teuchos::VERB_EXTREME);
        *debug_out << "with:" << std::endl;
        input_it->describe(*debug_out,Teuchos::VERB_EXTREME);
      }
#endif // Rythmos_DEBUG
      data_vec[node_index] = *input_it;
      input_it = input_data_list.erase(input_it);
#ifdef Rythmos_DEBUG
      if (debugLevel > 1)
      {
        *debug_out << "input_data_list after removing an element = " << std::endl;
        typename DataStore<Scalar>::DataStoreList_t::iterator
          data_it = input_data_list.begin();
        int i=0;
        for (; data_it != input_data_list.end() ; data_it++)
        {
          *debug_out << "item " << i << ":" << std::endl;
          data_it->describe(*debug_out,Teuchos::VERB_EXTREME);
          i++;
        }
      }
#endif // Rythmos_DEBUG
    }
    else
    {
      input_it++;
    }
  }
  // Check that we're not going to exceed our storage limit:
  if ((data_vec.size()+input_data_list.size()) > storage_limit)
    return(false);
  // Now add all the remaining points to data_vec
  data_vec.insert(data_vec.end(),input_data_list.begin(),input_data_list.end());
  // And sort data_vec:
  std::sort(data_vec.begin(),data_vec.end());
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    *debug_out << "data_vec at end of SetPoints:" << std::endl;
    for (int i=0 ; i<data_vec.size() ; ++i)
    {
      *debug_out << "data_vec[" << i << "] = " << std::endl;
      data_vec[i].describe(*debug_out,Teuchos::VERB_EXTREME);
    }
  }
#endif // Rythmos_DEBUG
  return(true);
}

template<class Scalar>
bool InterpolationBuffer<Scalar>::SetPoints( 
    const std::vector<Scalar>& time_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec )
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  std::vector<ScalarMag> accuracy_vec;
  accuracy_vec.reserve(x_vec.size());
  for (int i=0;i<x_vec.size();++i)
    accuracy_vec[i] = ST::zero();
  return(this->SetPoints(time_vec,x_vec,xdot_vec,accuracy_vec));
}

template<class Scalar>
bool InterpolationBuffer<Scalar>::GetPoints(
    const std::vector<Scalar>& time_vec
    ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec
    ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec
    ,std::vector<ScalarMag>* accuracy_vec) const
{
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::GetPoints");
  if (debugLevel > 1)
    *debug_out << "Calling interpolate..." << std::endl;
#endif // Rythmos_DEBUG
  typename DataStore<Scalar>::DataStoreVector_t data_out;
  bool status = interpolator->interpolate(data_vec, time_vec, &data_out);
  if (!status) return(status);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Interpolation successful" << std::endl;
#endif // Rythmos_DEBUG
  std::vector<Scalar> time_out_vec;
  DataStoreVectorToVector<Scalar>(data_out, &time_out_vec, x_vec, xdot_vec, accuracy_vec);
  // Double check that time_out_vec == time_vec
  if (time_vec.size() != time_out_vec.size()) return(false);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Conversion of DataStoreVector to Vector successful" << std::endl;
#endif // Rythmos_DEBUG
  return(true);
}


template<class Scalar>
bool InterpolationBuffer<Scalar>::SetRange(
    const Scalar& time_lower
    ,const Scalar& time_upper
    ,const InterpolationBufferBase<Scalar>& IB )
{
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::SetRange");
  if (debugLevel > 1)
  {
    *debug_out << "time_lower = " << time_lower << std::endl;
    *debug_out << "time_upper = " << time_upper << std::endl;
    *debug_out << "IB = " << IB.description() << std::endl;
  }
#endif // Rythmos_DEBUG
  std::vector<Scalar> input_nodes;
  bool status = IB.GetNodes(&input_nodes);
  if (!status) return(status);
  std::sort(input_nodes.begin(),input_nodes.end());
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    *debug_out << "input_nodes after sorting = " << std::endl;
    for (int i=0 ; i<input_nodes.size() ; ++i)
      *debug_out << "input_nodes[" << i << "] = " << input_nodes[i] << std::endl;
  }
#endif // Rythmos_DEBUG
  // Remove nodes outside the range [time_lower,time_upper]
  typename std::vector<Scalar>::iterator input_it_lower = input_nodes.begin();
  for (; input_it_lower != input_nodes.end() ; input_it_lower++)
  {
    if (*input_it_lower >= time_lower)
    {
      break;
    }
  }
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    int n0 = 0;
    int n1 = input_it_lower - input_nodes.begin();
    *debug_out << "Removing input_nodes before time_lower with indices: [" << n0 << "," << n1 << ")" << std::endl;
    for (int i=n0 ; i<n1; ++i)
    {
      *debug_out << "  input_nodes[" << i << "] = " << input_nodes[i] << std::endl;
    }
  }
#endif // Rythmos_DEBUG
  // tscoffe 10/19/06 Note:  erase removes the range [it_begin,it_end)
  if (input_it_lower - input_nodes.begin() >= 0)
    input_nodes.erase(input_nodes.begin(),input_it_lower);
  typename std::vector<Scalar>::iterator input_it_upper = input_nodes.end();
  input_it_upper--;
  for (; input_it_upper != input_nodes.begin() ; input_it_upper--)
  {
    if (*input_it_upper <= time_upper)
    {
      input_it_upper++;
      break;
    }
  }
  // This is to handle the case of one element in the vector
  if (*input_it_upper <= time_upper)
    input_it_upper++;
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    int n0 = input_it_upper - input_nodes.begin();
    int n1 = input_nodes.size();
    *debug_out << "Removing input_nodes after time_upper with indices [" << n0 << "," << n1 << ")" << std::endl;
    for (int i=n0 ; i<n1; ++i)
    {
      *debug_out << "  input_nodes[" << i << "] = " << input_nodes[i] << std::endl;
    }
  }
#endif // Rythmos_DEBUG
  if (input_it_upper - input_nodes.begin() < input_nodes.size())
    input_nodes.erase(input_it_upper,input_nodes.end());
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    *debug_out << "input_nodes remaining:" << std::endl;
    for (int i=0 ; i<input_nodes.size() ; ++i)
      *debug_out << "input_nodes[" << i << "] = " << input_nodes[i] << std::endl;
  }
#endif // Rythmos_DEBUG

  // Ask IB to interpolate more points if IB's order is higher than ours
  typedef Teuchos::ScalarTraits<Scalar> ST;
  Scalar h_safety = Scalar(2*ST::one());
  int IBOrder = IB.GetOrder();
  if (IBOrder >= interpolator->order())
  {
    std::list<Scalar> add_nodes;
    for (int i=0 ; i<input_nodes.size()-1 ; ++i)
    {
      Scalar h_0 = input_nodes[i+1] - input_nodes[i];
      Scalar h = pow(h_0,(IBOrder/interpolator->order())/h_safety);
#ifdef Rythmos_DEBUG
      if (debugLevel > 1)
      {
        *debug_out << "i = " << i << std::endl;
        *debug_out << "interpolator->order() = " << interpolator->order() << std::endl;
        *debug_out << "IB.GetOrder() = " << IB.GetOrder() << std::endl;
        *debug_out << "h = " << h << std::endl;
      }
#endif // Rythmos_DEBUG
      Scalar N = ceil(h_0/h);
      h = Scalar(h_0/N);
#ifdef Rythmos_DEBUG
      if (debugLevel > 1)
      {
        *debug_out << "h_0 = " << h_0 << std::endl;
        *debug_out << "N = " << N << std::endl;
        *debug_out << "h = " << h << std::endl;
        *debug_out << "Inserting an additional " << N-1 << " points to be interpolated:" << std::endl;
      }
#endif // Rythmos_DEBUG
      for (int j=1 ; j<N ; ++j)
      {
#ifdef Rythmos_DEBUG
        if (debugLevel > 1)
          *debug_out << input_nodes[i]+j*h << std::endl;
#endif // Rythmos_DEBUG
        add_nodes.push_back(input_nodes[i]+j*h);
      }
    }
    input_nodes.insert(input_nodes.end(),add_nodes.begin(),add_nodes.end());
    std::sort(input_nodes.begin(),input_nodes.end());
  }
  // If IB's order is lower than ours, then simply grab the node values and continue.
  // If IB's order is higher than ours, then grab the node values and ask IB to
  // interpolate extra values so that our order of accuracy is approximately
  // the same as the other IB's.
  // One approach:
  // Lets say IB's order is p and our order is r (p>r).
  // Given a particular interval with spacing h_0, the order of accuracy of IB is h_0^p
  // We want to find a spacing h such that h^r = h_0^p.  Clearly, this is h = h_0^{p/r}.
  // Given this new spacing, divide up the interval h_0 into h_0/h subintervals
  // and ask for the IB to interpolate points.  This will match basic order of
  // accuracy estimates.  Its probably a good idea to include a fudge factor in
  // there too.  E.g. h = h_0^{p/r}/fudge.

  // Don't forget to check the interval [time_lower,time_upper].
  // Use SetPoints and check return value to make sure we observe storage_limit.

  std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > input_x;
  std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > input_xdot;
  std::vector<ScalarMag> input_accuracy;
  status = IB.GetPoints( input_nodes, &input_x, &input_xdot, &input_accuracy );
  if (!status) return(status);
  // We could check that the accuracy meets our criteria here.
  status = SetPoints( input_nodes, input_x, input_xdot, input_accuracy );
  return(status);
}

template<class Scalar>
bool InterpolationBuffer<Scalar>::GetNodes( std::vector<Scalar>* time_vec ) const
{
  int N = data_vec.size();
  time_vec->clear();
  time_vec->reserve(N);
  for (int i=0 ; i<N ; ++i)
    time_vec->push_back(data_vec[i].time);
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"IB::GetNodes");
  if (debugLevel > 1)
  {
    *debug_out << this->description() << std::endl;
    for (int i=0 ; i<time_vec->size() ; ++i)
      *debug_out << "time_vec[" << i << "] = " << (*time_vec)[i] << std::endl;
  }
#endif // Rythmos_DEBUG
  return(true);
}

template<class Scalar>
bool InterpolationBuffer<Scalar>::RemoveNodes( std::vector<Scalar>& time_vec ) 
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  ScalarMag z = ST::zero();
  Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > vec_temp;
  int N = time_vec.size();
  for (int i=0; i<N ; ++i)
  {
    DataStore<Scalar> ds_temp(time_vec[i],vec_temp,vec_temp,z);
    typename DataStore<Scalar>::DataStoreVector_t::iterator 
      data_it = std::find(data_vec.begin(),data_vec.end(),ds_temp);
    if (data_it != data_vec.end())
      data_vec.erase(data_it);
  }
  return(true);
}

template<class Scalar>
int InterpolationBuffer<Scalar>::GetOrder() const
{
  return(interpolator->order());
}

template<class Scalar>
std::string InterpolationBuffer<Scalar>::description() const
{
  std::string name = "Rythmos::InterpolationBuffer";
  return(name);
}

template<class Scalar>
void InterpolationBuffer<Scalar>::describe(
      Teuchos::FancyOStream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ) const
{
  if (verbLevel == Teuchos::VERB_EXTREME)
  {
    out << description() << "::describe" << std::endl;
    out << "interpolator = " << interpolator->description() << std::endl;
    out << "storage_limit = " << storage_limit << std::endl;
    out << "data_vec = " << std::endl;
    for (int i=0; i<data_vec.size() ; ++i)
    {
      out << "data_vec[" << i << "] = " << std::endl;
      data_vec[i].describe(out,Teuchos::VERB_EXTREME);
    }
  }
}

template <class Scalar>
void InterpolationBuffer<Scalar>::setParameterList(Teuchos::RefCountPtr<Teuchos::ParameterList> const& paramList)
{
}

template <class Scalar>
Teuchos::RefCountPtr<Teuchos::ParameterList> InterpolationBuffer<Scalar>::getParameterList()
{
  return(Teuchos::null);
}

template <class Scalar>
Teuchos::RefCountPtr<Teuchos::ParameterList> InterpolationBuffer<Scalar>::unsetParameterList()
{
  return(Teuchos::null);
}

} // namespace Rythmos

#endif // Rythmos_INTERPOLATION_BUFFER_H
