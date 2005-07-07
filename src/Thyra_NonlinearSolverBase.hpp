// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
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

#ifndef THYRA_NONLINEAR_SOLVER_BASE_HPP
#define THYRA_NONLINEAR_SOLVER_BASE_HPP

#include "Thyra_LinearOpWithSolveBaseDecl.hpp"
#include "Thyra_ModelEvaluator.hpp"

namespace Thyra {

/** \brief Base class for all nonlinear equation solvers.
 * 
 * ToDo: Finish documentation.
 */
template <class Scalar>
class NonlinearSolverBase {
public:

  /** \brief .*/
  virtual ~NonlinearSolverBase() {}
  
  /** @name Pure virtual functions that must be overridden in subclasses */
  //@{

  /** \brief Request the solution of a system of nonlinear equations.
   *
   * ToDo: Finish documentation.
   */
  virtual SolveStatus<Scalar> solve(
    const ModelEvaluator<Scalar>          &model
    ,VectorBase<Scalar>                   *x
    ,const SolveCriteria<Scalar>          *solveCriteria = NULL
    ) const = 0;

  //@}

};

} // namespace Thyra

#endif // THYRA_NONLINEAR_SOLVER_BASE_HPP
