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

#ifndef Rythmos_LINEAR_INTERPOLATOR_H
#define Rythmos_LINEAR_INTERPOLATOR_H

#include "Rythmos_InterpolatorBase.hpp"
#include "Thyra_VectorStdOps.hpp"


namespace Rythmos {


/** \brief Concrete implemenation of <tt>InterpolatorBase</tt> just just does
 * simple linear interploation.
 */
template<class Scalar>
class LinearInterpolator : virtual public InterpolatorBase<Scalar>
{
public:

  /** \brief . */
  ~LinearInterpolator() {};

  /** \brief . */
  LinearInterpolator();

  /** \brief . */
  bool supportsCloning() const;

  /** \brief . */
  RCP<InterpolatorBase<Scalar> > cloneInterpolator() const;

  /** \brief . */
  void interpolate(
    const typename DataStore<Scalar>::DataStoreVector_t &data_in,
    const Array<Scalar> &t_values,
    typename DataStore<Scalar>::DataStoreVector_t *data_out
    ) const;

  /** \brief . */
  int order() const; 

  /** \brief . */
  std::string description() const;

  /** \brief . */
  void describe(
    FancyOStream &out,
    const Teuchos::EVerbosityLevel verbLevel
    ) const;

  /** \brief . */
  void setParameterList(RCP<ParameterList> const& paramList);

  /** \brief . */
  RCP<ParameterList> getNonconstParameterList();

  /** \brief . */
  RCP<ParameterList> unsetParameterList();

private:

