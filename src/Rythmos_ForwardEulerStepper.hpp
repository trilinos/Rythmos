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
#include "Teuchos_RefCountPtr.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_ModelEvaluator.hpp"
#include "Thyra_ModelEvaluatorHelpers.hpp"

namespace Rythmos {

/** \brief . */
template<class Scalar>
class ForwardEulerStepper : virtual public StepperBase<Scalar>
{
  public:

    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;
    
    /** \brief . */
    ForwardEulerStepper();
    ForwardEulerStepper(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    void setModel(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> >
    getModel() const;
    
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
    bool setPoints(
      const std::vector<Scalar>& time_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec
      ,const std::vector<ScalarMag>& accuracy_vec
      );
    
    /// Get values from buffer
    bool getPoints(
      const std::vector<Scalar>& time_vec
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec
      ,std::vector<ScalarMag>* accuracy_vec
      ) const;

    /// Fill data in from another interpolation buffer
    bool setRange(
      const TimeRange<Scalar>& range,
      const InterpolationBufferBase<Scalar> & IB
      );

    /** \brief . */
    TimeRange<Scalar> getTimeRange() const;

    /// Get interpolation nodes
    bool getNodes(std::vector<Scalar>* time_vec) const;

    /// Remove interpolation nodes
    bool removeNodes(std::vector<Scalar>& time_vec);

    /// Get order of interpolation
    int getOrder() const;

    /// Redefined from Teuchos::ParameterListAcceptor
    /** \brief . */
    void setParameterList(Teuchos::RefCountPtr<Teuchos::ParameterList> const& paramList);

    /** \brief . */
    Teuchos::RefCountPtr<Teuchos::ParameterList> getParameterList();

    /** \brief . */
    Teuchos::RefCountPtr<Teuchos::ParameterList> unsetParameterList();

  private:

    Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > model_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > solution_vector_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > residual_vector_;
    Scalar t_;
    Scalar dt_;

    Teuchos::RefCountPtr<Teuchos::ParameterList> parameterList_;

};

template<class Scalar>
ForwardEulerStepper<Scalar>::ForwardEulerStepper(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model)
{
  Teuchos::RefCountPtr<Teuchos::FancyOStream> out = this->getOStream();
  out->precision(15);
  out->setMaxLenLinePrefix(30);
  //out->pushLinePrefix("Rythmos::ForwardEulerStepper");
  //out->setShowLinePrefix(true);
  //out->setTabIndentStr("    ");

  typedef Teuchos::ScalarTraits<Scalar> ST;
  model_ = model;
  t_ = ST::zero();
  solution_vector_ = model_->getNominalValues().get_x()->clone_v();
  residual_vector_ = Thyra::createMember(model_->get_f_space());
}

template<class Scalar>
ForwardEulerStepper<Scalar>::ForwardEulerStepper()
{
}

template<class Scalar>
ForwardEulerStepper<Scalar>::~ForwardEulerStepper()
{
}

template<class Scalar>
Scalar ForwardEulerStepper<Scalar>::takeStep(Scalar dt, StepSizeType flag)
{
  if (flag == VARIABLE_STEP) { 
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
bool ForwardEulerStepper<Scalar>::setPoints(
    const std::vector<Scalar>& time_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
    ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec
    ,const std::vector<ScalarMag> & accuracy_vec 
    )
{
  return(false);
}

template<class Scalar>
bool ForwardEulerStepper<Scalar>::getPoints(
    const std::vector<Scalar>& time_vec
    ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec
    ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec
    ,std::vector<ScalarMag>* accuracy_vec) const
{
  return(false);
}

template<class Scalar>
bool ForwardEulerStepper<Scalar>::setRange(
  const TimeRange<Scalar>& range,
  const InterpolationBufferBase<Scalar>& IB)
{
  return(false);
}

template<class Scalar>
TimeRange<Scalar> ForwardEulerStepper<Scalar>::getTimeRange() const
{
  return invalidTimeRange<Scalar>();
}

template<class Scalar>
bool ForwardEulerStepper<Scalar>::getNodes(std::vector<Scalar>* time_vec) const
{
  return(false);
}

template<class Scalar>
bool ForwardEulerStepper<Scalar>::removeNodes(std::vector<Scalar>& time_vec) 
{
  return(false);
}

template<class Scalar>
int ForwardEulerStepper<Scalar>::getOrder() const
{
  return(1);
}

template <class Scalar>
void ForwardEulerStepper<Scalar>::setParameterList(Teuchos::RefCountPtr<Teuchos::ParameterList> const& paramList)
{
  parameterList_ = paramList;
  int outputLevel = parameterList_->get( "outputLevel", int(-1) );
  outputLevel = min(max(outputLevel,-1),4);
  this->setVerbLevel(static_cast<Teuchos::EVerbosityLevel>(outputLevel));
}

template <class Scalar>
Teuchos::RefCountPtr<Teuchos::ParameterList> ForwardEulerStepper<Scalar>::getParameterList()
{
  return(parameterList_);
}

template <class Scalar>
Teuchos::RefCountPtr<Teuchos::ParameterList> ForwardEulerStepper<Scalar>::unsetParameterList()
{
  Teuchos::RefCountPtr<Teuchos::ParameterList> temp_param_list = parameterList_;
  parameterList_ = Teuchos::null;
  return(temp_param_list);
}

template<class Scalar>
void ForwardEulerStepper<Scalar>::setModel(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model)
{
  TEST_FOR_EXCEPT(model == Teuchos::null)
  model_ = model;
}

template<class Scalar>
Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> >
ForwardEulerStepper<Scalar>::getModel() const
{
  return model_;
}

} // namespace Rythmos

#endif //Rythmos_FORWARDEULER_STEPPER_H
