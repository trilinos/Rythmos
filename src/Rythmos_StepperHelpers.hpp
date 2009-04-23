
#ifndef RYTHMOS_STEPPER_HELPERS_HPP
#define RYTHMOS_STEPPER_HELPERS_HPP


#include "Rythmos_Types.hpp"
#include "Rythmos_StepperBase.hpp"
#include "Teuchos_Assert.hpp"
#include "Thyra_AssertOp.hpp"
#include "Thyra_ModelEvaluator.hpp"
#include "Rythmos_InterpolationBufferHelpers.hpp"
#include "Rythmos_InterpolatorBase.hpp"
#include "Rythmos_InterpolatorBaseHelpers.hpp"


namespace Rythmos {


/** \brief Assert valid transient model for StepperBase.
 *
 */
template<class Scalar>
void assertValidModel(
  const StepperBase<Scalar>& stepper,
  const Thyra::ModelEvaluator<Scalar>& model
  )
{

  typedef Thyra::ModelEvaluatorBase MEB;

  TEUCHOS_ASSERT(stepper.acceptsModel());

  const MEB::InArgs<Scalar> inArgs = model.createInArgs();
  const MEB::OutArgs<Scalar> outArgs = model.createOutArgs();

  //TEUCHOS_ASSERT(inArgs.supports(MEB::IN_ARG_t));
  TEUCHOS_ASSERT(inArgs.supports(MEB::IN_ARG_x));
  TEUCHOS_ASSERT(outArgs.supports(MEB::OUT_ARG_f));
  
  if (stepper.isImplicit()) { // implicit stepper
    TEUCHOS_ASSERT( inArgs.supports(MEB::IN_ARG_x_dot) );
    TEUCHOS_ASSERT( inArgs.supports(MEB::IN_ARG_alpha) );
    TEUCHOS_ASSERT( inArgs.supports(MEB::IN_ARG_beta) );
    TEUCHOS_ASSERT( outArgs.supports(MEB::OUT_ARG_W) );
  } 
  //else { // explicit stepper
  //  TEUCHOS_ASSERT( !inArgs.supports(MEB::IN_ARG_x_dot) );
  //  TEUCHOS_ASSERT( !inArgs.supports(MEB::IN_ARG_alpha) );
  //  TEUCHOS_ASSERT( !inArgs.supports(MEB::IN_ARG_beta) );
  //  TEUCHOS_ASSERT( !outArgs.supports(MEB::OUT_ARG_W) );
  //}

}


/** \brief Set an initial condition on a stepper from a model if the stepper
 * does not already have an initial condition.
 *
 * \returns Returns <tt>true</tt> if the stepper->setInitialCondition(...) was
 * called and returns <tt>false</tt> otherwise.
 */
template<class Scalar>
bool setDefaultInitialConditionFromNominalValues(
  const Thyra::ModelEvaluator<Scalar>& model,
  const Ptr<StepperBase<Scalar> >& stepper
  )
{

  typedef ScalarTraits<Scalar> ST;
  typedef Thyra::ModelEvaluatorBase MEB;

  if (isInitialized(*stepper))
    return false;  // Already has an initial condition
  
  MEB::InArgs<Scalar> initCond = model.getNominalValues();

  if (!is_null(initCond.get_x())) {
    // IC has x, we will assume that initCont.get_t() is the valid start time.
    // Therefore, we just need to check that x_dot is also set or we will
    // create a zero x_dot
#ifdef RYTHMOS_DEBUG
    THYRA_ASSERT_VEC_SPACES( "setInitialConditionIfExists(...)", 
      *model.get_x_space(), *initCond.get_x()->space() );
#endif
    if (initCond.supports(MEB::IN_ARG_x_dot)) {
      if (is_null(initCond.get_x_dot())) {
        const RCP<Thyra::VectorBase<Scalar> > x_dot =
          createMember(model.get_x_space());
        assign(x_dot.ptr(), ST::zero());
      }
      else {
#ifdef RYTHMOS_DEBUG
        THYRA_ASSERT_VEC_SPACES( "setInitialConditionIfExists(...)", 
          *model.get_x_space(), *initCond.get_x_dot()->space() );
#endif
      }
    }
    stepper->setInitialCondition(initCond);
    return true;
  }

  // The model has not nominal values for which to set the initial
  // conditions so wo don't do anything!  The stepper will still have not
  return false;

}


/** \brief Restart a time stepper.
 *
 * This simple helper function just grabs the state out of the
 * <tt>*stepper</tt> object and then resets it on itself as an initial
 * condition.  This is generally used to restart a stepper when passing over a
 * breakpoint where the model is expected to be discontinuous in some way.
 */
template<class Scalar>
void restart( StepperBase<Scalar> *stepper )
{
#ifdef RYTHMOS_DEBUG
  TEST_FOR_EXCEPT(0==stepper);
#endif // RYTHMOS_DEBUG
  typedef Thyra::ModelEvaluatorBase MEB;
  const Rythmos::StepStatus<double>
    stepStatus = stepper->getStepStatus();
  const RCP<const Thyra::ModelEvaluator<Scalar> >
    model = stepper->getModel();
  // First, copy all of the model's state, including parameter values etc.
  MEB::InArgs<double> initialCondition = model->createInArgs();
  initialCondition.setArgs(model->getNominalValues());
  // Set the current values of the state and time
  RCP<const Thyra::VectorBase<double> > x, x_dot;
  Rythmos::get_x_and_x_dot(*stepper,stepStatus.time,&x,&x_dot);
  initialCondition.set_x(x);
  initialCondition.set_x_dot(x_dot);
  initialCondition.set_t(stepStatus.time);
  // Set the new initial condition back on the stepper.  This will effectively
  // reset the stepper to think that it is starting over again (which it is).
  stepper->setInitialCondition(initialCondition);
}

template<class Scalar>
void eval_model_explicit(
    const Thyra::ModelEvaluator<Scalar> &model,
    Thyra::ModelEvaluatorBase::InArgs<Scalar> &basePoint,
    const VectorBase<Scalar>& x_in,
    const typename Thyra::ModelEvaluatorBase::InArgs<Scalar>::ScalarMag &t_in,
    const Ptr<VectorBase<Scalar> >& f_out
    )
{
  typedef Thyra::ModelEvaluatorBase MEB;
  MEB::InArgs<Scalar> inArgs = model.createInArgs();
  MEB::OutArgs<Scalar> outArgs = model.createOutArgs();
  inArgs.setArgs(basePoint);
  inArgs.set_x(Teuchos::rcp(&x_in,false));
  if (inArgs.supports(MEB::IN_ARG_t)) {
    inArgs.set_t(t_in);
  }
  outArgs.set_f(Teuchos::rcp(&*f_out,false));
  model.evalModel(inArgs,outArgs);
}

template<class Scalar>
void eval_model_explicit_poly(
    const Thyra::ModelEvaluator<Scalar> &model,
    Thyra::ModelEvaluatorBase::InArgs<Scalar> &basePoint,
    const Teuchos::Polynomial< VectorBase<Scalar> > &x_poly,
    const typename Thyra::ModelEvaluatorBase::InArgs<Scalar>::ScalarMag &t,
    const Ptr<Teuchos::Polynomial<VectorBase<Scalar> > >& f_poly
    )
{
  typedef Thyra::ModelEvaluatorBase MEB;
  MEB::InArgs<Scalar> inArgs = model.createInArgs();
  MEB::OutArgs<Scalar> outArgs = model.createOutArgs();
  inArgs.setArgs(basePoint);
  inArgs.set_x_poly(Teuchos::rcp(&x_poly,false));
  if (inArgs.supports(MEB::IN_ARG_t)) {
    inArgs.set_t(t);
  }
  outArgs.set_f_poly(Teuchos::rcp(&*f_poly,false));

  model.evalModel(inArgs,outArgs);
}


// This function simply returns the boundary points if they're asked for.  Otherwise it throws.
template<class Scalar>
void defaultGetPoints(
    const Scalar& t_old, // required inArg
    const Ptr<const VectorBase<Scalar> >& x_old, // optional inArg
    const Ptr<const VectorBase<Scalar> >& xdot_old, // optional inArg
    const Scalar& t, // required inArg
    const Ptr<const VectorBase<Scalar> >& x, // optional inArg
    const Ptr<const VectorBase<Scalar> >& xdot, // optional inArg
    const Array<Scalar>& time_vec, // required inArg
    const Ptr<Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > > >& x_vec, // optional outArg
    const Ptr<Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > > >& xdot_vec, // optional outArg
    const Ptr<Array<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> >& accuracy_vec, // optional outArg
    const Ptr<InterpolatorBase<Scalar> > interpolator // optional inArg (note:  not const)
    ) 
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  assertTimePointsAreSorted(time_vec);
  TimeRange<Scalar> tr(t_old,t);
  TEUCHOS_ASSERT( tr.isValid() );
  if (!is_null(x_vec)) {
    x_vec->clear();
  }
  if (!is_null(xdot_vec)) {
    xdot_vec->clear();
  }
  if (!is_null(accuracy_vec)) {
    accuracy_vec->clear();
  }
  typename Array<Scalar>::const_iterator time_it = time_vec.begin();
  RCP<const VectorBase<Scalar> > tmpVec;
  RCP<const VectorBase<Scalar> > tmpVecDot;
  for (; time_it != time_vec.end() ; time_it++) {
    Scalar time = *time_it;
    TEUCHOS_ASSERT( tr.isInRange(time) );
    Scalar accuracy = ST::zero();
    if (compareTimeValues(time,t_old)==0) {
      if (!is_null(x_old)) {
        tmpVec = x_old->clone_v();
      }
      if (!is_null(xdot_old)) {
        tmpVecDot = xdot_old->clone_v();
      }
    } else if (compareTimeValues(time,t)==0) {
      if (!is_null(x)) {
        tmpVec = x->clone_v();
      }
      if (!is_null(xdot)) {
        tmpVecDot = xdot->clone_v();
      }
    } else {
      TEST_FOR_EXCEPTION(
          is_null(interpolator), std::logic_error,
          "Error, getPoints:  This stepper only supports time values on the boundaries!\n"
          );
      // At this point, we know time != t_old, time != t, interpolator != null, 
      // and time in [t_old,t], therefore, time in (t_old,t).  
      // t_old != t at this point because otherwise it would have been caught above.
      // Now use the interpolator to pass out the interior points
      typename DataStore<Scalar>::DataStoreVector_t ds_nodes;
      typename DataStore<Scalar>::DataStoreVector_t ds_out;
      {
        // t_old
        DataStore<Scalar> ds;
        ds.time = t_old;
        ds.x = rcp(x_old.get(),false);
        ds.xdot = rcp(xdot_old.get(),false);
        ds_nodes.push_back(ds);
      }
      {
        // t
        DataStore<Scalar> ds;
        ds.time = t;
        ds.x = rcp(x.get(),false);
        ds.xdot = rcp(xdot.get(),false);
        ds_nodes.push_back(ds);
      }
      Array<Scalar> time_vec_in;
      time_vec_in.push_back(time);
      interpolate<Scalar>(*interpolator,rcp(&ds_nodes,false),time_vec_in,&ds_out);
      Array<Scalar> time_vec_out;
      Array<RCP<const VectorBase<Scalar> > > x_vec_out;
      Array<RCP<const VectorBase<Scalar> > > xdot_vec_out;
      Array<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> accuracy_vec_out;
      dataStoreVectorToVector(ds_out,&time_vec_out,&x_vec_out,&xdot_vec_out,&accuracy_vec_out);
      TEUCHOS_ASSERT( time_vec_out.length()==1 );
      tmpVec = x_vec_out[0];
      tmpVecDot = xdot_vec_out[0];
      accuracy = accuracy_vec_out[0];
    }
    if (!is_null(x_vec)) {
      x_vec->push_back(tmpVec);
    }
    if (!is_null(xdot_vec)) {
      xdot_vec->push_back(tmpVecDot);
    }
    if (!is_null(accuracy_vec)) {
      accuracy_vec->push_back(accuracy);
    }
    tmpVec = Teuchos::null;
    tmpVecDot = Teuchos::null;
  }
}


} // namespace Rythmos


#endif // RYTHMOS_STEPPER_HELPERS_HPP