  RCP<ParameterList> parameterList_;

};

// non-member constructor
template<class Scalar>
RCP<LinearInterpolator<Scalar> > linearInterpolator()
{
  RCP<LinearInterpolator<Scalar> > li = rcp(new LinearInterpolator<Scalar>() );
  return li;
}

template<class Scalar>
LinearInterpolator<Scalar>::LinearInterpolator()
{}


template<class Scalar>
bool LinearInterpolator<Scalar>::supportsCloning() const
{
  return true;
}


template<class Scalar>
RCP<InterpolatorBase<Scalar> >
LinearInterpolator<Scalar>::cloneInterpolator() const
{
  RCP<LinearInterpolator<Scalar> >
    interpolator = Teuchos::rcp(new LinearInterpolator<Scalar>);
  if (!is_null(parameterList_))
    interpolator->parameterList_ = parameterList(*parameterList_);
  return interpolator;
}


template<class Scalar>
void LinearInterpolator<Scalar>::interpolate(
  const typename DataStore<Scalar>::DataStoreVector_t &data_in,
  const Array<Scalar> &t_values,
  typename DataStore<Scalar>::DataStoreVector_t *data_out
  ) const
{

  using Teuchos::as;
  typedef Teuchos::ScalarTraits<Scalar> ST;

#ifdef TEUCHOS_DEBUG
  assertBaseInterpolatePreconditions(data_in,t_values,data_out);
#endif // TEUCHOS_DEBUG
  
  // Output info
  const RCP<FancyOStream> out = this->getOStream();
  const Teuchos::EVerbosityLevel verbLevel = this->getVerbLevel(); 
  Teuchos::OSTab ostab(out,1,"LI::interpolator");
  if ( as<int>(verbLevel) >= as<int>(Teuchos::VERB_HIGH) ) {
    *out << "data_in:" << std::endl;
    for (unsigned int i=0 ; i<data_in.size() ; ++i) {
      *out << "data_in[" << i << "] = " << std::endl;
      data_in[i].describe(*out,Teuchos::VERB_EXTREME);
    }
    *out << "t_values = " << std::endl;
    for (unsigned int i=0 ; i<t_values.size() ; ++i) {
      *out << "t_values[" << i << "] = " << t_values[i] << std::endl;
    }
  }

  data_out->clear();

  // Return immediatly if not time points are requested ...
  if (t_values.size() == 0) {
    return;
  }

  if (data_in.size() == 1) {
    // trivial case of one node.  Preconditions assert that t_values[0] ==
    // data_in[0].time so we can just pass it out
    DataStore<Scalar> DS(data_in[0]);
    data_out->push_back(DS);
  }
  else { // data_in.size() >= 2
    int n = 0; // index into t_values
    // Loop through all of the time interpolation points in the buffer and
    // satisfiy all of the requested time points that you find.  NOTE: The
    // loop will be existed once all of the time points are satisified (see
    // return below).
    for (int i=0 ; i < as<int>(data_in.size())-1; ++i) {
      const Scalar& ti = data_in[i].time;
      const Scalar& tip1 = data_in[i+1].time;
      const Scalar  h = tip1-ti;
      const TimeRange<Scalar> range_i(ti,tip1);
      // For the interploation range of [ti,tip1], satisify all of the
      // requested points in this range.
      while ( range_i.isInRange(t_values[n]) ) {
        // First we check for exact node matches:
        if (compareTimeValues(t_values[n],ti)==0) {
          DataStore<Scalar> DS(data_in[i]);
          data_out->push_back(DS);
        }
        else if (compareTimeValues(t_values[n],tip1)==0) {
          DataStore<Scalar> DS(data_in[i+1]);
          data_out->push_back(DS);
        }
        else {
          // interpolate this point
          //
          // x(t) = (t-ti)/(tip1-ti) * xip1 + (1-(t-ti)/(tip1-ti)) * xi
          //
          // Above, it is easy to see that:
          //
          //    x(ti) = xi
          //    x(tip1) = xip1
          //
          DataStore<Scalar> DS;
          const Scalar& t = t_values[n];
          DS.time = t;
          // Get the time and interpolation node points
          RCP<const Thyra::VectorBase<Scalar> > xi = data_in[i].x;
          RCP<const Thyra::VectorBase<Scalar> > xip1 = data_in[i+1].x;
          RCP<const Thyra::VectorBase<Scalar> > xdoti = data_in[i].xdot;
          RCP<const Thyra::VectorBase<Scalar> > xdotip1 = data_in[i+1].xdot;
          // Get constants used in interplation
          const Scalar dt = t-ti;
          const Scalar dt_over_h = dt / h;
          const Scalar one_minus_dt_over_h = ST::one() - dt_over_h;
          // x = dt/h * xip1 + (1-dt/h) * xi
          RCP<Thyra::VectorBase<Scalar> > x = createMember(xi->space());
          Thyra::V_StVpStV(&*x,dt_over_h,*xip1,one_minus_dt_over_h,*xi);
          DS.x = x;
          // x = dt/h * xdotip1 + (1-dt/h) * xdoti
          RCP<Thyra::VectorBase<Scalar> > xdot;
          if ((xdoti != Teuchos::null) && (xdotip1 != Teuchos::null)) {
            xdot = createMember(xdoti->space());
            Thyra::V_StVpStV(&*xdot,dt_over_h,*xdotip1,one_minus_dt_over_h,*xdoti);
          }
          DS.xdot = xdot;
          // Estimate our accuracy ???
          DS.accuracy = h;
          // 2007/12/06: rabartl: Above, should the be a relative value of
          // some type.  What does this really mean?
          // Store this interplation
          data_out->push_back(DS);
        }
        // Move to the next user time point to consider!
        n++;
        if (n == as<int>(t_values.size())) {
          // WE ARE ALL DONE!  MOVE OUT!
          return;
        }
      }
      // Move on the the next interpolation time range
    }
  } // data_in.size() == 1
}


template<class Scalar>
int LinearInterpolator<Scalar>::order() const
{
  return(1);
}


template<class Scalar>
std::string LinearInterpolator<Scalar>::description() const
{
  std::string name = "Rythmos::LinearInterpolator";
  return(name);
}


template<class Scalar>
void LinearInterpolator<Scalar>::describe(
  FancyOStream &out,
  const Teuchos::EVerbosityLevel verbLevel
  ) const
{
  using Teuchos::as;
  if ( (as<int>(verbLevel) == as<int>(Teuchos::VERB_DEFAULT) ) ||
       (as<int>(verbLevel) >= as<int>(Teuchos::VERB_LOW)     )
     )
  {
    out << description() << "::describe" << std::endl;
  }
  else if (as<int>(verbLevel) >= as<int>(Teuchos::VERB_LOW))
  {}
  else if (as<int>(verbLevel) >= as<int>(Teuchos::VERB_MEDIUM))
  {}
  else if (as<int>(verbLevel) >= as<int>(Teuchos::VERB_HIGH))
  {}
}


template <class Scalar>
void LinearInterpolator<Scalar>::setParameterList(
  RCP<ParameterList> const& paramList
  )
{
  // 2007/12/06: rabartl: ToDo: Validate this parameter list!
  parameterList_ = paramList;
  int outputLevel = parameterList_->get( "outputLevel", int(-1) );
  outputLevel = std::min(std::max(outputLevel,-1),4);
  this->setVerbLevel(static_cast<Teuchos::EVerbosityLevel>(outputLevel));
  // 2007/05/18: rabartl: ToDo: Replace with standard "Verbose Object"
  // sublist! and validate the sublist!

}


template <class Scalar>
RCP<ParameterList>
LinearInterpolator<Scalar>::getNonconstParameterList()
{
  return(parameterList_);
}


template <class Scalar>
RCP<ParameterList>
LinearInterpolator<Scalar>::unsetParameterList()
{
  RCP<ParameterList> temp_param_list;
  std::swap( temp_param_list, parameterList_ );
  return(temp_param_list);
}



} // namespace Rythmos


#endif // Rythmos_LINEAR_INTERPOLATOR_H
