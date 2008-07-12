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

#ifndef Rythmos_FORWARDEULER_STEPPER_H
#define Rythmos_FORWARDEULER_STEPPER_H

#include "Rythmos_StepperBase.hpp"
#include "Teuchos_RCP.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_ModelEvaluator.hpp"
#include "Thyra_ModelEvaluatorHelpers.hpp"

namespace Rythmos {

/** \brief . */
template<class Scalar>
class ForwardEulerStepper : virtual public StepperBase<Scalar>
{
  public:

    typedef Teuchos::ScalarTraits<Scalar> ST;
    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;
    
    /** \brief . */
    ForwardEulerStepper();
    ForwardEulerStepper(const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    void setModel(const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >
    getModel() const;

    /** \brief . */
    RCP<const Thyra::VectorSpaceBase<Scalar> > get_x_space() const;
    
    /** \brief . */
    ~ForwardEulerStepper();

    /** \brief . */
    Scalar takeStep(Scalar dt, StepSizeType flag);

    /** \brief . */
    const StepStatus<Scalar> getStepStatus() const;

    /** \brief . */
    std::string description() const;

    /** \brief . */
    std::ostream& describe(
      std::ostream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ,const std::string          leadingIndent
      ,const std::string          indentSpacer
      ) const;
    
    /// Redefined from InterpolationBufferBase 
    /// Add points to buffer
    void addPoints(
      const Array<Scalar>& time_vec
      ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& x_vec
      ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& xdot_vec
      );
    
    /// Get values from buffer
    void getPoints(
      const Array<Scalar>& time_vec
      ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* x_vec
      ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* xdot_vec
      ,Array<ScalarMag>* accuracy_vec
      ) const;

    /// Fill data in from another interpolation buffer
    void setRange(
      const TimeRange<Scalar>& range,
      const InterpolationBufferBase<Scalar> & IB
      );

    /** \brief . */
    TimeRange<Scalar> getTimeRange() const;

    /// Get interpolation nodes
    void getNodes(Array<Scalar>* time_vec) const;

    /// Remove interpolation nodes
    void removeNodes(Array<Scalar>& time_vec);

    /// Get order of interpolation
    int getOrder() const;

    /// Redefined from Teuchos::ParameterListAcceptor
    /** \brief . */
    void setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& paramList);

    /** \brief . */
    Teuchos::RCP<Teuchos::ParameterList> getNonconstParameterList();

    /** \brief . */
    Teuchos::RCP<Teuchos::ParameterList> unsetParameterList();

  private:

    Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > model_;
    Teuchos::RCP<Thyra::VectorBase<Scalar> > solution_vector_;
    Teuchos::RCP<Thyra::VectorBase<Scalar> > residual_vector_;
    Scalar t_;
    Scalar dt_;

    Teuchos::RCP<Teuchos::ParameterList> parameterList_;
    bool isInitialized_;

    // Private member functions:
    void initialize_();

};

template<class Scalar>
ForwardEulerStepper<Scalar>::ForwardEulerStepper(const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model)
  : isInitialized_(false)
{
  this->setModel(model);
  initialize_();
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::initialize_()
{
  t_ = ST::zero();
  solution_vector_ = model_->getNominalValues().get_x()->clone_v();
  residual_vector_ = Thyra::createMember(model_->get_f_space());
  isInitialized_ = true;
}

template<class Scalar>
ForwardEulerStepper<Scalar>::ForwardEulerStepper()
  : isInitialized_(false)
{
}

template<class Scalar>
ForwardEulerStepper<Scalar>::~ForwardEulerStepper()
{
}

template<class Scalar>
RCP<const Thyra::VectorSpaceBase<Scalar> > ForwardEulerStepper<Scalar>::get_x_space() const
{
  TEST_FOR_EXCEPTION(!isInitialized_,std::logic_error,"Error, attempting to call get_x_space before initialization!\n");
  return(solution_vector_->space());
}

template<class Scalar>
Scalar ForwardEulerStepper<Scalar>::takeStep(Scalar dt, StepSizeType flag)
{
  if (flag == STEP_TYPE_VARIABLE) { 
    // print something out about this method not supporting automatic variable step-size
    typedef Teuchos::ScalarTraits<Scalar> ST;
    return(-ST::one());
  }
/*
  Thyra::ModelEvaluatorBase::InArgs<Scalar>   inArgs  = model_->createInArgs();
  Thyra::ModelEvaluatorBase::OutArgs<Scalar>  outArgs = model_->createOutArgs();

  inArgs.set_x(solution_vector_);
  inArgs.set_t(t_+dt);

  outArgs.set_f(residual_vector_);

  model_->evalModel(inArgs,outArgs);
*/
  Thyra::eval_f<Scalar>(*model_,*solution_vector_,t_+dt,&*residual_vector_);

  // solution_vector = solution_vector + dt*residual_vector
  Thyra::Vp_StV(&*solution_vector_,dt,*residual_vector_); 
  t_ += dt;

  dt_ = dt;

  return(dt);
}

template<class Scalar>
const StepStatus<Scalar> ForwardEulerStepper<Scalar>::getStepStatus() const
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  StepStatus<Scalar> stepStatus;

  stepStatus.stepSize = dt_; 
  stepStatus.order = 1;
  stepStatus.time = t_;
  stepStatus.stepLETValue = Scalar(-ST::one()); 
  stepStatus.solution = solution_vector_;
  stepStatus.residual = residual_vector_;

  return(stepStatus);
}

template<class Scalar>
std::string ForwardEulerStepper<Scalar>::description() const
{
  std::string name = "Rythmos::ForwardEulerStepper";
  return(name);
}

template<class Scalar>
std::ostream& ForwardEulerStepper<Scalar>::describe(
      std::ostream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ,const std::string          leadingIndent
      ,const std::string          indentSpacer
      ) const
{
  if ( (static_cast<int>(verbLevel) == static_cast<int>(Teuchos::VERB_DEFAULT) ) ||
       (static_cast<int>(verbLevel) >= static_cast<int>(Teuchos::VERB_LOW)     )
     ) {
    out << description() << "::describe" << std::endl;
    out << "model = " << model_->description() << std::endl;
  } else if (static_cast<int>(verbLevel) >= static_cast<int>(Teuchos::VERB_LOW)) {
  } else if (static_cast<int>(verbLevel) >= static_cast<int>(Teuchos::VERB_MEDIUM)) {
  } else if (static_cast<int>(verbLevel) >= static_cast<int>(Teuchos::VERB_HIGH)) {
    out << "model = " << std::endl;
    model_->describe(out,verbLevel,leadingIndent,indentSpacer);
    out << "solution_vector = " << std::endl;
    solution_vector_->describe(out,verbLevel,leadingIndent,indentSpacer);
    out << "residual_vector = " << std::endl;
    residual_vector_->describe(out,verbLevel,leadingIndent,indentSpacer);
  }
  return(out);
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::addPoints(
    const Array<Scalar>& time_vec
    ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& x_vec
    ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& xdot_vec
    )
{
  TEST_FOR_EXCEPTION(true,std::logic_error,"Error, addPoints is not implemented for ForwardEulerStepper.\n");
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::getPoints(
    const Array<Scalar>& time_vec
    ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* x_vec
    ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* xdot_vec
    ,Array<ScalarMag>* accuracy_vec) const
{
  TEST_FOR_EXCEPTION(true,std::logic_error,"Error, getPoints is not implemented for ForwardEulerStepper.\n");
}

template<class Scalar>
TimeRange<Scalar> ForwardEulerStepper<Scalar>::getTimeRange() const
{
  return invalidTimeRange<Scalar>();
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::getNodes(Array<Scalar>* time_vec) const
{
  TEST_FOR_EXCEPTION(true,std::logic_error,"Error, getNodes is not implemented for ForwardEulerStepper.\n");
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::removeNodes(Array<Scalar>& time_vec) 
{
  TEST_FOR_EXCEPTION(true,std::logic_error,"Error, removeNodes is not implemented for ForwardEulerStepper.\n");
}

template<class Scalar>
int ForwardEulerStepper<Scalar>::getOrder() const
{
  return(1);
}

template <class Scalar>
void ForwardEulerStepper<Scalar>::setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& paramList)
{
  parameterList_ = paramList;
  int outputLevel = parameterList_->get( "outputLevel", int(-1) );
  outputLevel = std::min(std::max(outputLevel,-1),4);
  this->setVerbLevel(static_cast<Teuchos::EVerbosityLevel>(outputLevel));
}

template <class Scalar>
Teuchos::RCP<Teuchos::ParameterList> ForwardEulerStepper<Scalar>::getNonconstParameterList()
{
  return(parameterList_);
}

template <class Scalar>
Teuchos::RCP<Teuchos::ParameterList> ForwardEulerStepper<Scalar>::unsetParameterList()
{
  Teuchos::RCP<Teuchos::ParameterList> temp_param_list = parameterList_;
  parameterList_ = Teuchos::null;
  return(temp_param_list);
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::setModel(const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model)
{
  TEST_FOR_EXCEPT(model == Teuchos::null)
  model_ = model;
}

template<class Scalar>
Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >
ForwardEulerStepper<Scalar>::getModel() const
{
  return model_;
}

} // namespace Rythmos

#endif //Rythmos_FORWARDEULER_STEPPER_H
