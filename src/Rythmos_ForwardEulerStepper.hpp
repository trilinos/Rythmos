//
// @HEADER
// ***********************************************************************
// 
//                           Rythmos Package
//                 Copyright (2005) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef Rythmos_STEPPER_FORWARDEULER_H
#define Rythmos_STEPPER_FORWARDEULER_H

#include "Rythmos_Stepper.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_ModelEvaluator.hpp"

namespace Rythmos {

/** \brief . */
template<class Scalar>
class ForwardEulerStepper : public Stepper<Scalar>
{
  public:
    
    /** \brief . */
    ForwardEulerStepper();
    ForwardEulerStepper(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model);
    
    /** \brief . */
    ~ForwardEulerStepper();

    /** \brief . */
    Scalar TakeStep(Scalar dt);
   
    /** \brief . */
    Scalar TakeStep();

    /** \brief . */
    Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > get_solution() const;

    /** \brief . */
    Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > get_residual() const;
    
    /** \brief . */
    std::string description() const;

    /** \brief . */
    std::ostream& describe(
      std::ostream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ,const std::string          leadingIndent
      ,const std::string          indentSpacer
      ) const;


  private:

    Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > model_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > solution_vector_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > residual_vector_;
    Scalar t_;

};

template<class Scalar>
ForwardEulerStepper<Scalar>::ForwardEulerStepper(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model)
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  model_ = model;
  t_ = ST::zero();
  solution_vector_ = model_->get_x_init()->clone_v();
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
Scalar ForwardEulerStepper<Scalar>::TakeStep()
{
  // print something out about this method not supporting automatic variable step-size
  typedef Teuchos::ScalarTraits<Scalar> ST;
  return(-ST::one());
}

template<class Scalar>
Scalar ForwardEulerStepper<Scalar>::TakeStep(Scalar dt)
{
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

  return(dt);
}

template<class Scalar>
Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > ForwardEulerStepper<Scalar>::get_solution() const
{
  return(solution_vector_);
}

template<class Scalar>
Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > ForwardEulerStepper<Scalar>::get_residual() const
{
  return(residual_vector_);
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
  if (verbLevel == Teuchos::VERB_EXTREME)
  {
    out << description() << "::describe" << std::endl;
    out << "model_ = " << std::endl;
    out << model_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
    out << "solution_vector_ = " << std::endl;
    out << solution_vector_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
    out << "residual_vector_ = " << std::endl;
    out << residual_vector_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
  }
  return(out);
}

} // namespace Rythmos

#endif //Rythmos_STEPPER_FORWARDEULER_H
