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

#ifndef Rythmos_BACKWARD_EULER_STEPPER_H
#define Rythmos_BACKWARD_EULER_STEPPER_H

#include "Rythmos_Stepper.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_ModelEvaluator.hpp"
#include "Thyra_NonlinearSolverBase.hpp"
#include "Thyra_SingleResidSSDAEModelEvaluator.hpp"

namespace Rythmos {

/** \brief . */
template<class Scalar>
class BackwardEulerStepper : public Stepper<Scalar>
{
  public:

    /** \brief . */
    BackwardEulerStepper(
      const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> >        &model
      ,const Teuchos::RefCountPtr<const Thyra::NonlinearSolverBase<Scalar> >  &solver
      );

    /** \brief . */
    void setModel(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    void setSolver(const Teuchos::RefCountPtr<const Thyra::NonlinearSolverBase<Scalar> > &solver);

    /** \brief . */
    Scalar TakeStep(Scalar dt);
   
    /** \brief . */
    Scalar TakeStep();

    /** \brief . */
    Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > get_solution() const;

    /** \brief . */
    Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > get_residual() const;

  private:

    Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > model_;
    Teuchos::RefCountPtr<const Thyra::NonlinearSolverBase<Scalar> > solver_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > x_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > scaled_x_old_;
    Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > f_;
    Scalar t_;
    Scalar t_old_;

    Thyra::SingleResidSSDAEModelEvaluator<Scalar>   neModel_;

};

// ////////////////////////////
// Defintions

template<class Scalar>
BackwardEulerStepper<Scalar>::BackwardEulerStepper(
  const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model
  ,const Teuchos::RefCountPtr<const Thyra::NonlinearSolverBase<Scalar> > &solver
  )
{
  setModel(model);
  setSolver(solver);
}

template<class Scalar>
void BackwardEulerStepper<Scalar>::setModel(const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model)
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  model_ = model;
  t_ = ST::zero();
  x_ = model_->get_x_init()->clone_v();
  f_ = Thyra::createMember(model_->get_f_space());

  scaled_x_old_ = x_->clone_v();
  
}

template<class Scalar>
void BackwardEulerStepper<Scalar>::setSolver(const Teuchos::RefCountPtr<const Thyra::NonlinearSolverBase<Scalar> > &solver)
{
  solver_ = solver;
}

template<class Scalar>
Scalar BackwardEulerStepper<Scalar>::TakeStep()
{
  // print something out about this method not supporting automatic variable step-size
  typedef Teuchos::ScalarTraits<Scalar> ST;
  return(-ST::one());
}

template<class Scalar>
Scalar BackwardEulerStepper<Scalar>::TakeStep(Scalar dt)
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  //
  // Setup the nonlinear equations:
  //
  //   f( (1/dt)* x + (-1/dt)*x_old), x, t ) = 0
  //
  V_StV( &*scaled_x_old_, Scalar(-ST::one()/dt), *x_ );
  t_old_ = t_;
  neModel_.initialize(model_,Scalar(ST::one()/dt),scaled_x_old_,ST::one(),Teuchos::null,t_old_+dt,Teuchos::null);
  //
  // Solve the implicit nonlinear system to a tolerance of ???
  //
  solver_->solve( neModel_, &*x_ ); // Note that x in input is x_old!
  //
  // Update the step
  //
  t_ += dt;

  return(dt);
}

template<class Scalar>
Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > BackwardEulerStepper<Scalar>::get_solution() const
{
  return(x_);
}

template<class Scalar>
Teuchos::RefCountPtr<const Thyra::VectorBase<Scalar> > BackwardEulerStepper<Scalar>::get_residual() const
{
  return(f_);
}


} // namespace Rythmos

#endif //Rythmos_BACKWARD_EULER_STEPPER_H
